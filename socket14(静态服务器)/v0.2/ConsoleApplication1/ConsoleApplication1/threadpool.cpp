#include "threadpool.h"
threadpool_t *threadpool_create(int thread_count, int queue_size, int flags)
{
	threadpool_t *pool;
	int i;
	do{
		//MAX_THREADS = 1024;	��ʵ�ʴ���thread_count��СΪ4;		MAX_QUEUE = 65535;
		if (thread_count <= 0 || thread_count > MAX_THREADS || queue_size <= 0 || queue_size > MAX_QUEUE)
		{
			return NULL;
		}
		//�̳߳ش�������
		if ((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL)
		{
			break;
		}
		/*��ʼ���̳߳�*/
		pool->thread_count = 0;
		//�̳߳ض��д�С
		pool->queue_size = queue_size;
		//�ö��е�ͷ����β����ָ��0���λ��
		pool->head = pool->tail = pool->count = 0;
		pool->shutdown = pool->started = 0;
		//Ϊ�̺߳�������з���ռ�
		pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
		//��ʼ���̳߳�������� �ö��д�С���ݴ���Ĵ�С���з��� ʵ�ʴ����queue_size��СΪ65535 ��Ϊ usign int ���͵Ĵ�С
		pool->queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size);
		//��ʼ�� mutex ���������� pthread_cond_t notify
		/*
		��̬��ʼ�������ڸ�pool->lockû�ж���Ϊȫ�ֱ��� �� û��ʹ��static�ؼ��� ��˲��� ��̬��ʼ����ʽ:	pthread_mutex_init(&mutex, NULL)
		��̬��ʼ��������������Ǿ�̬�ֲ��� ���ֱ��ʹ�ú��ʼ�� pthead_mutex_t muetx = PTHREAD_MUTEX_INITIALIZER;
		*/
		if ((pthread_mutex_init(&(pool->lock), NULL) != 0) || (pthread_cond_init(&(pool->notify), NULL) != 0) 
			|| (pool->threads == NULL) || (pool->queue == NULL))
		{
			break;
		}
		//�����߳� �߳����������ڴ����thread_count =  4 �ķ�Χ��
		for (i = 0; i < thread_count; i ++)
		{
			//�̳߳��̴߳����ص�������threadpool_thread�̲߳�������
			/*int pthread_create(pthread_t *thread, const pthread_attr_t *attr,void *(*start_routine) (void *), void *arg);*/
			if (pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void*)pool) != 0)
			{
				//threadpool_destory�ͷ��̳߳� �����һ���߳�û��û�д����ɹ���������̳߳�ȫ�����ٺ��ͷ�
				threadpool_destroy(pool, 0);
				return NULL;
			}
			//�̴߳����ɹ� ��ִ��
			pool->thread_count++;//�̳߳��� �����߳��߳���++
			pool->started++;//ִ���߳���++ ��Ծ�߳�����1
		}
		return pool;
	} while (false);
	//�ܹ�ִ����һ��˵��ǰ���Ȼ�����˴��� ���Ҫ���Ѿ��������̳߳ؽ����ͷ�
	if (pool != NULL)
	{
		threadpool_free(pool);
	}
	return NULL;
}

//���̳߳��е����������������� �������е�tailָ������ָ��β�� ������������δ�����������count + 1
int threadpool_add(threadpool_t *pool, void(*function)(void *), void *argument, int flags)
{
	//�����̳߳�
	int err = 0;
	int next;
	if (pool == NULL || function == NULL)
	{
		return THREADPOOL_INVALID;//const int THREADPOOL_INVALID = -1;
	}
	if (pthread_mutex_lock(&(pool->lock)) != 0)
	{
		return THREADPOOL_LOCK_FAILURE;
	}
	next = (pool->tail + 1) % pool->queue_size;
	do 
	{
		//�ж��̳߳ض����Ƿ�����
		if (pool->count == pool->queue_size)
		{
			err = THREADPOOL_QUEUE_FULL;
			break;
		}
		//�ж��̳߳��Ƿ�ر�
		if (pool->shutdown)
		{
			err = THREADPOOL_SHUTDOWN;
			break;
		}
		//���̳߳��м������ ���ҽ����Ӧ�ص����� �Ͳ���һ�����д���
		pool->queue[pool->tail].function = function;
		pool->queue[pool->tail].argument = argument;
		pool->tail = next;
		pool->count += 1;
		//��������һ�����������������ϵ��߳�
		if (pthread_cond_signal(&(pool->notify)) != 0)
		{
			err = THREADPOOL_LOCK_FAILURE;
			break;
		}
	} while (false);
	//�ͷ���
	if (pthread_mutex_unlock(&pool->lock) != 0)
	{
		err = THREADPOOL_LOCK_FAILURE;
	}
	return err;
}

