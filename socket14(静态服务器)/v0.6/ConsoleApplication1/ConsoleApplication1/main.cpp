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
#include <memory>
#include "Logging.h"

using namespace std;

const int MAXEVENTS = 5000;
const int LISTENQ = 1024;
//�̳߳����߳���������Ϊ4 ����Ϊ˫��6�߳�
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

//extern struct epoll_event* events;
void acceptConnection(int listen_fd, int epoll_fd, const string &path);

//���ȼ�����
//extern std::priority_queue<mytimer*, std::deque<mytimer*>, timerCmp> myTimerQueue;
//extern std::priority_queue<shared_ptr<TimerNode>, std::deque<shared_ptr<TimerNode>>, timerCmp> myTimerQueue;

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

	//�����˿ڸ��ò��� ����������ʮ�ֳ��� ����bindʱ "Address already in use"�Ĵ���
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
//void handle_expired_event()
//{
//	MutexLockGuard lock;
//	//pthread_mutex_lock(&qlock);
//	while (!myTimerQueue.empty())
//	{
//		shared_ptr<mytimer> ptimer_now = myTimerQueue.top();
//		if (ptimer_now->isDeleted())
//		{
//			myTimerQueue.pop();
//			//delete ptimer_now;��������ָ��Ͳ���Ҫ�ٽ��� delete�ͷŶ���Դ��
//		}
//		else if (ptimer_now->isvalid() == false)
//		{
//			myTimerQueue.pop();
//			//delete ptimer_now;
//		}
//		else
//		{
//			break;
//		}
//	}
//	//pthread_mutex_unlock(&qlock);
//}

int main()
{
	LOG << "hello world!!!";
	LOG << 654 << 3.2 << 0 << string("fg") << true;
	/*����#ifndef��Ϊ���ڱ�����ʱ �ܹ�������ε��ÿ�*/
	#ifndef _PTHREADS
		printf("_PTHREADS is not defined !\n");
	#endif
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
	if (Epoll::epoll_init(MAXEVENTS, LISTENQ) < 0)
	{
		/*������epoll���ж����� epoll_fd-epoll�׽��� ��epoll_init�����м��ᴴ�����׽���*/
		perror("epoll init failed");
		return 1;
	}
	//�����̳߳�
	if (ThreadPool::threadpool_create(THREADPOOL_THREAD_NUM, QUEUE_SIZE) < 0)
	{
		printf("Threadpool create failed\n");
		return 1;
	}
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

	shared_ptr<RequestData> request(new RequestData());
	//requestData *req = new requestData(); ������ָ��ͳһ������Դ
	request->setFd(listen_fd);
	//req->setFd(listen_fd);//�趨�׽���
	//�������¼����뵽epoll�� ͬʱ����req����
	//static int epoll_add(int fd, std::shared_ptr<requestData> request, __uint32_t events);
	//������ľ�̬��Ա/*���ü����¼�ΪET���ش���ģ�� �ɶ�д*/
	if (Epoll::epoll_add(listen_fd, request, EPOLLIN | EPOLLET) < 0)
	{
		perror("epoll_add error(main)");
		return 1;
	}
	while (true)
	{
		Epoll::my_epoll_wait(listen_fd, MAXEVENTS, -1);
		//ͨ���ж϶�ʱ���Ƿ���� ����ɾ����ʱ�� �Ӷ�ɾ������ ����
		//handle_expired_event();
	}
	return 0;
}