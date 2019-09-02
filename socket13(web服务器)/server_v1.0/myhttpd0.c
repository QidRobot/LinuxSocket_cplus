#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXSIZE 2048

int init_listen_fd(int port, int epfd)
{
	//�����������׽��� lfd
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1)
	{
		perror("socket error");
		exit(1);
	}
	//������������ַ�ṹ IP + port
	struct sockaddr_in srv_addr;

	bzero(&srv_addr, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(port);
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//�˿ڸ���
	int opt = 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt);

	//��lfd �󶨵�ַ�ṹ
	int ret = bind(lfd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
	if (ret == -1)
	{
		perror("bind error");
		exit(1);
	}
	//���ü�������
	ret = listen(lfd, 128);
	if (ret == -1)
	{
		perror("listen error");
		exit(1);
	}
	//lfd ��ӵ� epoll����
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = lfd;

	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
	if (ret == -1)
	{
		perror("epoll_ctl add lfd error");
		exit(1);
	}
	return lfd;
}

void do_accept(int lfd, int epfd)
{
	struct sockaddr_in clt_addr;
	socklen_t clt_addr_len = sizeof(clt_addr);

	int cfd = accept(lfd, (struct sockaddr*)&clt_addr, &clt_addr_len);
	if (cfd == -1)
	{
		perror("accept error");
		exit(1);
	}

	//��ӡ�ͻ���IP + port
	char client_ip[64] = { 0 };
	printf("New Client IP: %s, Port: %d, cfd = %d\n",
		inet_ntop(AF_INET, &clt_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)),
			ntohs(clt_addr.sin_port), cfd
		);

	//����cfd ������
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(cfd, F_SETFL, flag);

	//���½ڵ�cfd�ҵ�epoll��������
	struct epoll_event ev;
	ev.data.fd = cfd;

	//���ط�����ģʽ ��ETģʽ
	ev.events = EPOLLIN | EPOLLET;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, &ev);
	if (ret == -1)
	{
		perror("epoll_ctl add cfd error");
		exit(1);
	}
}

void do_read(int cfd, int epfd)
{
	//read cfd С--�� write��
	//��ȡһ��httpЭ�� ��� ��ȡget�ļ��� Э���
}

void epoll_run(int port)
{
	int i = 0;
	struct epoll_event all_events[MAXSIZE];

	//����һ��epoll��������
	int epfd = epoll_create(MAXSIZE);
	if (epfd == -1)
	{
		perror("epoll_create error");
		exit(1);
	}

	//����lfd,�������������
	int lfd = init_listen_fd(port, epfd);

	while (1)
	{
		//�����ڵ��Ӧ�¼�
		int ret = epoll_wait(epfd, all_events, MAXSIZE, -1);//�����ķ�ʽ ������������ѯ
		if (ret == -1)
		{
			perror("epoll_wait error");
			exit(1);
		}
		for (i = 0; i < ret; i ++)
		{
			//ֻ������¼� �����¼�Ĭ�ϲ�����
			struct epoll_event *pev = &all_events[i];

			//���Ƕ��¼�
			if (!(pev->events & EPOLLIN))
			{
				continue;
			}
			if (pev->data.fd == lfd)	//������������
			{
				do_accept(lfd, epfd);
			}
			else//������
			{
				do_read(pev->data.fd, epfd);
			}
		}
	}
}

int main(int argc, char *argv[])
{
	//�����в�����ȡ�˿� �� server�ṩ��Ŀ¼
	if (argc < 3)
	{
		printf("./server port path\n");
	}
	//��ȡ�û�����Ķ˿�
	int port = atoi(argv[1]);
	//�Ľ����̹���Ŀ¼
	int ret = chdir(argv[2]);
	if (ret != 0)
	{
		perror("chdir error");
		exit(1);
	}
	//���� epoll����
	epoll_run(port);
	return 0;
}