/*��Ҫ����epollģ��*/
/*
��Ҫ������������
	int epoll_create(int size);
	int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
	int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <ctype.h>

#include "wrap.h"

#define SRV_PORT 8000
/*���ļ����� �����Ҫ�޸�����Ҫ�޸������ļ�*/
#define OPEN_MAX 1024
#define MAXLINE 8192

int main(int argc, char *argv[])
{
	int i, j, listenfd, connfd, sockfd;
	int n, num = 0;
	ssize_t nready, efd, res;
	char buf[MAXLINE], str[INET_ADDRSTRLEN];/*#define INET_ADDRSTRLEN 16*/
	socklen_t clilen;
	
	struct sockaddr_in cliaddr, servaddr;
	/*epoll�ж��е� �����������ñ���*/
	struct epoll_event tep, ep[OPEN_MAX];

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);
	//�˿ڸ��� �����ȹرշ���˺�رտͻ����޷�����
	int opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt));
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SRV_PORT);//ָ���˿ں�
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//ָ����������IP
	Bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));//��
	Listen(listenfd, 20);//����ͬһʱ�̷�������������
	printf("Accept client connecting������\n");

	efd = epoll_create(OPEN_MAX);//����epollģ�� efdָ���������ڵ�
	if (efd == -1)
	{
		perr_exit("epoll_create error");
	}

	/*��ʼ��*/
	tep.events = EPOLLIN;//ָ��lfd�ļ���ʱ��Ϊ"��"
	tep.data.fd = listenfd;

	res = epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &tep);//��lfd����Ӧ�Ľṹ�����õ����ϣ�efd���ҵ�����
	if (res == -1)
	{
		perr_exit("epoll_ctl error");
	}

	for (;;)
	{
		/*epollΪserver���������¼� epΪstruct epoll_envent �������飬OPEN_MAXΪ����������-1��ʾ��������*/
		nready = epoll_wait(efd, ep, OPEN_MAX, -1);
		if (nready == -1)
		{
			perr_exit("epoll_wait error");
		}
		for (i = 0; i < nready; i ++)
		{
			if (!(ep[i].events & EPOLLIN))//������Ƕ��¼� ����ѭ��
			{
				continue;
			}
			if (ep[i].data.fd == listenfd)//�ж��������¼���fd�Ƿ�Ϊlfd
			{
				clilen = sizeof(cliaddr);
				connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);//��������

				printf("received from %s at PORT %d\n", inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)), ntohs(cliaddr.sin_port));
				printf("cfd %d---client %d\n", connfd, ++num);

				tep.events = EPOLLIN;
				tep.data.fd = connfd;
				res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);
				if (res == -1)
				{
					perr_exit("epoll_ctl error");
				}
			}
			else
			{
				sockfd = ep[i].data.fd;
				n = Read(sockfd, buf, MAXLINE);

				if (n == 0)//����0��˵���ͻ��˹ر�����
				{
					res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);//�����ļ��������Ӻ����ժ��
					if (res == -1)
					{
						perr_exit("epoll_ctl error");
					}
					Close(sockfd);//�ر���ÿͻ��˵�����
					printf("client[%d] closed connection\n", sockfd);
				}
				else if (n < 0)
				{
					perror("read n < 0 error: ");//����
					res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);//ժ���ڵ�
					Close(sockfd);
				}
				else
				{
					for (j = 0; j < n; j ++)
					{
						buf[j] = toupper(buf[j]);
					}
					Write(STDOUT_FILENO, buf, n);
					Write(sockfd, buf, n);
				}
			}
		}
	}
	Close(listenfd);
	Close(efd);
	return 0;
}