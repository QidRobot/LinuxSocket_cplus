#pragma once
/*��static�ؼ���*/
#ifndef EVENTPOLL
#define EVENTPOLL
#include "requestData.h"
#include "timer.h"
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <memory>
//��Epoll������װ��һ����
class Epoll 
{
public:
	//ʹ��SP_ReqData �滻 std::shared_ptr<RequestData>
	typedef std::shared_ptr<RequestData> SP_ReqData;
private:
	static const int MAXFDS = 1000;
	static epoll_event *events;
	//������ָ��������͵ı��� ���뵽hash���� ��hash���еĴ洢�� fd(�׽���) --> ��Ӧ����
	//static std::unordered_map<int, SP_ReqData> fd2req;
	/*������ķ�ʽ���洢���� shared_ptr<RequestData>���͵�����*/
	static SP_ReqData fd2req[MAXFDS];
	static int epoll_fd;
	static const std::string PATH;

	static TimerManager timer_manager;
public:
	//9.2�޸�����
	static int epoll_init(int maxevents, int listen_num);
	static int epoll_add(int fd, SP_ReqData request, __uint32_t events);
	static int epoll_mod(int fd, SP_ReqData request, __uint32_t events);
	//���¼�����ֱ������Ϊ���ش���ģʽ ��һ�δ���
	static int epoll_del(int fd, __uint32_t events = (EPOLLIN | EPOLLET | EPOLLONESHOT));
	static void my_epoll_wait(int listen_fd, int max_events, int timeout);
	static void acceptConnection(int listen_fd, int epoll_fd, const std::string path);
	//����ָ��vector���� ����ָ��Ķ�����requestData
	static std::vector<SP_ReqData> getEventsRequest(int listen_fd, int events_num, const std::string path);

	static void add_timer(SP_ReqData request_data, int timeout);
};
#endif