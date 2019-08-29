#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>//���ļ�Ŀ¼
#include <fcntl.h>
#include <errno.h>

#define MAXSIZE 4096

//���ʹ���ҳ��
void send_error(int cfd, int status, char *title, char *text)
{
	char buf[4096] = { 0 };
	sprintf(buf, "%s %d %s\r\n", "HTTP/1.1", status, title);
	sprintf(buf + strlen(buf), "Content-Type:%s\r\n", "text/html");
	sprintf(buf + strlen(buf), "Content-Length:%d\r\n", -1);
	sprintf(buf + strlen(buf), "Connection: close\r\n");
	send(cfd, buf, strlen(buf), 0);
	send(cfd, "\r\n", 2, 0);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "<html><head><title>%d %s</title></head>\n", status, title);
	sprintf(buf + strlen(buf), "<body bgcolor = \"#cc99cc\"><h4 align=\"center\">%d %s</h4>\n", status, title);
	sprintf(buf + strlen(buf), "%s\n", text);
	sprintf(buf + strlen(buf), "<hr>\n</body>\n</html>\n");
	send(cfd, buf, strlen(buf), 0);
	return;
}

//��ȡһ�� \r\n��β������
int get_line(int cfd, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;
	while ((i < size - 1) && (c != '\n'))
	{
		n = recv(cfd, &c, 1, 0);
		if (n > 0)
		{
			if (c == '\r')
			{
				n = recv(cfd, &c, 1, MSG_PEEK);
				if ((n > 0) && (c == '\n'))
				{
					recv(cfd, &c, 1, 0);
				}
				else
				{
					c = '\n';
				}
			}
			buf[i] = c;
			i++;
		}
		else
		{
			c = '\n';
		}
	}
	buf[i] = '\0';
	if (-1 == n)
	{
		i = n;
	}
	return i;
}

//��ʼ�������׽���
int init_listen_fd(int port, int epfd)
{
	//�����������׽��� lfd
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1)
	{
		perror("socket error");
		exit(1);
	}
	//������������ַ�ṹ IP + port
	struct sockaddr_in srv_addr;

	bzero(&srv_addr, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(port);
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//�˿ڸ���
	int opt = 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	//��lfd �󶨵�ַ�ṹ
	int ret = bind(lfd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
	if (ret == -1)
	{
		perror("bind error");
		exit(1);
	}
	//���ü�������
	ret = listen(lfd, 128);
	if (ret == -1)
	{
		perror("listen error");
		exit(1);
	}
	//lfd ��ӵ� epoll����
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = lfd;

	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
	if (ret == -1)
	{
		perror("epoll_ctl add lfd error");
		exit(1);
	}
	return lfd;
}

//�������������
void do_accept(int lfd, int epfd)
{
	struct sockaddr_in clt_addr;
	socklen_t clt_addr_len = sizeof(clt_addr);

	int cfd = accept(lfd, (struct sockaddr*)&clt_addr, &clt_addr_len);
	if (cfd == -1)
	{
		perror("accept error");
		exit(1);
	}

	//��ӡ�ͻ���IP + port
	char client_ip[64] = { 0 };
	printf("New Client IP: %s, Port: %d, cfd = %d\n",
		inet_ntop(AF_INET, &clt_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)),
		ntohs(clt_addr.sin_port), cfd
	);

	//����cfd ������
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(cfd, F_SETFL, flag);

	//���½ڵ�cfd�ҵ�epoll��������
	struct epoll_event ev;
	ev.data.fd = cfd;

	//���ط�����ģʽ ��ETģʽ
	ev.events = EPOLLIN | EPOLLET;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
	if (ret == -1)
	{
		perror("epoll_ctl add cfd error");
		exit(1);
	}
}

//�Ͽ�����
void disconnect(int cfd, int epfd)
{
	int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
	if (ret != 0)
	{
		perror("epoll_ctl error");
		exit(1);
	}
	close(cfd);
}

