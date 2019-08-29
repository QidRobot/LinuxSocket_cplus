#include "requestData.h"
#include "util.h"
#include "epoll.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/time.h>
#include <unordered_map>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <queue>

#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
using namespace cv;

//test
#include <iostream>
using namespace std;
//初始化锁PTHREAD_MUTEX_INITIALIZER则是一个结构常量。
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
//全局锁可以直接利用宏定义初始化 而动态锁需要利用锁初始化函数对锁进行初始化
pthread_mutex_t MimeType::lock = PTHREAD_MUTEX_INITIALIZER;
//设定全局变量
std::unordered_map<std::string, std::string> MimeType::mime;

//8.23 mime类的实现 -- 续8.25 mimeType即客户端请求文件类型
std::string MimeType::getMime(const std::string &suffix)
{
	if (mime.size() == 0)
	{
		pthread_mutex_lock(&lock);
		if (mime.size() == 0)
		{
			//向hashmap中加入 补充后更加完整
			mime[".html"] = "text/html; charset=utf-8";
			mime[".htm"] = "text/html; charset=utf-8";
			mime[".css"] = "text/css";
			mime[".avi"] = "video/x-msvideo";
			mime[".au"] = "audio/basic";
			mime[".bmp"] = "image/bmp";
			mime[".c"] = "text/plain";
			mime[".doc"] = "application/msword";
			mime[".gif"] = "image/gif";
			mime[".gz"] = "application/x-gzip";
			mime[".ico"] = "application/x-ico";
			mime[".jpg"] = "image/jpeg";
			mime[".png"] = "image/png";
			mime[".txt"] = "text/plain";
			mime[".mp3"] = "audio/mp3";
			mime[".mpeg"] = "video/mpeg";
			mime[".mpe"] = "video/mpeg";
			mime[".vrml"] = "model/vrml";
			mime[".wrl"] = "model/vrml";
			mime[".midi"] = "audio/midi";
			mime[".mid"] = "audio/midi";
			mime[".mov"] = "video/quicktime";
			mime[".wav"] = "audio/wav";
			mime[".qt"] = "video/quicktime";
			mime[".wav"] = "audio/wav";
			mime[".avi"] = "video/x-msvideo";
			mime["default"] = "text/html";
		}
		pthread_mutex_unlock(&lock);
	}
	//判断是否找到该类型
	if (mime.find(suffix) == mime.end())
	{
		return mime["default"];
	}
	else
	{
		return mime[suffix];
	}
}
//设置定时器优先级队列
priority_queue<mytimer*, deque<mytimer*>, timerCmp> myTimerQueue;
//初始化请求数据
requestData::requestData():
	now_read_pos(0), state(STATE_PARSE_URI), h_state(h_start), keep_alive(false), againTimes(0), timer(NULL)
{
	cout << "requestData constructed !" << endl;
}
requestData::requestData(int _epollfd, int _fd, std::string _path):
	now_read_pos(0), state(STATE_PARSE_URI), h_state(h_start), keep_alive(false), againTimes(0), timer(NULL),
	path(_path), fd(_fd), epollfd(_epollfd)
{}
requestData::~requestData()
{
	cout << "~requestData()" << endl;
	struct epoll_event ev;
	//超时请求一定是读操作 没有被动写
	ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	ev.data.ptr = (void*)this;
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
	if (timer != NULL)
	{
		timer->clearReq();
		timer = NULL;
	}
	close(fd);
}
//添加定时器
void requestData::addTimer(mytimer *mtimer)
{
	if (timer == NULL)
	{
		timer = mtimer;
	}
}
//得到文件描述符
int requestData::getFd()
{
	return fd;
}
void requestData::setFd(int _fd)
{
	fd = _fd;
}
//将请求内容全部重置
void requestData::reset()
{
	againTimes = 0;
	content.clear();
	file_name.clear();
	path.clear();
	now_read_pos = 0;
	state = STATE_PARSE_URI;
	h_state = h_start;
	headers.clear();
	keep_alive = false;
}
//分离定时器  --  如果定时器 不为空 我们对定时器进行清空操作 设置其参数 deleted 为true
void requestData::seperateTimer()
{
	if (timer)
	{
		timer->clearReq();
		timer = NULL;
	}
}
//处理请求
void requestData::handleRequest()
{
	char buff[MAX_BUFF];
	bool isError = false;
	while (true)
	{
		int read_num = readn(fd, buff, MAX_BUFF);
		if (read_num < 0)
		{
			perror("1");
			isError = true;
			break;
		}
		else if (read_num == 0)
		{
			//有请求出现但是读不到数据 可能是Request Aborted 或者来自网络的数据没有达到等原因
			perror("read_num == 0");
			if (errno == EAGAIN)
			{
				if (againTimes > AGAIN_MAX_TIMES)
				{
					isError = true;
				}
				else
				{
					++againTimes;
				}
			}
			else if (errno != 0)
			{
				isError = true;
			}
			break;
		}
		//表示当前已经读到的内容
		string now_read(buff, buff + read_num);
		content += now_read;
		if (state == STATE_PARSE_URI)
		{
			int flag = this->parse_URL();
			if (flag == PARSE_URI_AGAIN)//const int PARSE_URI_AGAIN = -1;
			{
				break;
			}
			else if (flag == PARSE_URI_ERROR)//const int PARSE_URI_ERROR = -2;
			{
				perror("2");
				isError = true;
				break;
			}
		}
		if (state == STATE_PARSE_HEADERS)
		{
			int flag = this->parse_Headers();
			if (flag == PARSE_URI_AGAIN)
			{
				break;
			}
			else if (flag == PARSE_HEADER_ERROR)
			{
				perror("3");
				isError = true;
				break;
			}
			if (method == METHOD_POST)
			{
				state = STATE_RECV_BODY;
			}
			else 
			{
				state = STATE_ANALYSIS;
			}
		}
		if (state == STATE_RECV_BODY)
		{
			int content_length = -1;
			if (headers.find("Content-length") != headers.end())
			{ 
				//将字符串类型转换成Int类型 将整数型字符串转换成整数型数字
				content_length = stoi(headers["Content-length"]);
			}
			else
			{
				isError = true;
				break;
			}
			if (content.size() < content_length)
			{
				continue;
			}
			state = STATE_ANALYSIS;
		}
		if (state == STATE_ANALYSIS)
		{
			int flag = this->analysisRequest();
			if (flag < 0)
			{
				isError = true;
				break;
			}
			else if (flag == ANALYSIS_SUCCESS)
			{
				state = STATE_FINISH;
				break;
			}
			else
			{
				isError = true;
				break;
			}
		}
	}
	if (isError)
	{
		delete this;
		return;
	}
	//加入epoll继续
	if (state == STATE_FINISH)
	{
		if (keep_alive)
		{
			printf("ok\n");
			this->reset();
		}
		else
		{
			delete this;
			return;
		}
	}
	//先加入时间信息，否则可能会出现刚加进去，下一个in触发来了，然后分离失败了后，又加入队列，最后超时被删，然后
	//正在线程中进行的任务出错，double free 错误
	pthread_mutex_lock(&qlock);
	mytimer *mtimer = new mytimer(this, 500);
	timer = mtimer;
	myTimerQueue.push(mtimer);
	pthread_mutex_unlock(&qlock);

	__uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
	int ret = epoll_mod(epollfd, fd, static_cast<void*>(this), _epo_event);
	if (ret < 0)
	{
		//返回错误处理
		delete this;
		return;
	}
}
//解析地址
int requestData::parse_URL()
{
	string &str = content;
	//读到完整的请求行再开始解析请求
	int pos = str.find('\r', now_read_pos);//从当前已经读到的内容中找到第一个'\r'
	if (pos < 0)
	{
		return PARSE_URI_AGAIN;
	}
	//去掉请求行所占的空间 节省空间 即截取0-pos位置的字符串
	string request_line = str.substr(0, pos);
	if (str.size() > pos + 1)
	{
		//保留剩下的字符串
		str = str.substr(pos + 1);
	}
	else
	{
		str.clear();
	}
	//Method GET http://www.facebook.com/ HTTP/1.1
	pos = request_line.find("GET");
	if (pos < 0)
	{
		pos = request_line.find("POST");
		if (pos < 0)
		{
			return PARSE_URI_ERROR;
		}
		else
		{
			method = METHOD_POST;
		}
	}
	else
	{
		method = METHOD_GET;
	}
	//filename GET http://www.facebook.com/ HTTP/1.1
	pos = request_line.find("/", pos);
	if (pos < 0)
	{
		return PARSE_URI_ERROR;
	}
	else
	{
		//将_pos 和 pos结合找到请求文件名字
		int _pos = request_line.find(' ', pos);
		if (_pos < 0)
		{
			return PARSE_URI_ERROR;
		}
		else
		{
			if (_pos - pos > 1)
			{
				//截取 pos + 1 到 _pos - pos + 1之间的字符串
				file_name = request_line.substr(pos + 1, _pos - pos - 1);
				int __pos = file_name.find('?');
				if (__pos >= 0)
				{
					file_name = file_name.substr(0, __pos);
				}
			}
			else
			{
				file_name = "index.html";
			}
		}
		pos = _pos;
	}
	//HTTP版本号 GET http://www.facebook.com/ HTTP/1.1
	pos = request_line.find("/", pos);
	if (pos < 0)
	{
		return PARSE_URI_ERROR;
	}
	else
	{
		if (request_line.size() - pos <= 3)
		{
			return PARSE_URI_ERROR;
		}
		else
		{
			string ver = request_line.substr(pos + 1, 3);
			if (ver == "1.0")
			{
				HTTPversion = HTTP_10;
			}
			else if (ver == "1.1")
			{
				HTTPversion = HTTP_11;
			}
			else
			{
				return PARSE_URI_ERROR;
			}
		}
	}
	state = STATE_PARSE_HEADERS;
	return PARSE_URI_SUCCESS;
}

