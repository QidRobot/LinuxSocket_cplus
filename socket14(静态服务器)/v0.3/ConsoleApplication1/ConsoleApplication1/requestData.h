#pragma once
#ifndef REQUESTDATA
#define REQUESTDATA
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
	static pthread_mutex_t lock;
	static std::unordered_map<std::string, std::string> mime;//hashmap��
	MimeType();
	MimeType(const MimeType &m);//�������캯�� ��������ʹ��ǳ������ֵʧЧ
public:
	static std::string getMime(const std::string &suffix);
};
//����ͷ״̬
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
struct mytimer;
struct requestData;
//��������
//enable_shared_from_this ����һ���Ի�������Ϊģ������ʵ�εĻ���ģ�� �̳��� thisָ��ͱ����shared_ptr����
struct requestData : public std::enable_shared_from_this<requestData>
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
	std::weak_ptr<mytimer> timer;
private:
	//������ַ
	int parse_URL();
	//��������ͷ
	int parse_Headers();
	//������������
	int analysisRequest();
public:
	requestData();
	requestData(int _epollfd, int _fd, std::string _path);
	~requestData();
	//void addTimer(mytimer *mtimer);
	void addTimer(std::shared_ptr<mytimer> mtimer);
	void reset();
	void seperateTimer();
	int getFd();
	void setFd(int _fd);
	void handleRequest();
	void handleError(int fd, int err_num, std::string short_msg);
};

//��ʱ���ṹ��
struct mytimer
{
	bool deleted;
	//size_t size type ��һ�ֱ����Ĵ�С ����ת��Ϊint��
	size_t expired_time;//��ʱ���Ĺ���ʱ�� ��ֵΪ����ʱ�� + timeout��ֵ
	//requestData *request_data;//����������ָ������ڸýṹ���е������� �ж� �Ƿ�����������-ʹ�ö�ʱ��
	/*ʹ������ָ��*/
	std::shared_ptr<requestData> request_data;

	/*��mytimer�м�������ָ��*/
	//mytimer(requestData *_request_data, int timeout);
	mytimer(std::shared_ptr<requestData> _request_data, int timeout);
	~mytimer();
	void update(int timeout);
	bool isvalid();
	void clearReq();
	void setDeleted();
	bool isDeleted() const;
	size_t getExpTime() const;
};
//���ö�ʱ���Ƚ��� �������Ų����� ���a�Ķ�ʱ�����ʱ�����b�򷵻�ture ���򷵻�false
struct timerCmp
{
	/*��ָ��ͳһ��Ϊ����ָ��*/
	bool operator()(std::shared_ptr<mytimer> &a, std::shared_ptr<mytimer> &b) const;
	//bool operator()(const mytimer *a, const mytimer *b) const;
};

/*��ϸ���� RALL������ ��ʵ�������Ϊ�����ڵ��ú���ʱ ��ʵ����ջ �ں����е��������� ������Ҳ����ջ��
����ں������ý���ʱ ǣ�浽��ջ ��ջ�ͻ��ͷ���Դ ������ǲ��õ���������߳���ʹ����Դ 
ʹ��RALL�����ܹ��ɹ��Ľ��������Դ���а�
*/

/*RAII��������ʹ��һ���������乹��ʱ��ȡ��Ӧ����Դ���ڶ����������ڿ��ƶ���Դ�ķ��ʣ�
ʹ֮ʼ�ձ�����Ч������ڶ���������ʱ���ͷŹ���ʱ��ȡ����Դ��*/
class MutexLockGuard//������Դ
{
public:
	//�����ڶ����ʼ��ʱ��ʽ���øù��캯�� ֻ��ͨ����ʾ���� ����֧����ʽ����ת��
	explicit MutexLockGuard();
	~MutexLockGuard();

private:
	static pthread_mutex_t lock;

private:
	MutexLockGuard(const MutexLockGuard&);
	MutexLockGuard& operator=(const MutexLockGuard&);
};
#endif