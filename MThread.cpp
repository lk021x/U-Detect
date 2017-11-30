#include "MThread.h"

MThread::MThread()
{
	pid = 0;
	prio=SCHED_FIFO;   //数值越大优先级越高 1~99
	threadStatus = THREAD_STATUS_NEW;
	pthread_attr_init(&attr);
}

MThread::~MThread()
{
	pthread_attr_destroy(&attr);
	if(pid != 0) pthread_cancel(pid);
}

void MThread::start()
{
	int ret;

	ret = pthread_create(&pid, &attr, thread_proxy_func, this);
	if(ret != 0) printf("thread %lu creating err!\n", pid);
}

//某个线程本身的ID
pthread_t MThread::getThreadID()
{
	return pid;
}

//获取线程状态
int MThread::getState()
{
	return threadStatus;
}

void* MThread::thread_proxy_func(void* pVoid)
{
	MThread* p = (MThread*) pVoid;
	p->threadStatus = THREAD_STATUS_RUNNING;
	p->pid = pthread_self();  //启动时获取当前线程ID就是线程本身的ID, 多个线程同时存在时，指的是当前活动的线程
	p->run();  // 用户实现
	p->threadStatus = THREAD_STATUS_EXIT;
	p->pid = 0;
	pthread_exit((void*)"thread is exit!");
	return NULL;
}

//等待线程结束
void MThread::wait()
{
	int ret;
	void *thread_result;

	if (pid == 0) return;
	ret = pthread_join(pid, &thread_result);
	if(ret != 0) printf("thread %lu joining  err!\n", pid);
	printf("%s\n", (char*)thread_result);
}

//设置线程的绑定状态
void MThread::setThreadScope(bool isSystem)
{
	if (isSystem){
		pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM); //绑定 有助提高线程响应速度
	}else{
		pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS); //非绑定
	}
}

//获取线程绑定状态
bool MThread::getThreadScope()
{
	int scopeType = 0;
	pthread_attr_getscope(&attr, &scopeType);
	return scopeType == PTHREAD_SCOPE_SYSTEM;
}

//设置线程优先级 SCHED_OTHER , 只支持sched.h 中定义的优先级别
void MThread::setThreadPriority(int priority)
{
	pthread_attr_getschedpolicy(&attr, &prio);
	prio = priority;
	pthread_attr_setschedpolicy(&attr, prio);
}

//获取线程优先级
int MThread::getThreadPriority()
{
	pthread_attr_getschedpolicy(&attr, &prio);
	return prio;
}

//设置线程分离属性
//注意 线程如果是分离的话创建后务必不要调用wait()函数，反之必须调用
void MThread::setThreadDetachState(bool isDetach)
{
	if(isDetach)
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);  //分离
	else
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);  //绑定
}