//解析请求头
/*
Accept: application/x-ms-application, image/jpeg, application/xaml+xml, [...]
Accept-Language: en-US
User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; WOW64; [...]
Accept-Encoding: gzip, deflate
Connection: Keep-Alive
Cookie: lsd=XW[...]; c_user=21[...]; x-referer=[...]
*/
int requestData::parse_Headers()
{
	string &str = content;
	int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
	int now_read_line_begin = 0;
	bool notFinish = true;
	for (int i = 0; i < str.size() && notFinish; ++i)
	{
		switch (h_state)
		{
			case h_start:
			{
				if (str[i] == '\n' || str[i] == '\r')
				{
					break;
				}
				h_state = h_key;
				key_start = i;
				now_read_line_begin = i;
				break;
			}
			case h_key:
			{
				if (str[i] == ':')
				{
					key_end = i;
					if (key_end - key_start <= 0)
					{
						return PARSE_HEADER_ERROR;
					}
					h_state = h_colon;
				}
				else if (str[i] == '\n' || str[i] == '\r')
				{
					return PARSE_HEADER_ERROR;
				}
				break;
			}
			case h_colon:
			{
				if (str[i] == ' ')
				{
					h_state = h_spaces_after_colon;
				}
				else
				{
					return PARSE_HEADER_ERROR;
				}
				break;
			}
			case h_spaces_after_colon:
			{
				h_state = h_value;
				value_start = i;
				break;
			}
			case h_value:
			{
				if (str[i] == '\r')
				{
					h_state = h_CR;
					value_end = i;
					if (value_end - value_start <= 0)
					{
						return PARSE_HEADER_ERROR;
					}
				}
				else if (i - value_start > 255)
				{
					return PARSE_HEADER_ERROR;
				}
				break;
			}
			case h_CR:
			{
				if (str[i] == '\n')
				{
					h_state = h_LF;
					//将str中的key_start位置到key_end之间进行截取放到key中
					string key(str.begin() + key_start, str.begin() + key_end);
					string value(str.begin() + value_start, str.begin() + value_end);
					headers[key] = value;//用hash表headers来存储
					now_read_line_begin = i;
				}
				else
				{
					return PARSE_HEADER_ERROR;
				}
				break;
			}
			case h_LF:
			{
				if (str[i] == '\r')
				{
					h_state = h_end_CR;
				}
				else
				{
					key_start = i;
					h_state = h_key;
				}
				break;
			}
			case h_end_CR:
			{
				if (str[i] == '\n')
				{
					h_state = h_end_LF;
				}
				else
				{
					return PARSE_HEADER_ERROR;
				}
				break;
			}
			case h_end_LF:
			{
				notFinish = false;
				key_start = i;
				now_read_line_begin = i;
				break;
			}
		}
	}
	if (h_state == h_end_LF)
	{//说明已经正常读完
		str = str.substr(now_read_line_begin);
		return PARSE_HEADER_SUCCESS;
	}
	str = str.substr(now_read_line_begin);
	return PARSE_HEADER_AGAIN;
}
//分析请求数据
int requestData::analysisRequest()
{
	if (method == METHOD_POST)
	{
		//get content
		char header[MAX_BUFF];
		sprintf(header, "HTTP/1.1 %d %s\r\n", 200, "OK");
		if (headers.find("Connection") != headers.end() && headers["Connection"] == "keep-alive")
		{
			keep_alive = true;
			sprintf(header, "%sConnection: keep-alive\r\n", header);
			sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, EPOLL_WAIT_TIME);
		}
		//cout << "content=" << content << endl;
		// test char*
		char *send_content = "I have receiced this.";

		sprintf(header, "%sContent-length: %zu\r\n", header, strlen(send_content));
		sprintf(header, "%s\r\n", header);
		size_t send_len = (size_t)writen(fd, header, strlen(header));
		if (send_len != strlen(header))
		{
			perror("Send header failed");
			return ANALYSIS_ERROR;
		}

		send_len = (size_t)writen(fd, send_content, strlen(send_content));
		if (send_len != strlen(send_content))
		{
			perror("Send content failed");
			return ANALYSIS_ERROR;
		}
		cout << "content size ==" << content.size() << endl;
		vector<char> data(content.begin(), content.end());
		Mat test = imdecode(data, CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
		imwrite("receive.bmp", test);
		return ANALYSIS_SUCCESS;
	}
	else if (method == METHOD_GET)
	{
		char header[MAX_BUFF];
		sprintf(header, "HTTP/1.1 %d %s\r\n", 200, "OK");
		if (headers.find("Connection") != headers.end() && headers["Connection"] == "keep-alive")
		{
			keep_alive = true;
			sprintf(header, "%sConnection: keep-alive\r\n", header);
			sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, EPOLL_WAIT_TIME);
		}
		int dot_pos = file_name.find('.');
		const char* filetype;
		if (dot_pos < 0)
			filetype = MimeType::getMime("default").c_str();
		else
			filetype = MimeType::getMime(file_name.substr(dot_pos)).c_str();
		struct stat sbuf;
		/*
			函数说明:    通过文件名filename获取文件信息，并保存在buf所指的结构体stat中
			返回值:      执行成功则返回0，失败返回-1，错误代码存于errno
		 struct stat
		{	dev_t     st_dev;   文件使用的设备号
			ino_t     st_ino;    索引节点号
			mode_t    st_mode;    文件对应的模式，文件，目录等
			nlink_t   st_nlink;   文件的硬连接数
			uid_t     st_uid;      所有者用户识别号
			gid_t     st_gid;     组识别号
			dev_t     st_rdev;    设备文件的设备号
			off_t     st_size;    以字节为单位的文件容量
			blksize_t st_blksize; 包含该文件的磁盘块的大小
			blkcnt_t  st_blocks;   该文件所占的磁盘块
			time_t    st_atime;   最后一次访问该文件的时间
			time_t    st_mtime;    最后一次修改该文件的时间
			time_t    st_ctime;    最后一次改变该文件状态的时间
		};*/
		if (stat(file_name.c_str(), &sbuf) < 0)
		{
			handleError(fd, 404, "Not Found!");
			return ANALYSIS_ERROR;
		}

		sprintf(header, "%sContent-type: %s\r\n", header, filetype);
		// 通过Content-length返回文件大小
		sprintf(header, "%sContent-length: %ld\r\n", header, sbuf.st_size);

		sprintf(header, "%s\r\n", header);
		size_t send_len = (size_t)writen(fd, header, strlen(header));
		if (send_len != strlen(header))
		{
			perror("Send header failed");
			return ANALYSIS_ERROR;
		}
		int src_fd = open(file_name.c_str(), O_RDONLY, 0);
		char *src_addr = static_cast<char*>(mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0));
		close(src_fd);

		// 发送文件并校验完整性
		send_len = writen(fd, src_addr, sbuf.st_size);
		if (send_len != sbuf.st_size)
		{
			perror("Send file failed");
			return ANALYSIS_ERROR;
		}
		/*函数说明 munmap()用来取消参数start所指的映射内存起始地址，参数length则是欲取消的内存大小。
		当进程结束或利用exec相关函数来执行其他程序时，映射内存会自动解除，但关闭对应的文件描述符时不会解除映射。*/
		munmap(src_addr, sbuf.st_size);
		return ANALYSIS_SUCCESS;
	}
	else
	{
		return ANALYSIS_ERROR;
	}
}

