/*.hpp �ļ���ʾ .cpp��.h�ļ�������ͬһ���ļ��� ��ʾΪ�������ʵ�ֶ�����ͬһ���ļ�	��������ʱ�Ϳ��Բ����ٽ��б��� ֱ������.obj
�ļ����е��ü���*/
#pragma once
#include "nocopyable.hpp"
#include <pthread.h>
#include <cstdio>

//nocopyableΪ�ǿ��� �Ǹ�ֵ�� ��˼̳и������ӵ����������
class MutexLock : noncopyable
{
public:
	MutexLock()
	{
		pthread_mutex_init(&mutex, NULL);
	}
	~MutexLock()
	{
		//����������ʱ����м��� �Ƿ�ֹ��ʱ����ʹ��
		pthread_mutex_lock(&mutex);
		pthread_mutex_destroy(&mutex);
	}
	void lock()
	{
		pthread_mutex_lock(&mutex);
	}
	void unlock()
	{
		pthread_mutex_unlock(&mutex);
	}
private:
	pthread_mutex_t mutex;
};

/*��ϸ���� RALL������ ��ʵ�������Ϊ�����ڵ��ú���ʱ ��ʵ����ջ �ں����е��������� ������Ҳ����ջ��
����ں������ý���ʱ ǣ�浽��ջ ��ջ�ͻ��ͷ���Դ ������ǲ��õ���������߳���ʹ����Դ
ʹ��RALL�����ܹ��ɹ��Ľ��������Դ���а�
*/

/*RAII��������ʹ��һ���������乹��ʱ��ȡ��Ӧ����Դ���ڶ����������ڿ��ƶ���Դ�ķ��ʣ�
ʹ֮ʼ�ձ�����Ч������ڶ���������ʱ���ͷŹ���ʱ��ȡ����Դ��*///������Դ
class MutexLockGuard : noncopyable
{
public:
	//�����ڶ����ʼ��ʱ��ʽ���øù��캯�� ֻ��ͨ����ʾ���� ����֧����ʽ����ת��
	/*�ڵ��øù��캯��ʱ �����������A a(19)�ͻᱨ�� ���ʹ�� a = 19���൱��ʹ����ʽ����ת��*/
	explicit MutexLockGuard(MutexLock &_mutex):mutex(_mutex)
	{		mutex.lock();
	}
	~MutexLockGuard()
	{
		mutex.unlock();
	}
private:
	MutexLock &mutex;
};