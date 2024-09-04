#include "zinx.h"
#include <sys/epoll.h>
#include <unistd.h>
#include "threadpool.h"

ZinxKernel::ZinxKernel()
{
	iEpollFd = epoll_create(1);
    pool = new threadPool<Request>(16);
	redis_ctx = redisConnect("127.0.0.1", 6379);
}

/*redis hget*/
std::vector<std::string> ZinxKernel::Zinx_Hget(std::string key, std::vector<std::string> fields)
{
	return poZinxKernel->HGET(key, fields);
}
std::vector<std::string> ZinxKernel::Zinx_HgetAll(std::string key)
{
	return poZinxKernel->HGET(key);
}
/*redis hset*/
bool ZinxKernel::Zinx_Hset(std::string key, std::map<std::string, std::string> fields)
{
	return poZinxKernel->HSET(key, fields);
}

std::vector<std::string> ZinxKernel::HGET(std::string key, std::vector<std::string> fields)
{
	redisReply *res = NULL;
	const char *argv[fields.size() + 2];
	size_t argl[fields.size() + 2];
	argv[0] = "HGET";
	char tmp[512];
	sprintf(tmp, "fileinfo:%s", key.c_str());
	argv[1] = tmp;
	argl[0] = 4;
	argl[1] = key.length() + 9;
	for (auto i = 0; i < fields.size(); ++i)
	{
		argv[i + 2] = fields[i].c_str();
		argl[i + 2] = fields[i].length();
	}
	res = (redisReply*)redisCommandArgv(redis_ctx, fields.size() + 2, argv, argl);
	std::vector<std::string> ret;
	if (NULL == res || REDIS_REPLY_ERROR == res->type || REDIS_REPLY_NIL == res->type)
	{
		freeReplyObject(res);
		ret = {""};
		return ret;
	}
	ret = {res->str};
	freeReplyObject(res);
	return ret;
}
std::vector<std::string> ZinxKernel::HGET(std::string key)
{

}
bool ZinxKernel::HSET(std::string key, std::map<std::string, std::string> fields)
{

}
ZinxKernel::~ZinxKernel()
{
	for (auto itr : m_ChannelList)
	{
		itr->Fini();
		delete itr;
	}
	if (iEpollFd >= 0)
	{
		close(iEpollFd);
	}
    delete pool;
	redisFree(redis_ctx);
}

bool ZinxKernel::Add_Channel(Ichannel & _oChannel)
{
	bool bRet = false;

	if (true == _oChannel.Init())
	{
		struct epoll_event stEvent;
		stEvent.events = EPOLLIN | EPOLLET;
		stEvent.data.ptr = &_oChannel;

		if (0 == epoll_ctl(iEpollFd, EPOLL_CTL_ADD, _oChannel.GetFd(), &stEvent))
		{
			m_ChannelList.push_back(&_oChannel);
			bRet = true;
		}
	}

	return bRet;
}

void ZinxKernel::Del_Channel(Ichannel & _oChannel)
{
	m_ChannelList.remove(&_oChannel);
	epoll_ctl(iEpollFd, EPOLL_CTL_DEL, _oChannel.GetFd(), NULL);
	_oChannel.Fini();
}

bool ZinxKernel::Add_Proto(Iprotocol & _oProto)
{
	m_ProtoList.push_back(&_oProto);
	return true;
}

void ZinxKernel::Del_Proto(Iprotocol & _oProto)
{
	m_ProtoList.remove(&_oProto);
}

bool ZinxKernel::Add_Role(Irole & _oRole)
{
	bool bRet = false;

	if (true == _oRole.Init())
	{
		m_RoleList.push_back(&_oRole);
		bRet = true;
	}

	return bRet;
}

void ZinxKernel::Del_Role(Irole & _oRole)
{
	m_RoleList.remove(&_oRole);
	_oRole.Fini();
}

void ZinxKernel::Run()
{
	int iEpollRet = -1;
	while (false == m_need_exit)
	{
		struct epoll_event atmpEvent[100];
        iEpollRet = epoll_wait(iEpollFd, atmpEvent, 100, -1);
		if (-1 == iEpollRet)
		{
			if (EINTR == errno)
			{
				continue;
			}
			else
			{
				break;
			}
		}
		for (int i = 0; i < iEpollRet; i++)
		{
			/*如果是监听通道类则从epoll中删除， 只处理一次*/
			if (((Ichannel*)(atmpEvent[i].data.ptr))->GetChannelInfo().compare("TcpListen") == 0)
			{
				ZinxKernel::Zinx_ClearChannelIn(*(Ichannel*)(atmpEvent[i].data.ptr));
			}
			pool->appand(new task(atmpEvent[i]));
		}
	}
}
void ZinxKernel::SendOut(UserData & _oUserData)
{

}
ZinxKernel *ZinxKernel::poZinxKernel = NULL;
bool ZinxKernel::ZinxKernelInit()
{
	if (NULL == poZinxKernel)
	{
		poZinxKernel = new ZinxKernel();
	}
	
	return true;
}