/*����Э��ͷ
HTTP/1.1 200 OK
server:xhttpd
Date: Fri, 18 Jul 2014 14:34:26 GMT
Content-Type: text/plain; charset=iso-8859-1
Content-Length: 32
Content-Language: zh-CN
Last-Modified: Fri, 18 Jul 2014 08:36:36 GMT
Connection: close
*/

//�ͻ��˵�fd, ����ţ� ���������� �ط��ļ����ͣ� �ļ�����
void send_respond(int cfd, int no, char *disp,const char *type, int len)
{
	char buf[1024] = { 0 };
	sprintf(buf, "HTTP/1.1 %d %s\r\n", no, disp);
	//send(cfd, buf, strlen(buf), 0);//����һ��
	sprintf(buf + strlen(buf), "Content-Type: %s\r\n", type);
	sprintf(buf + strlen(buf), "Content-Length: %d\r\n", len);
	//sprintf(buf, "\r\n");
	send(cfd, buf, strlen(buf), 0);
	send(cfd, "\r\n", 2, 0);
}

//ͨ���ļ�����ȡ�ļ�������
char *get_file_type(const char* name)
{
	char* dot;
	//�����������'.'�ַ�������������򷵻�NULL
	dot = strrchr(name, '.');
	if (dot == NULL)
	{
		return "text/plain; charset=utf-8";
	}
	else if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
	{
		return "text/html; charset=utf-8";
	}
	else if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jepg") == 0)
	{
		return "image/jpeg";
	}
	else if (strcmp(dot, ".gif") == 0)
	{
		return "image/gif";
	}
	else if (strcmp(dot, ".png") == 0)
	{
		return "image/png";
	}
	else if (strcmp(dot, ".css") == 0)
	{
		return "text/css";
	}
	else if (strcmp(dot, ".au") == 0)
	{
		return "audio/basic";
	}
	else if (strcmp(dot, ".wav") == 0)
	{
		return "audio/wav";
	}
	else if (strcmp(dot, ".avi") == 0)
	{
		return "video/x-msvideo";
	}
	else if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
	{
		return "video/quicktime";
	}
	else if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
	{
		return "video/mpeg";
	}
	else if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
	{
		return "model/vrml";
	}
	else if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
	{
		return "audio/midi";
	}
	else if (strcmp(dot, ".mp3") == 0)
	{
		return "audio/mpeg";
	}
	else
	{
		return "text/plain; charset=iso-8859-1";
	}
}

//16������ת��Ϊ10����
int hexit(char c)
{
	if (c >= '0' && c <= '9')
	{
		return c - '0';
	}
	if (c >= 'a' && c <= 'f')
	{
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F')
	{
		return c - 'A' + 10;
	}
	return 0;
}

//����
void decode_str(char *to, char *from)
{
	for (;*from != '\0'; ++to, ++from)
	{
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			*to = hexit(from[1]) * 16 + hexit(from[2]);
			from += 2;
		}
		else
		{
			*to = *from;
		}
	}
	*to = '\0';
}

//����
void encode_str(char *to, int tosize, const char* from)
{
	int tolen;
	for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from)
	{
		if (isalnum(*from) || strchr("/_.-~", *from) != (char*)0)
		{
			*to = *from;
			++to;
			++tolen;
		}
		else
		{
			//%%��ʾת�����˼
			sprintf(to, "%%%02x", (int) *from & 0xff);
			to += 3;
			tolen += 3;
		}
	}
	*to = '\0';
}

