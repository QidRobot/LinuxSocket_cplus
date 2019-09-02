//�Լ���װ�ĺ�����
#include "requestData.h"
#include "epoll.h"
#include "threadpool.h"
#include "util.h"
//���õ�ϵͳ��
#include <sys/epoll.h>
#include <queue>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <unistd.h>

using namespace std;

//�̳߳����߳���������Ϊ4
const int THREADPOOL_THREAD_NUM = 4;
//�̳߳��е�������д�С
const int QUEUE_SIZE = 65535;

const int PORT = 8888;
const int ASK_STATIC_FILE = 1;
const int ASK_IMAGE_STITCH = 2;

//ͨ������·���ı���̵�ִ��·�� �Ӷ��ﵽ����ָ���ļ�������
const char *PATH = "/home/jacob/dir";
//const string PATH = "/home/jacob/dir";
//const string PATH = "/";

const int TIMER_TIME_OUT = 500;

//externһ����ʹ���ڶ��ļ�֮����Ҫ����ĳЩ����ʱ��
//extern pthread_mutex_t qlock; ��Ϊʹ����RALL������ ��˲����ٵ���������

extern struct epoll_event* events;
void acceptConnection(int listen_fd, int epoll_fd, const string &path);

//���ȼ�����
extern priority_queue<mytimer*, deque<mytimer*>, timerCmp> myTimerQueue;

int socket_bind_listen(int port)
{
	//���port��ֵ ȡ��ȷ���䷶Χ
	if (port < 1024 || port > 65535)
	{
		return -1;
	}
	//����socket(IPV4 + TCP)�����ؼ����׽���������
	int listen_fd = 0;
	if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		return -1;
	}
	//�����˿ڸ��ò��� ����������ʮ�ֳ���
	int optval = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
	{
		return -1;
	}
	//���÷�����IP��Port �Լ�������������
	struct sockaddr_in server_addr;
	bzero((char*)&server_addr, sizeof(server_addr));//��սṹ���ڴ�
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons((unsigned short)port);
	if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
	{
		return -1;
	}
	//��ʼ���� ���ȴ����г���Ϊ LISTENQ ����epoll_create���Ѿ�������LISTENQ��С�ĺ����
	//#define LISTENQ 1024 //  int listen(int sockfd, int backlog);
	if (listen(listen_fd, LISTENQ) == -1)/*�ɹ�����0 ʧ�ܷ���-1*/
	{
		return -1;
	}
	//��Ч�ļ���������
	if (listen_fd == -1)
	{
		close(listen_fd);
		return -1;
	}
	return listen_fd;
}

void myHandler(void *args)
{
	requestData *req_data = (requestData*)args;
	req_data->handleRequest();
}

