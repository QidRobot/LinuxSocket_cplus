//�̳߳�
#pragma once
#ifndef THREADPOOL
#define THREADPOOL
#include "requestData.h"
#include <pthread.h>
#include <functional>
#include <memory>
#include <vector>

const int THREADPOOL_INVALID = -1;
const int THREADPOOL_LOCK_FAILURE = -2;
const int THREADPOOL_QUEUE_FULL = -3;
const int THREADPOOL_SHUTDOWN = -4;
const int THREADPOOL_THREAD_FAILURE = -5;
const int THREADPOOL_GRACEFUL = 1;

const int MAX_THREADS = 1024;
const int MAX_QUEUE = 65535;

//�رշ�ʽ Ϊֱ�ӹر� �������Źر�
typedef enum
{
	immediate_shutdown = 1,
	graceful_shutdown = 2
}ShutDownOption;

typedef struct ThreadPoolTask
{
	std::function<void(std::shared_ptr<void>)> fun;
	std::shared_ptr<void> args;
	//��������ָ��ȫ���޸�Ϊ����ָ��
	//void(*function)(void*);
	//void *argument;
};
/**
*  @struct threadpool
*  @brief The threadpool struct
*
*  @var notify       Condition variable to notify worker threads.
*  @var threads      Array containing worker threads ID.
*  @var thread_count Number of threads
*  @var queue        Array containing the task queue.
*  @var queue_size   Size of the task queue.
*  @var head         Index of the first element.
*  @var tail         Index of the next element.
*  @var count        Number of pending tasks
*  @var shutdown     Flag indicating if the pool is shutting down
*  @var started      Number of started threads
*/
void myHandler(std::shared_ptr<void> req);
class ThreadPool
{
private:
	//pthread_mutex_t ���ͣ��䱾����һ���ṹ�塣Ϊ����⣬Ӧ��ʱ�ɺ�����ʵ��ϸ�ڣ��򵥵�������������
	static pthread_mutex_t lock;
	//���ڶ����������� pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	//�߳���������������
	static pthread_cond_t notify;
	//�߳�ָ�� ���� 
	//pthread_t *threads;�޸�Ϊ����
	static std::vector<pthread_t> threads;
	//�̳߳�������� ��������а����ص�����������
	static std::vector<ThreadPoolTask> queue;
	static int thread_count;//�̳߳��߳���
	static int queue_size;//���д�С
	static int head;//���е�ͷ
	//tailָ��β�ڵ����һ�ڵ�
	static int tail;//���е�β��
	static int count;//��ʱ���е����� �߳�������е���������
	static int shutdown;
	static int started;

public:
	static int threadpool_create(int _thread_count, int _queue_size);
	static int threadpool_add(std::shared_ptr<void> args, std::function<void(std::shared_ptr<void>)> fun = myHandler);
	//Ĭ��ʹ�����ŵķ�ʽ�����̳߳�
	static int threadpool_destroy(ShutDownOption shutdown_option = graceful_shutdown);
	static int threadpool_free();
	static void *threadpool_thread(void *args);
};
#endif
