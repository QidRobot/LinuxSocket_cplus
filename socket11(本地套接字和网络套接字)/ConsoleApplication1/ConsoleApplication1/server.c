#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <stddef.h>
/*�����׽��� ���ο����������׽��ֽ�����ʽ�����и�д��*/
#include "wrap.h"

//��������socket�׽����ļ�
#define SERV_ADDR  "serv.socket"
int main(void)
{
	int lfd, cfd, len, size, i;
	//�����׽��ֺ������׽��ֵĵ�һ���������ڷ������˺Ϳͻ��˵Ľṹ���ַ�������׽��ֲ�ͬ
	struct sockaddr_un servaddr, cliaddr;
	char buf[4096];
	//int socket(int domain, int type, int protocol);
	//�ڶ����������ڴ� AF_UNIX
	lfd = Socket(AF_UNIX, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_UNIX;
	//��SERV_ADDR�ļ���ַ��������������ַ�ṹ��ȥ
	strcpy(servaddr.sun_path, SERV_ADDR);
	// offsetof(struct sockaddr_un, sun_path) ��ƫ������õ�ֵ��Ϊ2
	len = offsetof(struct sockaddr_un, sun_path) + strlen(servaddr.sun_path);
	unlink(SERV_ADDR);/* ȷ��bind֮ǰserv.sock�ļ�������,bind�ᴴ�����ļ� */
	//int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
	Bind(lfd, (struct sockaddr *)&servaddr, len);/* ��3������sizeof(servaddr) ������Ҫȷ�еı�ʾ����С*/
	//int listen(int sockfd, int backlog);
	Listen(lfd, 20);
	printf("Accept ...\n");
	while (1)
	{
		len = sizeof(cliaddr);
		//accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
		cfd = Accept(lfd, (struct sockaddr *)&cliaddr, (socklen_t *)&len);
		len -= offsetof(struct sockaddr_un, sun_path);      /* �õ��ļ����ĳ��� offsetof(struct sockaddr_un, sun_path) = 2*/
		cliaddr.sun_path[len] = '\0';                       /* ȷ����ӡʱ,û��������� */
		printf("client bind filename %s\n", cliaddr.sun_path);
		while ((size = read(cfd, buf, sizeof(buf))) > 0) {
			for (i = 0; i < size; i++)
				buf[i] = toupper(buf[i]);
			//д���׽����� ���д���
			write(cfd, buf, size);
			write(STDOUT_FILENO, buf, len);//�����ӡ���ն�
		}
		close(cfd);
	}
	close(lfd);
	return 0;
}