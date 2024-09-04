#ifndef ROLE_H
#define ROLE_H
#include "./kernel/zinx.h"
#include "protocal.h"
#include <hiredis/hiredis.h>
#include "./kernel/zinx.h"

class ChoiceRole : public Irole
{
    friend TcpFact;
    Iprotocol *proto = NULL;
public:
    ChoiceRole() {};
    /*初始化函数，开发者可以重写该方法实现对象相关的初始化，该函数会在role对象添加到zinxkernel时调用*/
    virtual bool Init();

    /*处理信息函数，重写该方法可以对业务数据进行处理，若还需要后续处理流程，则应返回相关数据信息（堆对象）*/
    virtual UserData *ProcMsg(UserData &_poUserData);

    /*去初始化函数，类似初始化函数，该函数会在role对象摘除出zinxkernel时调用*/
    virtual void Fini();
};

class IpOfFile : public Irole
{
    friend TcpFact;
    Iprotocol *proto = NULL;
public:
    IpOfFile(Iprotocol *_proto) : proto(_proto) {};
    /*初始化函数，开发者可以重写该方法实现对象相关的初始化，该函数会在role对象添加到zinxkernel时调用*/
    virtual bool Init();

    /*处理信息函数，重写该方法可以对业务数据进行处理，若还需要后续处理流程，则应返回相关数据信息（堆对象）*/
    virtual UserData *ProcMsg(UserData &_poUserData);

    /*去初始化函数，类似初始化函数，该函数会在role对象摘除出zinxkernel时调用*/
    virtual void Fini();
};

#endif // ROLE_H

