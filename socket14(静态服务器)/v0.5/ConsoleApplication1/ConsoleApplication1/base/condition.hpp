#pragma once
#include "nocopyable.hpp"
#include "mutexLock.hpp"
#include <pthread.h>
class Condition : noncopyable
{
public:
	explicit Condition(MutexLock &_mutex) : mutex(_mutex)
	{
		pthread_cond_init(cond, NULL);
	}
	~Condition()
	{
		pthread_cond_destroy(cond);
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
private:
	MutexLock &mutex;
	pthread_cond_t cond;
};