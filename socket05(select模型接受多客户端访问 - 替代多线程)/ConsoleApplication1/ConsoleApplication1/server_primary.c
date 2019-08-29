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
	int listenfd, connfd;//connectfd

	struct sockaddr_in clie_addr, serv_addr;
	socklen_t clie_ddr_len;

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
	fd_set rset, allset;//��������� ���ݼ���allset
	int ret, maxfd = 0, n, i, j;
	char buf[BUFSIZ];
	maxfd = listenfd;//����ļ�������

	FD_ZERO(&allset);//��ռ�������
	FD_SET(listenfd, &allset);//��������fd��ӵ�����������
	
	while (1)
	{
		rset = allset;//����
		ret = select(maxfd + 1, &rset, NULL, NULL, NULL);//ʹ��select����
		if (ret < 0)
		{
			perr_exit("select error");
		}
		/*
			����������Ĵ�����һ���ԵĽ��ڶ����������뵽������	
			����ÿ��ѭ���Ľ��� �����������뵽������ ÿ��ֻ����һ��
		*/
		if (FD_ISSET(listenfd, &rset))//�ж�listenfd�Ƿ���rset��	//listenfd������¼�
		{
			clie_ddr_len = sizeof(clie_addr);
			connfd = Accept(listenfd, (struct sockaddr*)&clie_addr, &clie_ddr_len);//�������� ...��������
			FD_SET(connfd, &allset);//���²�����fd��ӵ�����������
			if (maxfd < connfd)
			{
				maxfd = connfd;//�޸�maxfd
			}
			if (ret == 1)	//˵��selectֻ����һ�� ������listenfd ����ָ������ִ��
			{
				continue;//����һ��whileѭ��
			}
		}
		/*
			ÿ�ζ���ѭ���Ĳ��Ҳ鿴�Ƿ��пͻ������������������
		*/
		for (i = listenfd + 1; i <= maxfd; i ++)//����������¼���fd
		{
			if (FD_ISSET(i, &rset))//�ҵ�������¼���fd
			{
				n = Read(i, buf, sizeof(buf));
				if (n == 0)//��⵽�ͻ����Ѿ��ر�����
				{
					Close(i);
					FD_CLR(i, &allset);//���رյ�fd,�Ƴ�����������
				}
				else if(n == -1)
				{ 
					perr_exit("read error");
				}
				for (j = 0; j < n; j ++)
				{
					buf[j] = toupper(buf[j]);
				}
				Write(i, buf, n);//д���ͻ���
				Write(STDOUT_FILENO, buf, n);//��ʾ�ڷ�������
			}
		}
	}

	Close(listenfd);
}