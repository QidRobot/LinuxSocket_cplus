#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>
#include <ctype.h>

#include "wrap.h"

#define SRV_PORT 8000
#define OPEN_MAX 1024
#define MAXLINE 80

int main(int argc, char *argv[])
{
	int i, j, maxi, listenfd, connfd, sockfd;
	int nready;
	ssize_t n;

	char buf[MAXLINE], str[INET_ADDRSTRLEN];/*#define INET_ADDRSTRLEN 16*/
	socklen_t clilen;
	struct pollfd client[OPEN_MAX];
	struct sockaddr_in cliaddr, serv_addr;
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);
	//�˿ڸ��� �����ȹرշ���˺�رտͻ����޷�����
	int opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt));
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SRV_PORT);//ָ���˿ں�
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);//ָ����������IP
	Bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));//��
	Listen(listenfd, 128);//����ͬһʱ�̷�������������
	printf("Accept client connecting������\n");

	client[0].fd = listenfd;
	client[0].events = POLLIN;
	//���������ļ�����������ֵΪ-1
	for (i = 1; i < OPEN_MAX; i ++)
	{
		client[i].fd = -1;
	}
	maxi = 0;//ָ��listenfd;
	for (;;)
	{
		nready = poll(client, maxi + 1, -1);
		//����listenfd
		if (client[0].revents & POLLIN)//�ж��Ƿ�Ϊ���¼�
		{
			clilen = sizeof(cliaddr);
			connfd = Accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);//�������� ...��������
			printf("received from %s at PORT %d\n", inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)), ntohs(cliaddr.sin_port));
			for (i = 1; i < OPEN_MAX; i++)
			{
				if (client[i].fd < 0)		/*�ҵ�client[]��û��ʹ�õ�λ��--���仰˵���������õĿռ�*/
				{
					client[i].fd = connfd;	/*����accept���ص��ļ���������client[]��*/
					break;
				}
			}
			if (i == OPEN_MAX)				/*˵���ﵽ�����ͻ�������*/
			{
				perr_exit("too many clients!!!");
			}
			client[i].events = POLLIN;/*���øոշ��ص�connfd,��ض��¼�*/
			if (i > maxi)
			{
				maxi = i;/*����client[]������Ԫ���±�*/
			}
			if (--nready <= 0)
			{
				continue;/*û�и�������¼�ʱ�������ص�poll����*/
			}
		}
		for (i = 1; i <= maxi; i++)/*����ĸ�clients�����ݾ���*/
		{
			if ((sockfd = client[i].fd) < 0)
			{
				continue;/*�쳣����*/
			}
			if (client[i].revents & POLLIN)//�ҵ�������¼���fd
			{
				if ((n = Read(sockfd, buf, MAXLINE)) < 0)//��⵽�ͻ����Ѿ��ر�����
				{
					//������������ֵС��0��ʱ�������������Ҫ����
					if (errno == ECONNABORTED)/*�յ�RST��־*/
					{
						printf("client[%d] aborted connection\n", i);
						Close(sockfd);
						client[i].fd = -1;/*poll�в���ظ��ļ������� ֱ����Ϊ-1���� ������select�������Ƴ�*/
					}
					else
					{
						perr_exit("read error!!!");
					}
				}
				else if (n == 0)	/*˵���ͻ����ȹر�����*/
				{
					printf("client[%d] closed connection\n", i);
					Close(sockfd);
					client[i].fd = -1;
				}
				else
				{
					for (j = 0; j < n; j++)
					{
						buf[j] = toupper(buf[j]);
					}
					Writen(sockfd, buf, n);//��д���ݸ��ͻ���
					//�����ݽ��а�װ��ʾ
					Write(STDOUT_FILENO, buf, n);//��ʾ�ڷ�������
				}
				if (--nready <= 0)
				{
					break;
				}
			}
		}
	}
	return 0;
}