void requestData::handleError(int fd, int err_num, std::string short_msg)
{
	short_msg = " " + short_msg;
	char send_buff[MAX_BUFF];
	string body_buff, header_buff;

	

	body_buff += "<html><title>TKeed Error</title>";
	body_buff += "<body bgcolor=\"ffffff\">";
	body_buff += "<h4 align=\"center\"><font color=\"#FF0000\">" + to_string(err_num) + short_msg + "</font></h4>";
	body_buff += "<hr><h4 align=\"center\"><em><font color=\"#0000FF\">jacob's Web Server</font></em></h4>\n</body></html>";

	header_buff += "HTTP/1.1 " + to_string(err_num) + short_msg + "\r\n";
	header_buff += "Content-type: text/html\r\n";
	header_buff += "Connection: close\r\n";
	header_buff += "Content-length: " + to_string(body_buff.size()) + "\r\n";
	header_buff += "\r\n";

	sprintf(send_buff, "%s", header_buff.c_str());
	writen(fd, send_buff, strlen(send_buff));//先发请求头
	sprintf(send_buff, "%s", body_buff.c_str());
	writen(fd, send_buff, strlen(send_buff));//再发出body信息数据
}

mytimer::mytimer(requestData *_request_data, int timeout):deleted(false), request_data(_request_data)
{
	/*struct timeval
	{
		__time_t tv_sec;        //Seconds. 秒
		__suseconds_t tv_usec;  // Microseconds. 微秒 
	};*/
	struct timeval now;
	// int gettimeofday(struct timeval *tv, struct timezone *tz);成功返回 0  失败返回 -1
	gettimeofday(&now, NULL);
	// 以毫秒计算
	expired_time = ((now.tv_sec * 1000) + (now.tv_usec / 1000)) + timeout;
}
mytimer::~mytimer()
{
	cout << "~mytimer()" << endl;//析构函数
	if (request_data != NULL)
	{
		cout << "request_data = " << request_data << endl;
		delete request_data;
		request_data = NULL;//避免野指针
	}
}
//更新定时器的过期时间
void mytimer::update(int timeout)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	expired_time = ((now.tv_sec * 1000) + (now.tv_usec / 1000)) + timeout;
}
//判断定时器是否有效 统一使用毫秒计数
bool mytimer::isvalid()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	size_t temp = ((now.tv_sec * 1000) + (now.tv_usec / 1000));
	if (temp < expired_time)
	{
		return true;
	}
	else
	{
		this->setDeleted();
		return false;
	}

}
void mytimer::clearReq()
{
	request_data = NULL;
	this->setDeleted();
}
void mytimer::setDeleted()
{
	deleted = true;
}
bool mytimer::isDeleted() const
{
	return deleted;
}
size_t mytimer::getExpTime() const
{
	return expired_time;
}
bool timerCmp::operator()(const mytimer *a, const mytimer *b) const
{
	return a->getExpTime() > b->getExpTime();
}