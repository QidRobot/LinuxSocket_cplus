#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h> //--mkfifo
#include <fcntl.h>
#include <pthread.h>
#include <event2/event.h>
//#include <event2/event.h>
void sys_err(const char *str)
{
	perror(str);
	exit(1);
}

void write_cb(evutil_socket_t fd, short what, void *arg)
{
	char buf[] = "hello libevent";
	write(fd, buf, strlen(buf) + 1);
	sleep(1);
	return;
}
//void (*event_callback_fn)(evutil_socket_t fd, short what, void *arg);

int main(int argc, char *argv[])
{
	//���� fifo
	unlink("testfifo");
	mkfifo("testfifo", 0644);
	//��fifo��д��
	int fd = open("testfifo", O_WRONLY | O_NONBLOCK);//���÷�����
	if (fd == -1)
	{
		sys_err("open error");
	}
	//����event_base;
	struct event_base *base = event_base_new();

	//�����¼� ����
	struct event *ev = NULL;

	//����¼���event_base�� //EV_PERSIST����д
	//struct event *event_new(struct event_base *base, evutil_socket_t fd, short what, event_callback_fn cb, void *arg);
	ev = event_new(base, fd, EV_WRITE | EV_PERSIST, write_cb, NULL);

	//����¼���event_base��
	//int event_add(struct event *ev, const struct timeval *tv);
	event_add(ev, NULL);

	//����ѭ��
	//int event_base_dispatch(struct event_base *base,);	
	event_base_dispatch(base);

	//����event_base;
	event_base_free(base);
}