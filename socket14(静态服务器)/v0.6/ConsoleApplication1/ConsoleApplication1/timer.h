#pragma once
#include "requestData.h"
#include "nocopyable.h"
#include "MutexLock.h"
#include <unistd.h>
#include <memory>
#include <deque>
#include <queue>

//�������������
class RequestData;
//��ʱ���ṹ��
struct TimerNode 
{
	typedef std::shared_ptr<RequestData> SP_ReqData;
private:
	bool deleted;
	//size_t size type ��һ�ֱ����Ĵ�С ����ת��Ϊint��
	size_t expired_time;//��ʱ���Ĺ���ʱ�� ��ֵΪ����ʱ�� + timeout��ֵ
						//requestData *request_data;//����������ָ������ڸýṹ���е������� �ж� �Ƿ�����������-ʹ�ö�ʱ��
						/*ʹ������ָ��*/
	//std::shared_ptr<requestData> request_data;
	SP_ReqData request_data;

public:
	/*��mytimer�м�������ָ��*/
	//mytimer(requestData *_request_data, int timeout);
	TimerNode(SP_ReqData _request_data, int timeout);
	~TimerNode();
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
	bool operator()(std::shared_ptr<TimerNode> &a, std::shared_ptr<TimerNode> &b) const
	{
		return a->getExpTime() > b->getExpTime();
	}
	//bool operator()(const mytimer *a, const mytimer *b) const;
};

class TimerManager
{
	typedef std::shared_ptr<RequestData> SP_ReqData;
	typedef std::shared_ptr<TimerNode> SP_TimerNode;
private:
	std::priority_queue<SP_TimerNode, std::deque<SP_TimerNode>, timerCmp> TimerNodeQueue;
	MutexLock lock;

public:
	TimerManager();
	~TimerManager();
	//����ʱ�������ַ�ʽ һ���Ǽ���ʱ��ڵ� ����һ���Ǽ������� �����ù���ʱ��
	void addTimer(SP_ReqData request_data, int timeout);
	void addTimer(SP_TimerNode timer_node);
	void handle_expired_event();
};