int threadpool_destroy(threadpool_t *pool, int flags)
{
	printf("Thread pool destory !\n");
	int i, err = 0;
	if (pool == NULL)
	{
		return THREADPOOL_INVALID;//THREADPOOL_INVALID = -1
	}
	//�ɹ�����0 ���򷵻�һ��������
	if (pthread_mutex_lock(&(pool->lock)) != 0)
	{
		return THREADPOOL_LOCK_FAILURE;
	}
	do 
	{
		//�Ѿ��رյ�
		if (pool->shutdown)
		{
			err = THREADPOOL_SHUTDOWN;
			break;
		}
		//����flag�趨�رշ�ʽΪ���Ż��Ƿ����� THREADPOOL_GRACEFUL = 1
		//immediate_shutdown = 1, graceful_shutdown = 2 &��ʾ������ͬʱΪ1�����Ϊ1
		pool->shutdown = (flags & THREADPOOL_GRACEFUL) ? graceful_shutdown : immediate_shutdown;
		//���������߳�
		if ((pthread_cond_broadcast(&(pool->notify)) != 0) || (pthread_mutex_unlock(&(pool->lock)) != 0))
		{
			err = THREADPOOL_LOCK_FAILURE;
			break;
		}
		//�������ù����߳�
		for (i = 0; i < pool->thread_count; ++i)
		{
			//extern int pthread_join __P ((pthread_t __th, void **__thread_return));
			//��һ������Ϊ���ȴ����̱߳�ʶ�����ڶ�������Ϊһ���û������ָ�룬�����������洢���ȴ��̵߳ķ���ֵ��
			if (pthread_join(pool->threads[i], NULL) != 0)
			{
				err = THREADPOOL_THREAD_FAILURE;//const int THREADPOOL_THREAD_FAILURE = -5;
			}
		}
	} while (false);
	//�����в�����ȷ���ǿ�ʼ�ͷ��̳߳��ڴ�
	if (!err)//0�ͷ�0 ֻ��!0 = 1(true)
	{
		threadpool_free(pool);
	}
	return err;
}

int threadpool_free(threadpool_t *pool)
{
	if (pool == NULL || pool->started > 0)
	{
		return -1;
	}
	//�ж��Ƿ��Ѿ�������
	if (pool->threads)
	{
		free(pool->threads);
		free(pool->queue);
		//�����ڴ����̳߳ص�ʱ�����ڲ������������� ��������ڲ����̳߳ص�ʱ��Ӧ���ȶ��̳߳ؽ�����������
		pthread_mutex_lock(&(pool->lock));
		pthread_mutex_destroy(&(pool->lock));//�ͷ���
		pthread_cond_destroy(&(pool->notify));//�ͷ��߳�����
	}
	free(pool);
	return 0;
}

//�̲߳������� �̳߳��в����߳�ʱ�����õĻص�����
static void *threadpool_thread(void *threadpool)
{
	/*������е���Ӻ�ȡ���� ����Ҫ���� ������������� ��Խ�˶���߳�*/
	threadpool_t *pool = (threadpool_t *)threadpool;
	threadpool_task_t task;
	for (;;)
	{
		//ʹ��lock����Ϊ���������ȴ� --------- �����������Ϊ��mutex--����-1��
		pthread_mutex_lock(&(pool->lock));
		//�ȴ����������������ٵĻ��ѡ�����pthread_cond_wait()����ʱ������ӵ����
		while ((pool->count == 0) && (!pool->shutdown))
		{
			/*�����ȴ�һ����������
				int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex);
			�������ã�
				1.	�����ȴ���������cond����1������
				2.	�ͷ������յĻ��������������������൱��pthread_mutex_unlock(&mutex);
			1.2.����Ϊһ��ԭ�Ӳ�����
				3.	�������ѣ�pthread_cond_wait��������ʱ��������������������ȡ������pthread_mutex_lock(&mutex);*/
			pthread_cond_wait(&(pool->notify), &(pool->lock)); //�������ؼ�������ѭ��
		}
		//immediate_shutdown = 1  graceful_shutdown = 2
		if ((pool->shutdown == immediate_shutdown) ||
			((pool->shutdown == graceful_shutdown) &&
			(pool->count == 0)))
		{
			break;
		}
		/*ִ������ �ӵ�ǰheadλ��ȡ������ʼִ��*/ 
		task.function = pool->queue[pool->head].function;//���е�ͷ ��������
		task.argument = pool->queue[pool->head].argument;
		pool->head = (pool->head + 1) % pool->queue_size;//�����е�ͷ ����ָ����һ���ڵ� 
		//�߳��еȴ�ִ��������-1
		pool->count -= 1;

		//�����������Ϊ��mutex ++����+1��
		pthread_mutex_unlock(&(pool->lock));
		//�߳̿�ʼ����
		(*(task.function))(task.argument);
	}
	//��Ծ�߳��������� 1
	--pool->started;
	pthread_mutex_unlock(&(pool->lock));//�߳̽�������
	pthread_exit(NULL);//�߳��˳����� �������� �̲߳�һ���˳� ��˱���ʹ���߳��˳����� ���ҶԵ��ø��̵߳������̲߳�Ӱ��
	return(NULL);
}