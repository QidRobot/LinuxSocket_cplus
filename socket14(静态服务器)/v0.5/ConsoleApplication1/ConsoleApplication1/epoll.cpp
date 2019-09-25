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

//extern std::priority_queue<std::shared_ptr<mytimer>, std::deque<std::shared_ptr<mytimer>>, timerCmp> myTimerQueue;

/*��ؼ�ס��̬����������г�ʼ��*/
epoll_event *Epoll::events;
//MAXFDS = 1000��ʾ���Ŷ��ٸ�����
Epoll::SP_ReqData Epoll::fd2req[MAXFDS];
//std::unordered_map<int, std::shared_ptr<RequestData>> Epoll::fd2req;
int Epoll::epoll_fd = 0;
const std::string Epoll::PATH = "/home/jacob/dir";

TimerManager Epoll::timer_manager;

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
int Epoll::epoll_add(int fd, SP_ReqData request, __uint32_t events)
{
	struct epoll_event event;
	event.data.fd = fd;
	event.events = events;
	fd2req[fd] = request;
	//int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
	{
		perror("epoll_add error(epoll)");
		return -1;
	}
	return 0;
}

//�޸�������״̬
int Epoll::epoll_mod(int fd, SP_ReqData request, __uint32_t events)
{
	struct epoll_event event;
	event.data.fd = fd;
	event.events = events;
	fd2req[fd] = request;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) < 0)
	{
		perror("epoll_mod error");
		/*��shared_ptrָ�����͵�request ������� ������ε��޸��¼�û�гɹ�*/
		fd2req[fd].reset();
		return -1;
	}
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
	/*auto fd_ite = fd2req.find(fd);
	if (fd_ite != fd2req.end())
	{
		fd2req.erase(fd_ite);
	}*/
	fd2req[fd].reset();
	return 0;
}

//���ػ�Ծ�¼���
void Epoll::my_epoll_wait(int listen_fd, int max_events, int timeout)
{
	//timeout == -1��ʾepoll_waitһֱ���� 0��ʾֱ�ӷ��أ���ʹû���¼��������
	int event_count = epoll_wait(epoll_fd, events, max_events, timeout);
	if (event_count < 0)
	{
		perror("epoll wait error");
	}
	std::vector<SP_ReqData> req_data = getEventsRequest(listen_fd, event_count, PATH);
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
	//��ʱ��������¼� ����ʱ����С���ѵ����ȼ���ʱ�������е��� 
	timer_manager.handle_expired_event();
}