void ZinxKernel::ZinxKernelFini()
{
	delete poZinxKernel;
	poZinxKernel = NULL;
}

bool ZinxKernel::Zinx_Add_Channel(Ichannel & _oChannel)
{
	return poZinxKernel->Add_Channel(_oChannel);
}

void ZinxKernel::Zinx_Del_Channel(Ichannel & _oChannel)
{
	poZinxKernel->Del_Channel(_oChannel);
}

Ichannel *ZinxKernel::Zinx_GetChannel_ByInfo(std::string _szInfo)
{
    Ichannel *pret = NULL;

    for (auto itr:poZinxKernel->m_ChannelList)
    {
        if (_szInfo == (*itr).GetChannelInfo())
        {
            pret = &(*itr);
        }
    }

    return pret;
}

void ZinxKernel::Zinx_SetChannelOut(Ichannel & _oChannel)
{
	struct epoll_event stEvent;
	stEvent.events = EPOLLIN | EPOLLOUT | EPOLLET;
	stEvent.data.ptr = &_oChannel;

	epoll_ctl(poZinxKernel->iEpollFd, EPOLL_CTL_MOD, _oChannel.GetFd(), &stEvent);
}

void ZinxKernel::Zinx_ClearChannelOut(Ichannel & _oChannel)
{
	struct epoll_event stEvent;
	stEvent.events = EPOLLIN  | EPOLLET;
	stEvent.data.ptr = &_oChannel;

	epoll_ctl(poZinxKernel->iEpollFd, EPOLL_CTL_MOD, _oChannel.GetFd(), &stEvent);
}

void ZinxKernel::Zinx_ClearChannelIn(Ichannel & _oChannel)
{
	epoll_ctl(poZinxKernel->iEpollFd, EPOLL_CTL_DEL, _oChannel.GetFd(), NULL);
}

bool ZinxKernel::Zinx_Add_Proto(Iprotocol & _oProto)
{
	return poZinxKernel->Add_Proto(_oProto);
}

void ZinxKernel::Zinx_Del_Proto(Iprotocol & _oProto)
{
	poZinxKernel->Del_Proto(_oProto);
}

bool ZinxKernel::Zinx_Add_Role(Irole & _oRole)
{
	return poZinxKernel->Add_Role(_oRole);
}

std::list<Irole*> &ZinxKernel::Zinx_GetAllRole()
{
	return poZinxKernel->m_RoleList;
}

void ZinxKernel::Zinx_Del_Role(Irole & _oRole)
{
	poZinxKernel->Del_Role(_oRole);
}

void ZinxKernel::Zinx_Run()
{
	poZinxKernel->Run();
}

// 如果要发送的数据不是字符串，则需经过协议层处理成字符串
void ZinxKernel::Zinx_SendOut(UserData & _oUserData, Iprotocol & _oProto)
{
	SysIOReadyMsg iodic = SysIOReadyMsg(SysIOReadyMsg::OUT);
	BytesMsg oBytes = BytesMsg(iodic);
	UserDataMsg oUserDataMsg = UserDataMsg(oBytes);

	oUserDataMsg.poUserData = &_oUserData;
	_oProto.Handle(oUserDataMsg);
}
// 如果要发送的数据就是字符串，则不用经过协议层处理
void ZinxKernel::Zinx_SendOut(std::string & szBytes, Ichannel & _oChannel)
{
	SysIOReadyMsg iodic = SysIOReadyMsg(SysIOReadyMsg::OUT);
	BytesMsg oBytes = BytesMsg(iodic);
	oBytes.szData = szBytes;
	_oChannel.Handle(oBytes);
}

void task::process()
{
    Ichannel *poChannel = static_cast<Ichannel *>(event.data.ptr);
    if (0 != (EPOLLIN & event.events))
	{
		SysIOReadyMsg IoStat = SysIOReadyMsg(SysIOReadyMsg::IN);
		poChannel->Handle(IoStat);
		if (true == poChannel->ChannelNeedClose())
		{
			ZinxKernel::poZinxKernel->Del_Channel(*poChannel);
			delete poChannel;
		}
    }
    else if (0 != (EPOLLOUT & event.events))
	{
		poChannel->FlushOut();
		if (false == poChannel->HasOutput())
		{
			ZinxKernel::poZinxKernel->Zinx_ClearChannelOut(*poChannel);
		}
    }
}
