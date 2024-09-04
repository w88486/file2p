#ifndef PROTOCAL_H
#define PROTOCAL_H
#include "./kernel/zinx.h"
#include "./kernel/ZinxTCP.h"

class TcpFact;

class Protocal : public Iprotocol
{
    friend TcpFact;
    Irole *next = NULL;
    ZinxTcpData *pre = NULL;
public:
    Protocal();
    /*原始数据和业务数据相互函数，开发者重写该函数，实现协议*/
    virtual UserData *raw2request(std::string _szInput);

    /*原始数据和业务数据相互函数，开发者重写该函数，实现协议*/
    virtual std::string *response2raw(UserData &_oUserData);
protected:
    /*获取处理角色对象函数，开发者应该重写该函数，用来指定当前产生的用户数据消息应该传递给哪个角色处理*/
    virtual Irole *GetMsgProcessor(UserDataMsg &_oUserDataMsg);

    /*获取发送通道函数，开发者应该重写该函数，用来指定当前字节流应该由哪个通道对象发出*/
    virtual Ichannel *GetMsgSender(BytesMsg &_oBytes);
};

class TcpFact : public IZinxTcpConnFact
{
public:
    TcpFact() {}
    /*创建一个TCP监听对象，开发者应该重写该函数，用来创建具体的TCP监听对象*/
    virtual ZinxTcpData *CreateTcpDataChannel(int _fd);
};

#endif // PROTOCAL_H
