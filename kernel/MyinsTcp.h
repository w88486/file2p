#pragma once
#include "Myins.h"
#include "typedef.h"

class TcpChannelFact;
class TcpListen : public MChannel {
	int port = 0;
	int m_fd = -1;
	TcpChannelFact *tcpFact = NULL;
public:
	TcpListen(int _port, TcpChannelFact* fact) : port(_port),tcpFact(fact) {};
	// ͨ�� MChannel �̳�
	bool init() override;

	bool ReadFd(string& _input) override;

	bool WriteFd(string& _output) override;

	int GetFd() override;

	void fini() override;

	string GetChannelInfo() override;

	Mhandler* GetNextStage(MyinsMsg* _input) override;
	virtual ~TcpListen();
};
class TcpData : public MChannel {
public:
	Mhandler* m_out = NULL;
	Arg m_arg;
	//�̳�TcpData������Ӧͨ�����캯�����츸��
	TcpData(Arg _arg) : m_arg(_arg) {};
	// ͨ�� MChannel �̳�
	bool init() override;
	bool ReadFd(string& _input) override;
	bool WriteFd(string& _output) override;
	int GetFd() override;
	void fini() override;
	string GetChannelInfo() override;
	virtual Mhandler* GetNextStage(MyinsMsg* _input) = 0;
	virtual ~TcpData()
	{
		if (NULL != m_out)
		{
			delete m_out;
			m_out = NULL;
		}
	}
};
class TcpChannelFact {
public:
	virtual TcpData* CreateTcpChannel(Arg _arg) = 0;
	virtual ~TcpChannelFact() {}
};