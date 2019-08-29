#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>  
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <stddef.h>

#include "wrap.h"

//��Ҫ�����ͻ��˺ͷ���˵��׽��� ��Ϊ�����ӷ����ʱ��Ҫʹ�õ�����˵��׽��ֽṹ
#define SERV_ADDR "serv.socket"
#define CLIE_ADDR "clie.socket"

int main(void)
{
	int  cfd, len;
	struct sockaddr_un servaddr, cliaddr;
	char buf[4096];
	cfd = Socket(AF_UNIX, SOCK_STREAM, 0);
	bzero(&cliaddr, sizeof(cliaddr));
	cliaddr.sun_family = AF_UNIX;
	strcpy(cliaddr.sun_path, CLIE_ADDR);
	len = offsetof(struct sockaddr_un, sun_path) + strlen(cliaddr.sun_path);     /* ����ͻ��˵�ַ�ṹ��Ч���� */
	unlink(CLIE_ADDR);
	Bind(cfd, (struct sockaddr *)&cliaddr, len);                                 /* �ͻ���Ҳ��Ҫbind, ���������Զ���*/
	/*����˵�ַ��ʼ��*/
	bzero(&servaddr, sizeof(servaddr));                                          /* ����server ��ַ */
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, SERV_ADDR);

	len = offsetof(struct sockaddr_un, sun_path) + strlen(servaddr.sun_path);   /* ����������˵�ַ�ṹ��Ч���� */
	//int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
	Connect(cfd, (struct sockaddr *)&servaddr, len);
	//fgets(buf, sizeof(buf), stdin) != NULL �ȴ��ͻ��˽������� ������ǽ�����־�򲻻�ֹͣ
	while (fgets(buf, sizeof(buf), stdin) != NULL) {
		write(cfd, buf, strlen(buf));//���ַ���д�뵽�׽����н��д���
		len = read(cfd, buf, sizeof(buf));//���׽����е����� ��buf�� �������ַ����� ����
		write(STDOUT_FILENO, buf, len);//�����ӡ���ն�
	}
	close(cfd);
	return 0;
}