//����http�����ж��ļ��Ƿ���� Ȼ��ط�
void http_request(int cfd, const char *request)
{
	//���http������
	char method[12], path[1024], protocol[12];
	sscanf(request, "%[^ ] %[^ ] %[^ ]", method, path, protocol);
	printf("method = %s, path = %s, protocol = %s", method, path, protocol);
	
	//ת��	������ʶ����������� -������
	//����
	decode_str(path, path);

	char* file = path + 1;//ȥ��path�е�/ ��ȡ�����ļ���

	//���û��ָ�����ʵ���Դ Ĭ����ʾ��ԴĿ¼�е�����
	if (strcmp(path, "/") == 0)
	{
		//file��ֵ ��ԴĿ¼�ĵ�ǰλ��
		file = "./";
	}

	struct stat sbuf;

	//�ж��ļ��Ƿ����
	int ret = stat(file, &sbuf);
	if (ret == -1)
	{
		//��ʾ���ļ�ʧ��
		//�ط������ 404 ����ҳ��
		send_error(cfd, 404, "Not Found", "NO such file or direntry");
		return;
	}
	//�ж����ļ�����Ŀ¼
	if (S_ISDIR(sbuf.st_mode))//Ŀ¼
	{
		//����ͷ��Ϣ
		char* file_type;
		file_type = get_file_type(".html");
		send_respond(cfd, 200, "OK", file_type, -1);
		//����Ŀ¼��Ϣ
		send_dir(cfd, file);
	}
	else if (S_ISREG(sbuf.st_mode))//��һ����ͨ�ļ�
	{
		//printf("-----------------It's a file\n");
		//�ط�httpЭ��Ӧ��
		//send_respond(int cfd, int no, char *disp, char *type, int len)
		
		//��ȡ�ļ�����
		char* file_type;
		file_type = get_file_type(file);
		send_respond(cfd, 200, "OK", file_type, sbuf.st_size);

		//�ط����ͻ�����������
		send_file(cfd, file);
	}
}

//����Ŀ¼����
void send_dir(int cfd, const char *dirname)
{
	int i, ret;
	//ƴһ��htmlҳ��<table></table>
	char buf[4094] = { 0 };
	sprintf(buf, "<html><head><title>Ŀ¼����%s</title></head>", dirname);
	sprintf(buf + strlen(buf), "<body><h1>��ǰĿ¼��%s</h1><table>", dirname);
	char enstr[1024] = { 0 };
	char path[1024] = { 0 };
	//Ŀ¼�����ָ��
	struct dirent** ptr;
	int num = scandir(dirname, &ptr, NULL, alphasort);
	//����
	for (i = 0; i < num; ++i)
	{
		char* name = ptr[i]->d_name;
		//ƴ���ļ�������·��
		sprintf(path, "%s/%s", dirname, name);
		printf("path = %s ==================\n", path);
		struct stat st;
		stat(path, &st);
		
		//��������  %E5 %A7
		encode_str(enstr, sizeof(enstr), name);

		//������ļ�
		if (S_ISREG(st.st_mode))
		{
			sprintf(buf + strlen(buf), "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>", enstr, name, (long)st.st_size);
		}
		else if (S_ISDIR(st.st_mode))
		{
			sprintf(buf + strlen(buf), "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>", enstr, name, (long)st.st_size);
		}
		ret = send(cfd, buf, strlen(buf), 0);
		if (ret == -1)
		{
			if (errno == EAGAIN)
			{
				perror("send error");
				continue;
			}
			else if (errno == EINTR)
			{
				perror("send error");
				continue;
			}
			else
			{
				perror("send error");
				exit(1);
			}
		}
		memset(buf, 0, sizeof(buf));
		//�ַ���ƴ��
	}
	sprintf(buf + strlen(buf), "</table></body></html>");
	send(cfd, buf, strlen(buf), 0);
	printf("dir message send OK!!!!\n");
#if 0
	//��Ŀ¼
	DIR* dir = opendir(dirname);
	if (dir == NULL)
	{
		perror("opendir error");
		exit(1);
	}
	//��Ŀ¼
	struct dirent* ptr = NULL;
	while ((ptr = readaddir(dir)) != NULL)
	{
		char * name = ptr->d_name;
	}
	closedir(dir)��
#endif
}

