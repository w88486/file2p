#include "MChannel.h"
#include<stdlib.h>
#include <iostream>
#include<unistd.h>
#include <fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<algorithm>
using namespace std;
bool StdinChannel::init()
{

	return true;
}

bool StdinChannel::ReadFd(string& _input)
{
	string str;
	cin >> str;
	_input.assign(str);
	return true;
}

bool StdinChannel::WriteFd(string& _output)
{
	return false;
}

int StdinChannel::GetFd()
{
	return 0;
}


void StdinChannel::fini()
{
}

Mhandler* StdinChannel::GetNextStage(MyinsMsg* _input)
{
	return m_out;
}

string StdinChannel::GetChannelInfo()
{
	return string();
}


bool StdOutChannel::init()
{
	return true;
}

bool StdOutChannel::ReadFd(string& _input)
{
	return true;
}

bool StdOutChannel::WriteFd(string &_ouput)
{
	cout << _ouput << endl;
	return true;
}

int StdOutChannel::GetFd()
{
	return 1;
}


void StdOutChannel::fini()
{
}

Mhandler* StdOutChannel::GetNextStage(MyinsMsg* _input)
{
	return nullptr;
}

string StdOutChannel::GetChannelInfo()
{
	return string();
}

FIFOChannel::FIFOChannel(string fn, bool flag, MChannel *out) :filename(fn), isRead(flag), m_out(out)
{

}

bool FIFOChannel::init()
{
	//���ݹܵ�������ļ�
	int flag = O_RDONLY;
	if (false == isRead) {
		flag = O_WRONLY;
	}
	int fd = open(filename.c_str(), flag | O_NONBLOCK);
	if (fd < 0) {
		printf("open erro\n");
		return false;
	}
	m_fd = fd;
	return true;
}

bool FIFOChannel::ReadFd(string& _intput)
{
	//��ȡ����
	char buffer[1024] = { 0 };
	int len = read(m_fd, buffer, sizeof(buffer));
	_intput.assign(buffer, len);
	return true;
}

bool FIFOChannel::WriteFd(string& _output)
{
	//��string����������������д��
	char *buffer = (char*)malloc(_output.size());
	_output.copy(buffer, _output.size(), 0);
	int ret = write(m_fd, buffer, _output.size());
	free(buffer);
	if (ret < 0) {
		printf("write erro\n");
		return false;
	}
	return true;
}

int FIFOChannel::GetFd()
{
	return m_fd;
}


void FIFOChannel::fini()
{
	//�رչܵ�
	close(m_fd);
}

Mhandler* FIFOChannel::GetNextStage(MyinsMsg* _input)
{
	return m_out;
}

string FIFOChannel::GetChannelInfo()
{
	return string();
}


void MChannel::DataSendOut(string _ouput)
{
	//��¼����������
	m_buffer.append(_ouput);
	//�޸�epoll�������ͣ����out
	MyinsKernel::GetInstance()->ModChannel_AddOut(this);
}

void MChannel::FlushOut()
{
	WriteFd(m_buffer);
	m_buffer.clear();
}

string MChannel::Convert2Printable(string _output)
{
	char* pcTemp = (char*)calloc(1UL, _output.size() * 3 + 1);
	int iCurPos = 0;

	if (NULL != pcTemp)
	{
		for (int i = 0; i < _output.size(); ++i)
		{
			iCurPos += sprintf(pcTemp + iCurPos, "%02X ", (unsigned char)_output[i]);
		}
		pcTemp[iCurPos] = 0;
	}

	std::string szRet = std::string(pcTemp);
	free(pcTemp);

	return szRet;
}

MyinsMsg* MChannel::InternelHandle(MyinsMsg* _input)
{
	auto pmsg = dynamic_cast<SysIOMsg*>(_input);
	if (NULL != pmsg) {
		//������������ȡ
		if (pmsg->dic == SysIOMsg::IN) {
			string content;
			if (true == ReadFd(content)) {
				auto msg = new ByteMsg(content, *pmsg);
				msg->chInfo = this->GetChannelInfo();
				return msg;
			}
		}
		else {
			//������������������ݣ����ı�epoll
			auto pret = dynamic_cast<ByteMsg*>(_input);
			if (NULL != pret) {
				DataSendOut(pret->content);
			}
		}
	}
	return nullptr;
}

Mhandler* MChannel::GetNext(MyinsMsg* _input)
{
	auto pmsg = dynamic_cast<ByteMsg*>(_input);
	if (NULL != pmsg) {
		return GetNextStage(pmsg);
	}
	return nullptr;
}

void dataProcess::toup(string &input)
{
	transform(input.begin(), input.end(), input.begin(), ::toupper);
}


MyinsMsg* dataProcess::InternelHandle(MyinsMsg* _input)
{
	auto pmsg = dynamic_cast<ByteMsg*>(_input);
	if (NULL != pmsg) {
		if ('a' <= pmsg->content[0] && pmsg->content[0] <= 'z') {
			toup(pmsg->content);
		}
		//����send
		MyinsKernel::GetInstance()->KernelSendOut(m_out, pmsg->content);
	}
	return nullptr;
}

Mhandler* dataProcess::GetNext(MyinsMsg* _input)
{
	return nullptr;
}
