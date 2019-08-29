#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#define SERV_PORT 9527
void sys_err(const char *str)
{
	perror(str);
	exit(1);
}
int main(int argc, char *argv[])
{
	int lfd = 0, cfd = 0;
	int ret;
	int i;
	char buf[BUFSIZ], client_IP[1024];//BUFSIZ 4096
	struct sockaddr_in serv_addr, clit_addr;
	socklen_t clit_addr_len;
	/*�������Ҫ���������õĶ˿ں�IP��ַת���������ֽ���*/
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);//htons ��ʾ16λ������
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);//htons ��ʾ32λ������
	//�½�һ�������׽���
	lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1)
	{
		sys_err("socket error!!!");
	}
	//sockaddr_in  ��ַ�ṹ ��һ�ֽṹ������
	bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	listen(lfd, 128);

	clit_addr_len = sizeof(clit_addr);
	//���ܿͻ��˵�
	cfd = accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);
	if (cfd == -1)
	{
		sys_err("accept error");
	}
	/*inet_ntop ������ip��ַת���ɱ���ip��ַ  ��������ip��ַ�ͱ���ip��ַ�����ǲ�ͬ����������*/
	printf("client ip: %s port: %d\n",
		inet_ntop(AF_INET, &clit_addr.sin_addr.s_addr, client_IP, sizeof(client_IP)),
		/*������˿� ת����  ��������˿�*/
		ntohs(clit_addr.sin_port)
	);
	while (1)
	{
		ret = read(cfd, buf, sizeof(buf));
		//д�����ػ�����
		write(STDOUT_FILENO, buf, ret);
		for (i = 0; i < ret; i++)
		{
			buf[i] = toupper(buf[i]);
		}
		//ͨ���ļ�������д���ͻ���
		write(cfd, buf, ret);
	}	
	close(lfd);
	close(cfd);
	return 0;
}