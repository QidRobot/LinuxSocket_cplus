#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include "wrap.h"

#define SRV_PORT 8000

int main(int argc, char *argv[])
{
	int i, j, n, maxi;

	int nready, client[FD_SETSIZE];/*�Զ�������client ��ֹ����1024���ļ������� FD_SETSIZEĬ��Ϊ1024*/
	int maxfd, listenfd, connfd, sockfd;//connectfd
	char buf[BUFSIZ], str[INET_ADDRSTRLEN];/*#define INET_ADDRSTRLEN 16*/
	
	struct sockaddr_in clie_addr, serv_addr;
	socklen_t clie_ddr_len;
	fd_set rset, allset;//��������� ���ݼ���allset

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

	maxfd = listenfd;//����ļ�������

	maxi = -1;//���Ƚ�maxfd_index��������±�ָ��-1
	for (i = 0; i < FD_SETSIZE; i++)
	{
		client[i] = -1;//������������λ���ȶ�������Ϊ-1
	}
	FD_ZERO(&allset);//��ռ�������
	FD_SET(listenfd, &allset);//��������fd��ӵ�����������

	while (1)
	{
		rset = allset;//����
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);//ʹ��select���� 
		if (nready < 0)
		{
			perr_exit("select error");
		}
		if (FD_ISSET(listenfd, &rset))//�ж�listenfd�Ƿ���rset��	//listenfd������¼�
		{
			clie_ddr_len = sizeof(clie_addr);
			connfd = Accept(listenfd, (struct sockaddr*)&clie_addr, &clie_ddr_len);//�������� ...��������
			printf("received from %s at PORT %d\n", inet_ntop(AF_INET, &clie_addr.sin_addr, str, sizeof(str)), ntohs(clie_addr.sin_port));
			for (i = 0; i < FD_SETSIZE; i ++)
			{
				if (client[i] < 0)		/*�ҵ�client[]��û��ʹ�õ�λ��*/
				{
					client[i] = connfd;	/*����accept���ص��ļ���������client[]��*/
					break;
				}
			}
			if(i == FD_SETSIZE)	/*�ﵽselect�ܼ������ļ�����*/
			{
				fputs("too many clients\n", stderr);
			}
			FD_SET(connfd, &allset);/*�����ļ�����������allset����µ��ļ�������connfd*/
			if (connfd > maxfd)
			{
				maxfd = connfd;/*select��һ��������Ҫʹ������ļ������� + 1*/
			}
			if (i > maxi)	/*��֤maxi�������client[]���һ��Ԫ���±�*/
			{
				maxi = i;
			}
			if (--nready == 0)//���ֻ��һ���ļ���������Ϊlistenfd������ֱ�ӽ�����һ��ѭ��
			{
				continue;
			}
		}
		for (i = 0; i <= maxi; i++)/*����ĸ�clients�����ݾ���*/
		{
			if ((sockfd = client[i]) < 0)
			{
				continue;
			}
			if (FD_ISSET(sockfd, &rset))//�ҵ�������¼���fd
			{
				if((n = Read(sockfd, buf, sizeof(buf))) == 0)//��⵽�ͻ����Ѿ��ر�����
				{
					Close(sockfd);
					FD_CLR(sockfd, &allset);/*���select�Դ��ļ��������ļ��*/
					client[i] = -1;
				}
				else if (n > 0)
				{
					for (j = 0; j < n; j++)
					{
						buf[j] = toupper(buf[j]);
					}
					Write(sockfd, buf, n);//д���ͻ���
					Write(STDOUT_FILENO, buf, n);//��ʾ�ڷ�������
				}
				
			}
		}
	}
	Close(listenfd);
	return 0;
}