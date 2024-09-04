#pragma once
#include <list>
#include <sys/epoll.h>
#include "MChannel.h"
#include "Mhandler.h"
#include "threadpool.h"
class MChannel;
using namespace std;

class task : public Request
{
	struct epoll_event event;

public:
	task(struct epoll_event _event) : event(_event) {};
	void process() override;
};
class MyinsKernel
{
	int epoll_fd = -1;
	static MyinsKernel *kernel;
	list<MChannel *> channelList;
	threadPool<Request> *pool;
	MyinsKernel();

public:
	void run();
	void SetKernelStop()
	{
		pool->SetPoolStop();
		stop = true;
	}
	bool AddTask(Request *);
	bool AddChannel(MChannel *channel);
	void DelChannel(MChannel *channel);
	bool ModChannel_AddOut(MChannel *channel);
	void ModChannel_DelOut(MChannel *channel);
	void KernelSendOut(MChannel *channel, string Msg);
	// ������Ϣ�ҵ��ܵ�
	MChannel *GetChannelByInfo(string info);
	static MyinsKernel *GetInstance()
	{
		return kernel;
	}
	virtual ~MyinsKernel();

private:
	bool stop = false;
};
