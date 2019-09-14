#include "timer.h"
#include "epoll.h"
#include <unordered_map>
#include <string>
#include <sys/time.h>
#include <unistd.h>
#include <deque>
#include <queue>
#include <iostream>
using namespace std;

//mytimer::mytimer(requestData *_request_data, int timeout):deleted(false), request_data(_request_data)
TimerNode::TimerNode(SP_ReqData _request_data, int timeout) : deleted(false), request_data(_request_data)
{
	cout << "TimeNode()" << endl;
	/*struct timeval
	{
	__time_t tv_sec;        //Seconds. ��
	__suseconds_t tv_usec;  // Microseconds. ΢��
	};*/
	struct timeval now;
	// int gettimeofday(struct timeval *tv, struct timezone *tz);�ɹ����� 0  ʧ�ܷ��� -1
	gettimeofday(&now, NULL);
	// �Ժ������
	expired_time = ((now.tv_sec * 1000) + (now.tv_usec / 1000)) + timeout;
}

TimerNode::~TimerNode()
{
	cout << "~TimeNode()" << endl;//��������
	if (request_data)
	{
		//int Epoll::epoll_del(int fd, __uint32_t events)
		//�����Ѿ���epoll���ж����˸ú����������������� __uint32_t events = (EPOLLIN | EPOLLET | EPOLLONESHOT) Ĭ�ϲ���
		Epoll::epoll_del(request_data->getFd());
		//cout << "request_data = " << request_data << endl;
		//delete request_data;
		//request_data = NULL;//����Ұָ��
	}
}
//���¶�ʱ���Ĺ���ʱ��
void TimerNode::update(int timeout)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	//�Ժ������
	expired_time = ((now.tv_sec * 1000) + (now.tv_usec / 1000)) + timeout;
}
//�ж϶�ʱ���Ƿ���Ч ͳһʹ�ú������
bool TimerNode::isvalid()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	//size_t��ʵ����һ��unsigned int����
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
void TimerNode::clearReq()
{
	//request_data = NULL;
	request_data.reset();
	this->setDeleted();
}
void TimerNode::setDeleted()
{
	deleted = true;
}
bool TimerNode::isDeleted() const
{
	return deleted;
}
size_t TimerNode::getExpTime() const
{
	return expired_time;
}

//������TimerManager��ʵ��
TimerManager::TimerManager()
{

}
TimerManager::~TimerManager()
{

}
//����ʱ�������ַ�ʽ һ���Ǽ���ʱ��ڵ� ����һ���Ǽ������� �����ù���ʱ��
void TimerManager::addTimer(SP_ReqData request_data, int timeout)
{
	//�½�һ��ʱ��ڵ� ��ʱ��ڵ���뵽ʱ��ڵ������ ��ʱ��ڵ����request����
	SP_TimerNode new_node(new TimerNode(request_data, timeout));
	{
		MutexLockGuard locker(lock);
		TimerNodeQueue.push(new_node);
	}
	//����������� �� ʱ��ڵ������й���
	request_data->linkTimer(new_node);
}

void TimerManager::addTimer(SP_TimerNode timer_node)
{}

/*�����߼�
��Ϊ(1)���ȶ��в�֧���������
(2)��ʹ֧�� ���ɾ��ĳ�ڵ���ƻ��˶ѵĽṹ ��Ҫ���¸��¶ѽṹ
���Զ��ڱ���Ϊdeleted��ʱ��ڵ� ���ӳٵ���(1)��ʱ����(2)ǰ��Ľڵ㶼��ɾ�� ���Ż�ɾ��
һ���㱻��Ϊdeleted �������TIMER_TIMER_OUTʱ���ɾ��
�������������ô�
(1)��һ���ô��ǲ���Ҫ�������ȼ����� ʡʱ
(2)�ڶ����ô��Ǹ���ʱʱ��һ������ʱ�� �����趨�ĳ�ʱʱ����ɾ�������ޣ�������һ����ʱʱ�������ɾ����
��������������ڳ�ʱ�����һ���������ֳ�����һ�ξͲ�����������requestData�ڵ��ˣ��������Լ����ظ�����
ǰ���requestData ������һ��delete��һ��new��ʱ��
*/

//��������¼� ���ڶ�ʱ�����ȼ�������ΪС���ѣ�����ֻ��Ҫ�ж�����Ƿ��ȥ���ɣ�������û�й��� ��ô˵�����ж�ʱ�����������
void TimerManager::handle_expired_event()
{
	MutexLockGuard locker(lock);
	//pthread_mutex_lock(&qlock);
	while (!TimerNodeQueue.empty())
	{
		SP_TimerNode ptimer_now = TimerNodeQueue.top();
		if (ptimer_now->isDeleted())
		{
			TimerNodeQueue.pop();
			//delete ptimer_now;��������ָ��Ͳ���Ҫ�ٽ��� delete�ͷŶ���Դ��
		}
		else if (ptimer_now->isvalid() == false)
		{
			TimerNodeQueue.pop();
			//delete ptimer_now;
		}
		else
		{
			//���û�й��ڵ�ʱ�䶨ʱ�� �� ����ѭ��
			break;
		}
	}
	//pthread_mutex_unlock(&qlock);
}