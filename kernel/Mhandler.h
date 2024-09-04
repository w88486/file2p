#pragma once
#include<string>
#include<vector>
using namespace std;


class MyinsMsg {
public:
	MyinsMsg() {}
	virtual ~MyinsMsg() {}
};

class Mhandler
{
public:
	void Handle(MyinsMsg *_input);
	virtual MyinsMsg* InternelHandle(MyinsMsg* _input) = 0;
	virtual Mhandler* GetNext(MyinsMsg* _input) = 0;
	virtual ~Mhandler() {}
};
class SysIOMsg : public MyinsMsg {
public:
	enum IOdic {
		IN,
		OUT
	}dic;
	SysIOMsg(IOdic _dic) :dic(_dic){};
};
class ByteMsg : public SysIOMsg {
public:
	string chInfo = "";
	string content;
	ByteMsg(string text, SysIOMsg& ioMsg) :content(text), SysIOMsg(ioMsg.dic) {};
};
class UserData :public ByteMsg{
	
};


