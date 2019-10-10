#include "Thread.h"
#include "CurrentThread.hpp"

#include <memory>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <linux/unistd.h>
#include <stdint.h>
#include <assert.h>

#include <iostream>
using namespace std;

namespace CurrentThread
{
	__thread int	t_cachedTid;
	__thread char	t_tidString[32];
	__thread int	t_tidStringLength;
	__thread const	char* t_threadName;
	   
	void cacheTid();
	inline int tid()
	{
		if (__builtin_expect(t_cachedTid == 0, 0))
		{
			cacheTid();
		}
		return t_cachedTid;
	}
	inline const char* tidString() //for logging
	{
		return t_tidString;
	}
	inline int tidStringLength() //for logging
	{
		return t_tidStringLength;
	}
	inline const char* name()
	{
		return t_threadName;
	}
}

pid_t gettid()
{
	return static_cast<pid_t>(::syscall(SYS_gettid));
}

//Ϊ�����߳��б���name, tid��Щ����
struct ThreadData
{
	typedef Thread::ThreadFunc ThreadFunc;
	ThreadFunc func_;
	string name_;
	pid_t* tid_;
	CountDownLatch* latch_;

	ThreadData(const ThreadFunc& func, 
		const string& name, pid_t* tid, 
		CountDownLatch* latch) 
	:func_(func), 
	name_(name), 
	tid_(tid), 
	latch_(latch)
	{}

	void runInThread()
	{
		*tid_ = CurrentThread::tid();
		tid_ = NULL;
		latch_->countDown();
		latch_ = NULL;
		CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
		prctl(PR_SET_NAME, CurrentThread::t_threadName);
		func_();
		CurrentThread::t_threadName = "finished";
	}
};

void* startThread(void* obj)
{
	ThreadData* data = static_cast<ThreadData*>(obj);
	data->runInThread();
	delete data;
	return NULL;
}

void CurrentThread::cacheTid()
{
	if (t_cachedTid == 0)
	{
		t_cachedTid = gettid();
		t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d", t_cachedTid);
	}
}

Thread::Thread(const ThreadFunc& func, const string& n)
	: started_(false), joined_(false), pthreadId_(0), tid_(0), func_(func), name_(n), latch_(1)
{
	setDefaulName();
}

Thread::~Thread()
{
	if (started_ && !joined_)
	{
		pthread_detach(pthreadId_);
	}
}

void Thread::setDefaulName()
{
	if (name_.empty())
	{
		char buf[32];
		snprintf(buf, sizeof buf, "Thread");
		name_ = buf;
	}
}

void Thread::start()
{
	//assert���������ּ�����ʽ expression �������ֵΪ�٣���Ϊ0������ô������stderr��ӡһ��������Ϣ Ȼ��ͨ������ abort ����ֹ��������
	assert(!started_);//���started_true���߳��˳� �����߳��Ѿ�ִ��
	started_ = true;
	ThreadData* data = new ThreadData(func_, name_, &tid_, &latch_);
	//pthread_create(&pid, NULL, producer, NULL);
	if (pthread_create(&pthreadId_, NULL, &startThread, data))
	{
		started_ = false;
		delete data;
	}
	else
	{
		latch_.wait();
		assert(tid_ > 0);
	}
}

int Thread::join()
{
	assert(started_);
	assert(!joined_);
	joined_ = true;
	//pthread_join(cid, NULL);
	return pthread_join(pthreadId_, NULL);
}
