#pragma once
#include "nocopyable.h"
#include "MutexLock.h"
#include <pthread.h>
#include <errno.h>
#include <cstdint>
#include <time.h>

class Condition : noncopyable
{
public:
	explicit Condition(MutexLock &_mutex) : mutex(_mutex)
	{
		pthread_cond_init(&cond, NULL);
	}
	~Condition()
	{
		pthread_cond_destroy(&cond);
	}
	void wait()
	{
	/*int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex);
		�������ã�
		1.	�����ȴ���������cond����1������
		2.	�ͷ������յĻ��������������������൱��pthread_mutex_unlock(&mutex);
		 1.2.����Ϊһ��ԭ�Ӳ�����
		3.	�������ѣ�pthread_cond_wait��������ʱ��������������������ȡ������pthread_mutex_lock(&mutex);
	*/
		pthread_cond_wait(&cond, mutex.get());
	}
	void notify()
	{
		//�������������ķ�ʽ���л��� �����������һ��
		pthread_cond_signal(&cond);
	}
	void notifyAll()
	{
		//���ù㲥�ķ�ʽ���л��� ȫ���������������������е��߳�
		pthread_cond_broadcast(&cond);
	}
	bool waitForSeconds(int seconds)
	{
		struct timespec abstime;
		//clock_gettime( ) �ṩ�����뼶�ľ�ȷ��  
		/*
		int clock_gettime(clockid_t clk_id, struct timespec *tp);����˵����
		
		clockid_t clk_id ����ָ����ʱʱ�ӵ����ͣ�������4�֣�
		CLOCK_REALTIME:ϵͳʵʱʱ��,��ϵͳʵʱʱ��ı���ı�,����UTC1970-1-1 0:0:0��ʼ��ʱ,
		�м�ʱ�����ϵͳʱ�䱻�û��ó�����,���Ӧ��ʱ����Ӧ�ı�
		CLOCK_MONOTONIC:��ϵͳ������һ����ʼ��ʱ,����ϵͳʱ�䱻�û��ı��Ӱ��
		CLOCK_PROCESS_CPUTIME_ID:�����̵���ǰ����ϵͳCPU���ѵ�ʱ��
		CLOCK_THREAD_CPUTIME_ID:���̵߳���ǰ����ϵͳCPU���ѵ�ʱ��
		struct timespec *tp�����洢��ǰ��ʱ�䣬��ṹ���£�
		struct timespec
		{
			time_t tv_sec; //seconds
			long tv_nsec; //nanoseconds
		};
		����ֵ��0�ɹ��� - 1ʧ��
		*/
		//����Ӧ�û�ȡ��ǰʱ�� Ȼ�󽫵�ǰʱ����ϵȴ�ʱ�� һ�𴫸�pthread_cond_timedwait
		clock_gettime(CLOCK_REALTIME, &abstime);
		abstime.tv_sec += static_cast<time_t>(seconds);
		//��ʱ�ȴ���������
		/*int pthread_cond_timedwait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex, const struct timespec *restrict abstime);*/
		return ETIMEDOUT == pthread_cond_timedwait(&cond, mutex.get(), &abstime);
	}

private:
	MutexLock &mutex;
	pthread_cond_t cond;
};