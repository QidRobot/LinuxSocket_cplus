#include "epoll.h"
#include "threadpool.h"
#include "util.h"
#include <sys/epoll.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <deque>
#include <queue>

int TIMER_TIME_OUT = 500;
extern std::priority_queue<std::shared_ptr<mytimer>, std::deque<std::shared_ptr<mytimer>>, timerCmp> myTimerQueue;

epoll_event *Epoll::events;
std::unordered_map<int, std::shared_ptr<requestData>> Epoll::fd2req;
int Epoll::epoll_fd = 0;
const std::string Epoll::PATH = "/home/jacob/dir";


//�����Ƕ�epollģ�͵��ĸ������ķ�װ �ֱ�Ϊ epoll_create epoll_ctl epoll_wait
int Epoll::epoll_init(int maxevents, int listen_num)
{
	//�ں˴���ָ����С�ĺ���� �ں�ͬʱ�ᴴ���¼���˫������
	epoll_fd = epoll_create(listen_num + 1);
	/*epoll�������������-�ɹ����طǸ� �ļ������� ���󷵻� -1
	#include <sys/epoll.h>
	int epoll_create(int size);*/
	if (epoll_fd == -1)
	{
		return -1;
	}
	//events.reset(new epoll_event[maxevents], [](epoll_event *data){delete [] data;});
	//�����¼�����
	events = new epoll_event[maxevents];//5000
	return 0;
}
//ע����������
int Epoll::epoll_add(int fd, std::shared_ptr<requestData> request, __uint32_t events)
{
	struct epoll_event event;
	event.data.fd = fd;
	event.events = events;
	//int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
	{
		perror("epoll_add error(epoll)");
		return -1;
	}
	fd2req[fd] = request;
	return 0;
}
//�޸�������״̬
int Epoll::epoll_mod(int fd, std::shared_ptr<requestData> request, __uint32_t events)
{
	struct epoll_event event;
	event.data.fd = fd;
	event.events = events;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) < 0)
	{
		perror("epoll_mod error");
		return -1;
	}
	fd2req[fd] = request;
	return 0;
}
//��epoll��ɾ��������
int Epoll::epoll_del(int fd, __uint32_t events)
{
	struct epoll_event event;
	event.data.fd = fd;
	event.events = events;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event) < 0)
	{
		perror("epoll_add error");
		return -1;
	}
	//c++11�����е� auto����
	auto fd_ite = fd2req.find(fd);
	if (fd_ite != fd2req.end())
	{
		fd2req.erase(fd_ite);
	}
	return 0;
}
//���ػ�Ծ�¼���
void Epoll::my_epoll_wait(int listen_fd, int max_events, int timeout)
{
	int event_count = epoll_wait(epoll_fd, events, max_events, timeout);
	if (event_count < 0)
	{
		perror("epoll wait error");
	}
	std::vector<std::shared_ptr<requestData>> req_data = getEventsRequest(listen_fd, event_count, PATH);
	if (req_data.size() > 0)
	{
		for (auto &req : req_data)
		{
			if (ThreadPool::threadpool_add(req) < 0)
			{
				//�̳߳����˻��߹ر��˵�ԭ���������μ�����������
				break;
			}
		}
	}
}
#include <iostream>
#include <arpa/inet.h>
using namespace std;
void Epoll::acceptConnection(int listen_fd, int epoll_fd, const std::string path)
{
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(struct sockaddr_in));
	socklen_t client_addr_len = 0;
	int accept_fd = 0;
	char client_IP[1024];
	while ((accept_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len)) > 0)
	{
		printf("client ip: %s port: %d\n",
			inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_IP, sizeof(client_IP)),
			/*������˿� ת����  ��������˿�*/
			ntohs(client_addr.sin_port)
		);
		/*
		// TCP�ı������Ĭ���ǹرյ�
		int optval = 0;
		socklen_t len_optval = 4;
		getsockopt(accept_fd, SOL_SOCKET,  SO_KEEPALIVE, &optval, &len_optval);
		cout << "optval ==" << optval << endl;
		*/
		//���÷�����ģʽ ������������� util.h�е�
		int ret = setSockNonBlocking(accept_fd);
		if (ret < 0)
		{
			perror("Set non block failed");
			return;
		}
		//��������ǰ�汾����������ֻ�ǽ��˴�����ָ���Ϊ����ָ�����
		std::shared_ptr<requestData> req_info(new requestData(epoll_fd, accept_fd, path));
		//requestData *req_info = new requestData(epoll_fd, accept_fd, path);
		//�ļ����������Զ� ��Ե����ģʽ ��֤һ��socket��������һʱ��ֻ��һ���̴߳���
		//��ʹ����EPOLLETģʽ�����п���һ���׽����ϵ��¼�����δ��� ��˲��� EPOLLONESHOT��˼��
		__uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
		
		//int Epoll::epoll_add(int fd, std::shared_ptr<requestData> request, __uint32_t events)
		Epoll::epoll_add(accept_fd, req_info, _epo_event);
		
		//����ʱ����Ϣ �滻��ָ��Ϊ����ָ��
		std::shared_ptr<mytimer> mtimer(new mytimer(req_info, TIMER_TIME_OUT));
		//mytimer *mtimer = new mytimer(req_info, TIMER_TIME_OUT);
		req_info->addTimer(mtimer);
		//pthread_mutex_lock(&qlock);
		MutexLockGuard lock;
		myTimerQueue.push(mtimer);
		//pthread_mutex_unlock(&qlock);
	}
}
//�ַ�������
std::vector<std::shared_ptr<requestData>> Epoll::getEventsRequest(int listen_fd, int events_num, const std::string path)
{
	std::vector<std::shared_ptr<requestData>> req_data;
	for (int i = 0; i < events_num; i++)
	{
		//��ȡ���¼�������������
		int fd = events[i].data.fd;

		//���¼�������������Ϊ����������
		if (fd == listen_fd)
		{
			acceptConnection(listen_fd, epoll_fd, path);
		}
		else if (fd < 3)
		{
			break;
		}
		else
		{
			//�ų������¼�
			if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)))
			{
				auto fd_ite = fd2req.find(fd);
				if (fd_ite != fd2req.end())
				{
					fd2req.erase(fd_ite);
				}
				//printf("error event\n");
				//delete request;
				continue;
			}
			//������������뵽�̳߳���
			//�����̳߳�֮ǰ��Timer��request����
			std::shared_ptr<requestData> cur_req(fd2req[fd]);
			cur_req->seperateTimer();
			req_data.push_back(cur_req);
			auto fd_ite = fd2req.find(fd);
			if (fd_ite != fd2req.end())
			{
				fd2req.erase(fd_ite);
			}
			//int threadpool_add(threadpool_t *pool, void(*function)(void *), void *argument, int flags)
			/*����ص�������Ϊ�����������ݵĻص�����  myHandler -�� handleRequest*/
			//int rc = threadpool_add(tp, myHandler, events[i].data.ptr, 0);
		}
	}
	return req_data;
}
