#include "threadpool.h"
//�����е����Գ�ʼ��
//��ʼ�� mutex ���������� pthread_cond_t notify
/*
��̬��ʼ�������ڸ�pool->lockû�ж���Ϊȫ�ֱ��� �� û��ʹ��static�ؼ��� ��˲��� ��̬��ʼ����ʽ:	pthread_mutex_init(&mutex, NULL)
��̬��ʼ��������������Ǿ�̬�ֲ��� ���ֱ��ʹ�ú��ʼ�� pthead_mutex_t muetx = PTHREAD_MUTEX_INITIALIZER;
*/
pthread_mutex_t ThreadPool::lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ThreadPool::notify = PTHREAD_COND_INITIALIZER;
std::vector<pthread_t> ThreadPool::threads;
//�̳߳��е��������
std::vector<ThreadPoolTask> ThreadPool::queue;
int ThreadPool::thread_count = 0;//��ǰ�̳߳��е��߳���
int ThreadPool::queue_size = 0;
int ThreadPool::head = 0;
int ThreadPool::tail = 0;
int ThreadPool::count = 0;//��ʾ��ǰ�����е�������
int ThreadPool::shutdown = 0;
int ThreadPool::started = 0;

int ThreadPool::threadpool_create(int _thread_count, int _queue_size)
{
	bool err = false;
	do{
		//MAX_THREADS = 1024;	��ʵ�ʴ���thread_count��СΪ4;		MAX_QUEUE = 65535;
		if (_thread_count <= 0 || _thread_count > MAX_THREADS || _queue_size <= 0 || _queue_size > MAX_QUEUE)
		{
			_thread_count = 4;
			_queue_size = 1024;
		}
		/*��ʼ���̳߳� ��ʾ��ǰ�̳߳��е��߳���*/
		thread_count = 0;
		//�̳߳ض��д�С
		queue_size = _queue_size;
		//�ö��е�ͷ����β����ָ��0���λ��
		head = tail = count = 0;
		shutdown = started = 0;

		//��������Ϊvector ��˿���ʹ��resize����
		threads.resize(_thread_count);
		queue.resize(_queue_size);
		
		//�����߳� �߳����������ڴ����thread_count =  4 �ķ�Χ��
		for (int i = 0; i < _thread_count; ++i)
		{
			//�̳߳��̴߳����ص�������threadpool_thread�̲߳�������
			/*int pthread_create(pthread_t *thread, const pthread_attr_t *attr,void *(*start_routine) (void *), void *arg);*/
			if (pthread_create(&threads[i], NULL, threadpool_thread, (void*)(0)) != 0)
			{
				//threadpool_destory�ͷ��̳߳� �����һ���߳�û��û�д����ɹ���������̳߳�ȫ�����ٺ��ͷ�
				//threadpool_destroy(pool, 0);
				return -1;
			}
			//�̴߳����ɹ� ��ִ��
			thread_count++;//�̳߳��� �����߳��߳���++
			started++;//ִ���߳���++ ��Ծ�߳�����1
		}
	} while (false);
	//�ܹ�ִ����һ��˵��ǰ���Ȼ�����˴��� ���Ҫ���Ѿ��������̳߳ؽ����ͷ�
	if (err)
	{
		return -1;
	}
	return 0;
}

void myHandler(std::shared_ptr<void> req)
{
	//ʹ������ָ��ʱ �漰������ת�� ����ʹ��static_pointer_cast �� dynamic_pointer_cast ����dynamic_pointer_cast�����˵���Ǻܰ�ȫ
	std::shared_ptr<RequestData> request = std::static_pointer_cast<RequestData>(req);
	request->handleRequest();
}

//���̳߳��е����������������� �������е�tailָ������ָ��β�� ������������δ�����������count + 1
int ThreadPool::threadpool_add(std::shared_ptr<void> args, std::function<void(std::shared_ptr<void>)> fun)
{
	//�����̳߳�
	int next, err = 0;
	if (pthread_mutex_lock(&lock) != 0)
	{
		return THREADPOOL_LOCK_FAILURE;
	}
	do 
	{
		next = (tail + 1) % queue_size;
		//�ж��̳߳ض����Ƿ�����
		if (count == queue_size)
		{
			err = THREADPOOL_QUEUE_FULL;
			break;
		}
		//�ж��̳߳��Ƿ�ر�
		if (shutdown)
		{
			err = THREADPOOL_SHUTDOWN;
			break;
		}
		//�����ڶ�����ע��ص������Ͳ��� 
		queue[tail].fun = fun;
		queue[tail].args = args;
		tail = next;
		++count;

		//��������һ�����������������ϵ��߳�
		if (pthread_cond_signal(&notify) != 0)
		{
			err = THREADPOOL_LOCK_FAILURE;
			break;
		}
	} while (false);
	//�ͷ���
	if (pthread_mutex_unlock(&lock) != 0)
	{
		err = THREADPOOL_LOCK_FAILURE;
	}
	return err;
}