#include <iostream>
#include <arpa/inet.h>
using namespace std;
void Epoll::acceptConnection(int listen_fd, int epoll_fd, const std::string path)
{
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(struct sockaddr_in));
	//socklen_t client_addr_len = 0; �޸�����ĵ�ַ���� �ڶ�ȡIP��ַ�Ͷ˿�ʱ�Ͳ���0
	socklen_t client_addr_len = sizeof(client_addr);
	int accept_fd = 0;
	//char client_IP[1024];
	while ((accept_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len)) > 0)
	{
		cout << inet_ntoa(client_addr.sin_addr) << endl;
		cout << ntohs(client_addr.sin_port) << endl;
		/*����ʹ�����·�ʽ���д�ӡ*/
		//printf("client ip: %s port: %d\n",
		//	inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_IP, sizeof(client_IP)),
		//	/*������˿� ת����  ��������˿�*/
		//	ntohs(client_addr.sin_port)
		//);
		/*
		// TCP�ı������Ĭ���ǹرյ�
		int optval = 0;
		socklen_t len_optval = 4;
		getsockopt(accept_fd, SOL_SOCKET,  SO_KEEPALIVE, &optval, &len_optval);
		cout << "optval ==" << optval << endl;
		*/
		//���Ʒ���������󲢷�������
		if (accept_fd >= MAXFDS)
		{
			close(accept_fd);
			continue;
		}
		//���÷�����ģʽ ������������� util.h�е�
		int ret = setSockNonBlocking(accept_fd);
		if (ret < 0)
		{
			perror("Set non block failed");
			return;
		}
		//��������ǰ�汾����������ֻ�ǽ��˴�����ָ���Ϊ����ָ�����
		SP_ReqData req_info(new RequestData(epoll_fd, accept_fd, path));
		/*2019��9��11��09:50:32 ����˵�� �½�*/
		//requestData *req_info = new requestData(epoll_fd, accept_fd, path);

		//�ļ����������Զ� ��Ե����ģʽ ��֤һ��socket��������һʱ��ֻ��һ���̴߳���
		//��ʹ����EPOLLETģʽ�����п���һ���׽����ϵ��¼�����δ��� ��˲��� EPOLLONESHOT��˼��
		__uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
		//int Epoll::epoll_add(int fd, std::shared_ptr<requestData> request, __uint32_t events)
		Epoll::epoll_add(accept_fd, req_info, _epo_event);
		
		timer_manager.addTimer(req_info, TIMER_TIME_OUT);
		//����ʱ����Ϣ �滻��ָ��Ϊ����ָ��
		//std::shared_ptr<mytimer> mtimer(new mytimer(req_info, TIMER_TIME_OUT));
		//mytimer *mtimer = new mytimer(req_info, TIMER_TIME_OUT);
		//req_info->addTimer(mtimer);
		//pthread_mutex_lock(&qlock);
		//MutexLockGuard lock;
		//myTimerQueue.push(mtimer);
		//pthread_mutex_unlock(&qlock);
	}
}

//�ַ�������
std::vector<std::shared_ptr<RequestData>> Epoll::getEventsRequest(int listen_fd, int events_num, const std::string path)
{
	std::vector<SP_ReqData> req_data;
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
			printf("fd < 3\n");
			break;
		}
		else
		{
			//�ų������¼�
			if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP))
			{
				printf("error event\n");
				if (fd2req[fd])
				{
					//����ʱ�����¼�������з���
					fd2req[fd]->seperateTimer();
				}
				//��ո��¼�����
				fd2req[fd].reset();
				/*auto fd_ite = fd2req.find(fd);
				if (fd_ite != fd2req.end())
				{
					fd2req.erase(fd_ite);
				}*/
				//printf("error event\n");
				//delete request;
				continue;
			}

			//������������뵽�̳߳����������
			//�����̳߳�֮ǰ��Timer��request���� ��˵����ɾ��ʱ��
			SP_ReqData cur_req = fd2req[fd];
			//SP_ReqData cur_req(fd2req[fd]);

			//�����Ӧfd��������Ļ�
			if (cur_req)
			{
				//�ж��¼��Ƿ�ɶ�
				if ((events[i].events & EPOLLIN) || (events[i].events & EPOLLPRI))
				{
					cur_req->enableRead();
				}
				//�ж��¼��Ƿ��д
				else 
				{
					cur_req->enableWrite();
				}
				cur_req->seperateTimer();
				req_data.push_back(cur_req);
				fd2req[fd].reset();
			}
			else
			{
				cout << "SP cur_req is invalid" << endl;
			}
			/*cur_req->seperateTimer();
			req_data.push_back(cur_req);
			auto fd_ite = fd2req.find(fd);
			if (fd_ite != fd2req.end())
			{
				fd2req.erase(fd_ite);
			}*/

			//int threadpool_add(threadpool_t *pool, void(*function)(void *), void *argument, int flags)
			/*����ص�������Ϊ�����������ݵĻص�����  myHandler -�� handleRequest*/
			//int rc = threadpool_add(tp, myHandler, events[i].data.ptr, 0);
		}
	}
	return req_data;
}
void Epoll::add_timer(SP_ReqData request_data, int timeout)
{
	timer_manager.addTimer(request_data, timeout);
}
