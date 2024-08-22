#define DEBUG
#include"MyinsTcp.h"
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<stdlib.h>
#include<iostream>
#include<arpa/inet.h>

bool TcpListen::init()
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	bool ret = false;
	if (fd < 0) {
		perror(__FILE__"socket");
	}
	else {
		struct sockaddr_in server_in;
		server_in.sin_family = AF_INET;
		server_in.sin_port = htons(port);
		server_in.sin_addr.s_addr = INADDR_ANY;
		//�����ظ���
		int flag = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(server_in));
		if (0 > bind(fd, (struct sockaddr*)&server_in, sizeof(server_in))) {
			perror(__FILE__"bind");
		}
		else {
			if (0 > listen(fd, 10000)) {
				perror(__FILE__"listen");
			}
			else {
				m_fd = fd;
				ret = true;
			}
		}
	}
	return ret;
}

bool TcpListen::ReadFd(string& _input)
{
	struct sockaddr_in client_in;
	socklen_t len;
	bool ret = false;
	int client_fd = accept(m_fd, (struct sockaddr*)&client_in, &len);
	if (0 <= client_fd){
		//�����ഴ������ͨ��
		char ip[13];
		int iip = ntohl(client_in.sin_addr.s_addr);
		inet_ntop(AF_INET, &iip, ip, sizeof(ip));
		Arg arg(client_fd, ip);
		auto tcpChannel = tcpFact->CreateTcpChannel(arg);
		MyinsKernel::GetInstance()->AddChannel(tcpChannel);
		ret = true;
	}
	else {
		perror(__FILE__"accept");
	}
	return ret;
}

bool TcpListen::WriteFd(string& _output)
{
	return false;
}

int TcpListen::GetFd()
{
	return m_fd;
}

void TcpListen::fini()
{
	if (0 <= m_fd) {
		close(m_fd);
	}
}

string TcpListen::GetChannelInfo()
{
	return string("TCP_LISTEN");
}

Mhandler* TcpListen::GetNextStage(MyinsMsg* _input)
{
	return nullptr;
}

TcpListen::~TcpListen()
{
	if (NULL != tcpFact) {
		delete tcpFact;
	}
}

bool TcpData::init()
{
	return true;
}

bool TcpData::ReadFd(string& _input)
{
	bool ret = false;
	char buf[1024] ={ 0 };
	int retl;
	//ÿ�η�������
	while (0 < (retl = recv(m_fd, buf, sizeof(buf), MSG_DONTWAIT))) {
			_input.append(buf, retl);
			ret = true;
	}
	if (false == ret) {
		SetChannelClose();
	}
	return ret;
}

bool TcpData::WriteFd(string& _output)
{
	bool ret = false;
	char *buf = (char*)malloc(_output.length());
	_output.copy(buf, _output.length(), 0);
	if (_output.length() == send(m_fd, buf, sizeof(buf), 0)) {
		ret = true;
		#ifdef DEBUG
		std::cout << "<----------------------------------------->" << std::endl;
		std::cout << "send to " << m_fd << ":" << MChannel::Convert2Printable(_output) << std::endl;
		std::cout << "<----------------------------------------->" << std::endl;
		#endif
	};
	free(buf);
	return false;
}

int TcpData::GetFd()
{
	return m_fd;
}

void TcpData::fini()
{
	if (NULL != m_out) {
		delete m_out;
	}
	if (0 <= m_fd) {
		close(m_fd);
	}
}

string TcpData::GetChannelInfo()
{
	return string("TCP_DATA"+m_fd);
}

class Echo : public Mhandler {
	// ͨ�� Mhandler �̳�
	MyinsMsg* InternelHandle(MyinsMsg* _input) override
	{
		GET_REF_FROM_MSG(ByteMsg, output, *_input);
		//������Ϣ�йܵ���Ϣ��ȡ�ܵ�
		auto channel = MyinsKernel::GetInstance()->GetChannelByInfo(output.chInfo);
		//��ܵ���������
		MyinsKernel::GetInstance()->KernelSendOut(channel,output.content);
		return nullptr;
	}
	Mhandler* GetNext(MyinsMsg* _input) override
	{
		return nullptr;
	}
};

