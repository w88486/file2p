#include "MyinsKernel.h"
#include <unistd.h>
MyinsKernel *MyinsKernel::kernel = new MyinsKernel();
MyinsKernel::MyinsKernel()
{
	pool = new threadPool<Request>(20);
	int fd = epoll_create(1);
	if (fd > 0)
	{
		epoll_fd = fd;
	}
}
void MyinsKernel::run()
{
	// ѭ������ִ��
	while (!stop)
	{
		struct epoll_event events[MAX_THREADS];
		// ������
		int ret_size = epoll_wait(epoll_fd, events, 200, -1);
		if (0 == ret_size)
		{
			if (EINTR == errno)
			{
				continue;
			}
		}
		// ������ͨ�������񽻸��̳߳�ִ�в���
		for (int i = 0; i < ret_size; ++i)
		{
			auto channel = static_cast<MChannel *>(events[i].data.ptr);
			if (channel->ChannelNeedClose())
			{
				kernel->DelChannel(channel);
			}
			else
			{
				task *t = new task(events[i]);
				pool->appand(t);
			}
		}
	}
}

bool MyinsKernel::AddTask(Request *t)
{
	pool->appand(t);
}

bool MyinsKernel::AddChannel(MChannel *channel)
{
	// ��ʼ���ܵ�
	if (true == channel->init())
	{
		struct epoll_event event;
		event.events = EPOLLIN | EPOLLET;
		event.data.ptr = channel;
		// ��������������0��������
		int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, channel->GetFd(), &event);
		if (ret < 0)
		{
			return false;
		}
		channelList.push_back(channel);
		return true;
	}
}

void MyinsKernel::DelChannel(MChannel *channel)
{
	// ��������������0ȡ����������
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, channel->GetFd(), NULL);
	channelList.remove(channel);
	channel->fini();
	delete channel;
	channel = NULL;
}

bool MyinsKernel::ModChannel_AddOut(MChannel *channel)
{
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLOUT | EPOLLET;
	event.data.ptr = channel;
	// ��������������0��������
	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, channel->GetFd(), &event);
	if (ret < 0)
	{
		return false;
	}
	return true;
}

void MyinsKernel::ModChannel_DelOut(MChannel *channel)
{
	// ��������������1ȡ����������
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLET;
	event.data.ptr = channel;
	// ��������������0��������
	int ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, channel->GetFd(), &event);
}

void MyinsKernel::KernelSendOut(MChannel *channel, string Msg)
{
	SysIOMsg iodic(SysIOMsg::OUT);
	channel->Handle(new ByteMsg(Msg, iodic));
}

MChannel *MyinsKernel::GetChannelByInfo(string info)
{
	for (auto *channel : channelList)
	{
		if (info == (*channel).GetChannelInfo())
		{
			return &(*channel);
		}
	}
	return nullptr;
}

MyinsKernel::~MyinsKernel()
{
	close(epoll_fd);
}
// ִ����Ӧ�Ĺܵ�����
void task::process()
{
	// ��Ҫ��������
	if (0 != (event.events & EPOLLIN))
	{
		auto in = static_cast<MChannel *>(event.data.ptr);
		SysIOMsg *pmsg = new SysIOMsg(SysIOMsg::IN);
		in->Handle(pmsg);
	}
	// ��Ҫ���,ˢ�»���
	if (0 != (event.events & EPOLLOUT))
	{
		auto out = static_cast<MChannel *>(event.data.ptr);
		out->FlushOut();
		MyinsKernel::GetInstance()->ModChannel_DelOut(out);
	}
}