//���ͷ����������ļ��������
void send_file(int cfd, const char *filename)
{
	//���ļ�
	int fd = open(filename, O_RDONLY);
	if (fd == -1)
	{
		//404����ҳ��
		send_error(cfd, 404, "Not Found", "NO such file or direntry");
		//perror("open error");
		exit(1);
	}

	int n = 0, ret = 0;
	char buf[4096] = { 0 };
	//�����ķ����������ļ� --cfd �ܷ��ʿͻ��˵�socket

	while ((n = read(fd, buf, sizeof(buf))) > 0)
	{
		ret = send(cfd, buf, n, 0);
		if (ret == -1)
		{
			//printf("errno = %d\n", errno);
			if (errno == EAGAIN)
			{
				perror("send error");
				continue;
			}
			else if (errno == EINTR)
			{
				perror("send error");
				continue;
			}
			else
			{
				perror("send error");
				exit(1);
			}
		}
	}
	if (n == -1)
	{
		perror("read file error");
		exit(1);
	}
	close(fd);
}

//���ļ�����
void do_read(int cfd, int epfd)
{
	//read cfd С--�� write��
	//��ȡһ��httpЭ�� ��� ��ȡget�ļ��� Э���
	char line[1024] = { 0 };
	int len = get_line(cfd, line, sizeof(line));//�� http����Э�� ���� GET/hello.c HTTP/1.1
	if (len == 0)
	{
		printf("����������⵽�ͻ��˹ر�.....\n");
		disconnect(cfd, epfd);
	}
	else
	{
		//char method[16], path[256], protocol[16];
		//sscanf(line, "%[^ ] %[^ ] %[^ ]", method, path, protocol);
		//printf("method = %s, path = %s, protocol = %s", method, path, protocol);
		printf("=============����ͷ================\n");
		printf("���������ݣ�%s", line);
		while (1)
		{
			char buf[1024] = { 0 };
			len = get_line(cfd, buf, sizeof(buf));
			if (buf[0] == '\n')
			{
				break;
			}
			//printf("-------len = %d------%s", len, buf);
			//sleep(1);
			else if (len == -1)
			{
				break;
			}
		}
	}
	//���Դ�Сд���ַ����Ƚ�
	if (strncasecmp("get", line, 3) == 0)
	{
		http_request(cfd, line);
		//�ر��׽��֣�cfd��epoll��del
		disconnect(cfd, epfd);
	}
}

//epoll��Ӧ��ģ��
void epoll_run(int port)
{
	int i = 0;
	struct epoll_event all_events[MAXSIZE];

	//����һ��epoll��������
	int epfd = epoll_create(MAXSIZE);
	if (epfd == -1)
	{
		perror("epoll_create error");
		exit(1);
	}

	//����lfd,�������������
	int lfd = init_listen_fd(port, epfd);

	while (1)
	{
		//�����ڵ��Ӧ�¼�
		int ret = epoll_wait(epfd, all_events, MAXSIZE, -1);//�����ķ�ʽ ������������ѯ
		if (ret == -1)
		{
			perror("epoll_wait error");
			exit(1);
		}
		for (i = 0; i < ret; i++)
		{
			//ֻ������¼� �����¼�Ĭ�ϲ�����
			struct epoll_event *pev = &all_events[i];

			//���Ƕ��¼�
			if (!(pev->events & EPOLLIN))
			{
				continue;
			}
			if (pev->data.fd == lfd)	//������������
			{
				do_accept(lfd, epfd);
			}
			else//������
			{
				do_read(pev->data.fd, epfd);
			}
		}
	}
}

int main(int argc, char *argv[])
{
	//�����в�����ȡ�˿� �� server�ṩ��Ŀ¼
	if (argc < 3)
	{
		printf("./server port path\n");
	}
	//��ȡ�û�����Ķ˿�
	int port = atoi(argv[1]);
	//�Ľ����̹���Ŀ¼
	int ret = chdir(argv[2]);
	if (ret != 0)
	{
		perror("chdir error");
		exit(1);
	}
	//���� epoll����
	epoll_run(port);
	return 0;
}