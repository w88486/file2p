#include "protocal.h"
#include "tcpdata.h"
#include "msg.h"
#include "role.h"

Protocal::Protocal()
{

}

/*原始数据和业务数据相互函数，开发者重写该函数，实现协议*/
UserData *Protocal::raw2request(std::string _szInput)
{
    return new DeviceMsg(_szInput);
}

/*原始数据和业务数据相互函数，开发者重写该函数，实现协议*/
std::string *Protocal::response2raw(UserData &_oUserData)
{
    GET_REF2DATA(DeviceMsg, ret, _oUserData);
    auto retstr = new std::string((char*)(ret.ctx->buf));
    return retstr;
}
/*获取处理角色对象函数，开发者应该重写该函数，用来指定当前产生的用户数据消息应该传递给哪个角色处理*/
Irole *Protocal::GetMsgProcessor(UserDataMsg &_oUserDataMsg)
{
    return next;
}

/*获取发送通道函数，开发者应该重写该函数，用来指定当前字节流应该由哪个通道对象发出*/
Ichannel *Protocal::GetMsgSender(BytesMsg &_oBytes)
{
    return pre;
}

ZinxTcpData *TcpFact::CreateTcpDataChannel(int _fd)
{
    auto *role = new ChoiceRole();


    Protocal *protocal = new Protocal();

    auto *data = new TcpData(_fd);

    role->proto = protocal;

    data->next = protocal;

    protocal->pre = data;
    protocal->next = role;

    ZinxKernel::Zinx_Add_Proto(*protocal);
    ZinxKernel::Zinx_Add_Role(*role);

    return data;
}
