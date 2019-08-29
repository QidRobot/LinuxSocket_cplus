#include "wrap.h"
#define SERV_PORT 9527
//void sys_err(const char *str)
//{
//	perror(str);
//	exit(1);
//}
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
	lfd = Socket(AF_INET, SOCK_STREAM, 0);
	//sockaddr_in  ��ַ�ṹ ��һ�ֽṹ������
	ret = Bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	Listen(lfd, 128);

	clit_addr_len = sizeof(clit_addr);
	//���ܿͻ��˵�
	cfd = Accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);

	/*inet_ntop ������ip��ַת���ɱ���ip��ַ  ��������ip��ַ�ͱ���ip��ַ�����ǲ�ͬ����������*/
	printf("client ip: %s port: %d\n",
		inet_ntop(AF_INET, &clit_addr.sin_addr.s_addr, client_IP, sizeof(client_IP)),
		/*������˿� ת����  ��������˿�*/
		ntohs(clit_addr.sin_port)
	);
	while (1)
	{
		ret = Read(cfd, buf, sizeof(buf));
		//д�����ػ�����
		Write(STDOUT_FILENO, buf, ret);
		for (i = 0; i < ret; i++)
		{
			buf[i] = toupper(buf[i]);
		}
		//ͨ���ļ�������д���ͻ���
		Write(cfd, buf, ret);
	}	
	close(lfd);
	close(cfd);
	return 0;
}