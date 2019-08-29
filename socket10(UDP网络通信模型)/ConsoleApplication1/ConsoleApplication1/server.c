#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#define SERV_PORT 8000
int main(void)
{
	struct sockaddr_in serv_addr, clie_addr;
	socklen_t clie_addr_len;
	int sockfd;
	char buf[BUFSIZ];//BUFSIZ ��СΪ512
	char str[INET_ADDRSTRLEN];//��СΪ16���ֽ�
	int i, n;
	//int socket(int domain, int type, int protocol);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);//protocolΪ0��ʾʹ��Ĭ��Э�� ��SOCK_DGRAM�������UDPЭ��
	bzero(&serv_addr, sizeof(serv_addr));//��յ�ַ�ڴ�
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(SERV_PORT);
	//int bind(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
	bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));//�󶨷���˵�ַ�ṹ���׽��ֵ���Ϣ
	printf("Accepting connections ...\n");
	while (1)
	{
		clie_addr_len = sizeof(clie_addr);//�ͻ��˵�ַ�ṹ
		//�滻��TCP����ͨ���е�read �� accept����
		//ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,struct sockaddr *src_addr, socklen_t *addrlen);
		n = recvfrom(sockfd, buf, BUFSIZ, 0, (struct sockaddr *)&clie_addr, &clie_addr_len);
		if (n == -1)
			perror("recvfrom error");
		//���ͻ��������ַת��Ϊ���ص�ַconst char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
		//���ͻ��˶˿�ת��Ϊ���ض˿�uint16_t ntohs(uint16_t netshort);
		printf("received from %s at PORT %d\n", inet_ntop(AF_INET, &clie_addr.sin_addr, str, sizeof(str)),ntohs(clie_addr.sin_port));
		for (i = 0; i < n; i++)
			buf[i] = toupper(buf[i]);//���ռ������ַ���ת��Ϊ��д
		//ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
		n = sendto(sockfd, buf, n, 0, (struct sockaddr *)&clie_addr, sizeof(clie_addr));
		if (n == -1)
			perror("sendto error");
	}
	close(sockfd);
	return 0;
}