int ThreadPool::threadpool_destroy(ShutDownOption shutdown_option)
{
	printf("Thread pool destory !\n");
	int i, err = 0;

	//�ɹ�����0 ���򷵻�һ��������
	if (pthread_mutex_lock(&lock) != 0)
	{
		return THREADPOOL_LOCK_FAILURE;
	}
	do 
	{
		//�Ѿ��رյ�
		if (shutdown)
		{
			err = THREADPOOL_SHUTDOWN;
			break;
		}
		//����flag�趨�رշ�ʽΪ���Ż��Ƿ����� THREADPOOL_GRACEFUL = 1
		//immediate_shutdown = 1, graceful_shutdown = 2 &��ʾ������ͬʱΪ1�����Ϊ1
		//pool->shutdown = (flags & THREADPOOL_GRACEFUL) ? graceful_shutdown : immediate_shutdown;
		shutdown = shutdown_option;
		//���������߳�
		if ((pthread_cond_broadcast(&notify) != 0) || (pthread_mutex_unlock(&lock) != 0))
		{
			err = THREADPOOL_LOCK_FAILURE;
			break;
		}
		//�������ù����߳�
		for (i = 0; i < thread_count; ++i)
		{
			//extern int pthread_join __P ((pthread_t __th, void **__thread_return));
			//��һ������Ϊ���ȴ����̱߳�ʶ�����ڶ�������Ϊһ���û������ָ�룬�����������洢���ȴ��̵߳ķ���ֵ��
			if (pthread_join(threads[i], NULL) != 0)
			{
				err = THREADPOOL_THREAD_FAILURE;//const int THREADPOOL_THREAD_FAILURE = -5;
			}
		}
	} while (false);
	//�����в�����ȷ���ǿ�ʼ�ͷ��̳߳��ڴ�
	if (!err)//0�ͷ�0 ֻ��!0 = 1(true)
	{
		threadpool_free();
	}
	return err;
}

int ThreadPool::threadpool_free()
{
	if (started > 0)
	{
		//��ʾ���л��ŵ��߳�
		return -1;
	}
	pthread_mutex_lock(&lock);
	pthread_mutex_destroy(&lock);
	pthread_cond_destroy(&notify);
	return 0;
	//if (pool == NULL || pool->started > 0)
	//{
	//	return -1;
	//}
	////�ж��Ƿ��Ѿ�������
	//if (pool->threads)
	//{
	//	free(pool->threads);
	//	free(pool->queue);
	//	//�����ڴ����̳߳ص�ʱ�����ڲ������������� ��������ڲ����̳߳ص�ʱ��Ӧ���ȶ��̳߳ؽ�����������
	//	pthread_mutex_lock(&(pool->lock));
	//	pthread_mutex_destroy(&(pool->lock));//�ͷ���
	//	pthread_cond_destroy(&(pool->notify));//�ͷ��߳�����
	//}
	//free(pool);
	//return 0;
}

//�̲߳������� �̳߳��в����߳�ʱ�����õĻص�����
void *ThreadPool::threadpool_thread(void *args)
{
	while (true)
	{
		ThreadPoolTask task;
		pthread_mutex_lock(&lock);
		while ((count == 0) && (!shutdown))
		{
			/*�����ȴ�һ����������
			int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex);
			�������ã�
			1.	�����ȴ���������cond����1������
			2.	�ͷ������յĻ��������������������൱��pthread_mutex_unlock(&mutex);
			1.2.����Ϊһ��ԭ�Ӳ�����
			3.	�������ѣ�pthread_cond_wait��������ʱ��������������������ȡ������pthread_mutex_lock(&mutex);*/
			pthread_cond_wait(&notify, &lock); //�������ؼ�������ѭ��
		}

		//immediate_shutdown = 1  graceful_shutdown = 2
		if ((shutdown == immediate_shutdown) ||
			((shutdown == graceful_shutdown) && (count == 0)))
		{
			break;
		}

		/*ִ������ �ӵ�ǰheadλ��ȡ������ʼִ��*/
		task.fun = queue[head].fun;//���е�ͷ ��������
		task.args = queue[head].args;
		queue[head].fun = NULL;
		queue[head].args.reset();
		head = (head + 1) % queue_size;//�����е�ͷ ����ָ����һ���ڵ� //�߳��еȴ�ִ��������-1
		--count;/*��������һ������󽫶����е���������һ*/
		//�����������Ϊ��mutex ++����+1��
		pthread_mutex_unlock(&lock);
		//�߳̿�ʼ���� -- ���仰˵�̱߳�ɿ��� ����ʼִ��
		(task.fun)(task.args);
	}
	--started;//�˳�һ���߳�
	pthread_mutex_unlock(&lock);//�߳̽������� �����ѭ������break������ ����û���ͷ��� ����������ͷ���Ҳ��һ�ַǳ���ȫ������
	printf("This threadpool thread finishs!\n");
	pthread_exit(NULL);//�߳��˳����� �������� �̲߳�һ���˳� ��˱���ʹ���߳��˳����� ���ҶԵ��ø��̵߳������̲߳�Ӱ��
	return(NULL);
}