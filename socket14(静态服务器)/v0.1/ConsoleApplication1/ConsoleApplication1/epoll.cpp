#include "epoll.h"
#include <sys/epoll.h>
#include <errno.h>
#include "threadpool.h"
struct epoll_event* events;
//�����Ƕ�epollģ�͵��ĸ������ķ�װ �ֱ�Ϊ epoll_create epoll_ctl epoll_wait
int epoll_init()
{
	//�ں˴���ָ����С�ĺ���� �ں�ͬʱ�ᴴ���¼���˫������
	int epoll_fd = epoll_create(LISTENQ + 1);
	/*epoll�������������-�ɹ����طǸ� �ļ������� ���󷵻� -1
	#include <sys/epoll.h>
	int epoll_create(int size);*/
	if (epoll_fd == -1)
	{
		return -1;
	}
	//�����¼�����
	events = new epoll_event[MAXEVENTS];//5000
	return epoll_fd;
}
//ע����������
int epoll_add(int epoll_fd, int fd, void *request, __uint32_t events)
{
	struct epoll_event event;
	event.data.ptr = request;
	event.events = events;
	//int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
	{
		perror("epoll_add error");
		return -1;
	}
	return 0;
}
//�޸�������״̬
int epoll_mod(int epoll_fd, int fd, void *request, __uint32_t events)
{
	struct epoll_event event;
	event.data.ptr = request;
	event.events = events;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) < 0)
	{
		perror("epoll_add error");
		return -1;
	}
	return 0;
}
//��epoll��ɾ��������
int epoll_del(int epoll_fd, int fd, void *request, __uint32_t events)
{
	struct epoll_event event;
	event.data.ptr = request;
	event.events = events;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event) < 0)
	{
		perror("epoll_add error");
		return -1;
	}
	return 0;
}
//���ػ�Ծ�¼���
int my_epoll_wait(int epoll_fd, struct epoll_event *events, int max_events, int timeout)
{
	int ret_count = epoll_wait(epoll_fd, events, max_events, timeout);
	if (ret_count < 0)
	{
		perror("epoll wait error");
	}
	return ret_count;
}