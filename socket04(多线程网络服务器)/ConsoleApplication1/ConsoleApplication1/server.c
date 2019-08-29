#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include "wrap.h"
#define MAXLINE 8192
#define SRV_PORT 8000

struct s_info
{
	struct sockaddr_in cliaddr;
	int connfd;
};
void *do_work(void *arg)
{
	int n, i;
	struct s_info *ts = (struct s_info*)arg;
	char buf[MAXLINE];
	char str[INET_ADDRSTRLEN]; //���ֵ����16
	while (1)
	{
		n = Read(ts->connfd, buf, MAXLINE);//���ͻ���
		if (n == 0)
		{
			printf("the client %d closed...\n", ts->connfd);
			break;//����ѭ�� �ر�cfd
		}
		printf("received from %s at PORT %d\n", inet_ntop(AF_INET, &(*ts).cliaddr.sin_addr, str, sizeof(str)), ntohs((*ts).cliaddr.sin_port));
		for (i = 0; i < n; i ++)
		{
			buf[i] = toupper(buf[i]);
		}
		Write(STDOUT_FILENO, buf, n);
		Write(ts->connfd, buf, n);
	}
	Close(ts->connfd);
	return (void *)0;
	//pthread_exit(0);Ч��һ��
}
int main(int argc, char *argv[])
{
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddr_len;
	int listenfd, connfd;
	pthread_t tid;

	struct s_info ts[256];//�����ṹ������
	int i = 0;

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);//����һ��socket �õ������׽���	

	//�˿ڸ��� �����쳣�رշ���˺��޷�����
	int opt = 1;

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt));

	//memset(&srv_addr, 0, sizeof(srv_addr));	//����ַ�ṹ����
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SRV_PORT);//ָ���˿ں�
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//ָ����������IP

	Bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));//��

	Listen(listenfd, 128);//����ͬһʱ�̷�������������
	printf("Accepting client connect....\n");

	while (1)
	{
		cliaddr_len = sizeof(cliaddr);
		connfd = Accept(listenfd, (struct sockaddr*)&cliaddr, &cliaddr_len);//���������ͻ�����������
		ts[i].cliaddr = cliaddr;
		ts[i].connfd = connfd;
		
		pthread_create(&tid, NULL, do_work, (void*)&ts[i]);
		pthread_detach(tid);//���̷߳��� ��ֹ��ʬ�̲߳���
		i++;
	}
	return 0;
}