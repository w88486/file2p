#ifndef MCHANNEL_H
#define MCHANNEL_H
#include "MChannel.h"
#include<stdlib.h>
#include <iostream>
#include<unistd.h>
#include <fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<algorithm>
using namespace std;

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
#endif