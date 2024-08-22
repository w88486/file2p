#pragma once
#include<string>
#include"MyinsKernel.h"
#include"Mhandler.h"
using namespace std;
class MChannel : public Mhandler
{
public:
	bool m_NeedClose = false;
	string m_buffer;
	vector<string> md;
	void SetChannelClose() { m_NeedClose = true; }
	bool ChannelNeedClose() { return m_NeedClose; }
	virtual bool init() = 0;
	virtual bool ReadFd(string& _input) = 0;
	virtual bool WriteFd(string& _output) = 0;
	virtual int GetFd() = 0;
	void DataSendOut(string _ouput);
	void FlushOut();
	virtual void fini() = 0;
	virtual string GetChannelInfo() = 0;
	//�ܵ���һ��������̣���Ҫ��д
	virtual Mhandler *GetNextStage(MyinsMsg* _input) = 0;
	string Convert2Printable(string _output);
	// ͨ�� Mhandler �̳�
	MyinsMsg* InternelHandle(MyinsMsg* _input) override;
	Mhandler* GetNext(MyinsMsg* _input) override;

};