void acceptConnection(int listen_fd, int epoll_fd, const string &path)
{
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(struct sockaddr_in));
	socklen_t client_addr_len = 0;
	int accept_fd = 0;
	while ((accept_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len)) > 0)
	{
		/*
		// TCP�ı������Ĭ���ǹرյ�
		int optval = 0;
		socklen_t len_optval = 4;
		getsockopt(accept_fd, SOL_SOCKET,  SO_KEEPALIVE, &optval, &len_optval);
		cout << "optval ==" << optval << endl;
		*/
		//���÷�����ģʽ
		int ret = setSockNonBlocking(accept_fd);
		if (ret < 0)
		{
			perror("Set non block failed");
			return;
		}
		requestData *req_info = new requestData(epoll_fd, accept_fd, path);
		//�ļ����������Զ� ��Ե����ģʽ ��֤һ��socket��������һʱ��ֻ��һ���̴߳���
		//��ʹ����EPOLLETģʽ�����п���һ���׽����ϵ��¼�����δ���
		__uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
		epoll_add(epoll_fd, accept_fd, static_cast<void*>(req_info), _epo_event);
		//����ʱ����Ϣ
		mytimer *mtimer = new mytimer(req_info, TIMER_TIME_OUT);
		req_info->addTimer(mtimer);
		//pthread_mutex_lock(&qlock);
		{
			MutexLockGuard();
		}
		myTimerQueue.push(mtimer);
		//pthread_mutex_unlock(&qlock);
	}
}
//�ַ�������
void handle_events(int epoll_fd, int listen_fd, struct epoll_event* events, int events_num, const string &path, threadpool_t* tp)
{
	for (int i = 0; i < events_num; i ++)
	{
		//��ȡ���¼�������������
		requestData* request = (requestData*)(events[i].data.ptr);
		int fd = request->getFd();
		//���¼�������������Ϊ����������
		if (fd == listen_fd)
		{
			acceptConnection(listen_fd, epoll_fd, path);
		}
		else
		{
			//�ų������¼�
			if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)))
			{
				printf("error event\n");
				delete request;
				continue;
			}
			//������������뵽�߳���
			//�����̳߳�֮ǰ��Timer��request����
			request->seperateTimer();
			//int threadpool_add(threadpool_t *pool, void(*function)(void *), void *argument, int flags)
			/*����ص�������Ϊ�����������ݵĻص�����  myHandler -�� handleRequest*/
			int rc = threadpool_add(tp, myHandler, events[i].data.ptr, 0);
		}
	}
}
/*�����߼�
��Ϊ(1)���ȶ��в�֧���������
(2)��ʹ֧�� ���ɾ��ĳ�ڵ���ƻ��˶ѵĽṹ ��Ҫ���¸��¶ѽṹ
���Զ��ڱ���Ϊdeleted��ʱ��ڵ� ���ӳٵ���(1)��ʱ����(2)ǰ��Ľڵ㶼��ɾ�� ���Ż�ɾ��
һ���㱻��Ϊdeleted �������TIMER_TIMER_OUTʱ���ɾ��
�������������ô�
(1)��һ���ô��ǲ���Ҫ�������ȼ����� ʡʱ
(2)�ڶ����ô��Ǹ���ʱʱ��һ������ʱ�� �����趨�ĳ�ʱʱ����ɾ�������ޣ�������һ����ʱʱ�������ɾ����
��������������ڳ�ʱ�����һ���������ֳ�����һ�ξͲ�����������requestData�ڵ��ˣ��������Լ����ظ�����
ǰ���requestData ������һ��delete��һ��new��ʱ��
*/
//��������¼� ���ڶ�ʱ�����ȼ�������ΪС���ѣ�����ֻ��Ҫ�ж�����Ƿ��ȥ���ɣ�������û�й��� ��ô˵�����ж�ʱ�����������
void handle_expired_event()
{
	MutexLockGuard();
	//pthread_mutex_lock(&qlock);
	while (!myTimerQueue.empty())
	{
		mytimer *ptimer_now = myTimerQueue.top();
		if (ptimer_now->isDeleted())
		{
			myTimerQueue.pop();
			delete ptimer_now;
		}
		else if (ptimer_now->isvalid() == false)
		{
			myTimerQueue.pop();
			delete ptimer_now;
		}
		else
		{
			break;
		}
	}
	//pthread_mutex_unlock(&qlock);
}
int main()
{
	//����PATHָ���Ĺ���·�� �ı���̵�ִ��·��
	int ret = chdir(PATH);
	if (ret != 0)
	{
		perror("chdir error");
		exit(1);
	}
	//�źŴ�����
	handle_for_sigpipe();
	//����epoll�����ģ�� �׽���
	int epoll_fd = epoll_init();
	if (epoll_fd < 0)
	{
		perror("epoll init failed");
		return 1;
	}
	//�����̳߳�
	threadpool_t *threadpool = threadpool_create(THREADPOOL_THREAD_NUM, QUEUE_SIZE, 0);
	//���������׽���
	int listen_fd = socket_bind_listen(PORT);
	if (listen_fd < 0)
	{
		perror("socket bind failed");
		return 1;
	}
	//���÷���������ģʽ
	if (setSockNonBlocking(listen_fd) < 0)
	{
		perror("set socket non block failed");
		return 1;
	}
	__uint32_t event = EPOLLIN | EPOLLET;
	requestData *req = new requestData();
	req->setFd(listen_fd);//�趨�׽���
	//�������¼����뵽epoll�� ͬʱ����req����
	epoll_add(epoll_fd, listen_fd, static_cast<void*>(req), event);
	while (true)
	{
		//int my_epoll_wait(int epoll_fd, struct epoll_event *events, int max_events, int timeout)
		int events_num = my_epoll_wait(epoll_fd, events, MAXEVENTS, -1);
		if (events_num == 0)
		{
			continue;
		}
		printf("%d\n", events_num);
		//����events���飬���ݼ������༰���������ͷַ�����
		handle_events(epoll_fd, listen_fd, events, events_num, PATH, threadpool);
		//����
		handle_expired_event();
	}
	return 0;
}