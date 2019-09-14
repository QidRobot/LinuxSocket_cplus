#pragma once
#ifndef REQUESTDATA
#define REQUESTDATA
#include "timer.h"
#include <string>
#include <unordered_map> 
#include <memory>
/*����ͷ�ļ�(C++11)��#include <unordered_map> -- hashmap��
���壺unordered_map<int,int>��unordered_map<string, double>��
���룺���罫(��ABC�� -> 5.45) ����unordered_map<string, double> hash�У�hash[��ABC��]=5.45*/
const int STATE_PARSE_URI = 1;//������ַ
const int STATE_PARSE_HEADERS = 2;//����ͷ
const int STATE_RECV_BODY = 3;//�յ���������
const int STATE_ANALYSIS = 4;
const int STATE_FINISH = 5;
//���������ֵ
const int MAX_BUFF = 4096;
//��������ֵ��Ƕ��������ݣ�������Request Aborted�������������������û�дﵽ��ԭ�� �������������Գ���һ���Ĵ���������
//���Դ������Ϊ200
const int AGAIN_MAX_TIMES = 200;
//������ַ
const int PARSE_URI_AGAIN = -1;
const int PARSE_URI_ERROR = -2;
const int PARSE_URI_SUCCESS = 0;
//����ͷ
const int PARSE_HEADER_AGAIN = -1;
const int PARSE_HEADER_ERROR = -2;
const int PARSE_HEADER_SUCCESS = 0;
//��������
const int ANALYSIS_ERROR = -2;
const int ANALYSIS_SUCCESS = 0;
//HTTP�������� POST��������GET����
const int METHOD_POST = 1;
const int METHOD_GET = 2;
const int HTTP_10 = 1;
const int HTTP_11 = 2;

const int EPOLL_WAIT_TIME = 500;

//�����ж������ļ���׺��
class MimeType
{
private:
	//static pthread_mutex_t lock;
	static void init();
	static std::unordered_map<std::string, std::string> mime;//hashmap��
	MimeType();
	MimeType(const MimeType &m);//�������캯�� ��������ʹ��ǳ������ֵʧЧ

public:
	static std::string getMime(const std::string &suffix);

private:
	//ʹ���߳�ֻ����һ�η�ʽ ���������� ʹ��������ס��Դ - �������һ�������Ż�
	static pthread_once_t once_control;
	//��̬����һ��Ҫ��ʼ���� ���������ʵ���м�ʵ���˸ñ���
};

//����ͷ״̬
/*ö�������е�ֵ ������ֵ��δ��ֵʱ��һ��Ԫ�ص�ֵΪ0 �����ֵ���μ�1
����и�ֵ�Ļ��� ����ı�����Ӹø�ֵλ�ÿ�ʼ�𽥼�1
*/
enum HeaderState
{
	h_start = 0,
	h_key,
	h_colon,
	h_spaces_after_colon,
	h_value,
	h_CR,
	h_LF,
	h_end_CR,
	h_end_LF
};

struct TimerNode;

//��������
//enable_shared_from_this ����һ���Ի�������Ϊģ������ʵ�εĻ���ģ�� �̳��� thisָ��ͱ����shared_ptr����
class RequestData : public std::enable_shared_from_this<RequestData>
{
private:
	int againTimes;//��� �۲�������� �����������ֵAGAIN_MAX_TIMESΪ200
	std::string path;
	int fd;
	int epollfd;
	std::string content;//content���������������
	int method;
	int HTTPversion;//httpЭ��汾
	std::string file_name;
	int now_read_pos;
	int state;
	int h_state;
	bool isfinish;
	bool keep_alive;
	std::unordered_map<std::string, std::string> headers;
	//���������ڲ� ���� ��ʱ�� ʹ��weak_ptr��Ȼ�����Կ����������� ���ǿ���֪�������Ƿ񻹻���
	//mytimer *timer;
	std::weak_ptr<TimerNode> timer;

private:
	//������ַ
	int parse_URL();
	//��������ͷ
	int parse_Headers();
	//������������
	int analysisRequest();

public:
	RequestData();
	RequestData(int _epollfd, int _fd, std::string _path);
	~RequestData();
	void linkTimer(std::shared_ptr<TimerNode> mtimer);
	//void addTimer(mytimer *mtimer);
	//void addTimer(std::shared_ptr<TimerNode> mtimer);
	void reset();
	void seperateTimer();
	int getFd();
	void setFd(int _fd);
	void handleRequest();
	void handleError(int fd, int err_num, std::string short_msg);
};

#endif