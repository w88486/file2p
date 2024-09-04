#include "role.h"
#include "./kernel/MQTT.h"
#include "msg.h"

/*初始化函数，开发者可以重写该方法实现对象相关的初始化，该函数会在role对象添加到zinxkernel时调用*/
bool ChoiceRole::Init()
{
    return true;
}

/*处理信息函数，重写该方法可以对业务数据进行处理，若还需要后续处理流程，则应返回相关数据信息（堆对象）*/
UserData *ChoiceRole::ProcMsg(UserData &_poUserData)
{
    GET_REF2DATA(DeviceMsg, msg, _poUserData);
    // 进行相应操作 搜索文件所在地（处理连接请求，等等并根据网络拓扑图）
    switch(msg.ctx->type)
    {
        case MQTT::SUBSCRIBE:
            SetNextProcessor(*(new IpOfFile(proto)));
            break;
        case MQTT::CONNECT:
            break;
    }
    return &_poUserData;
}

/*去初始化函数，类似初始化函数，该函数会在role对象摘除出zinxkernel时调用*/
void ChoiceRole::Fini()
{

}

/*初始化函数，开发者可以重写该方法实现对象相关的初始化，该函数会在role对象添加到zinxkernel时调用*/
bool IpOfFile::Init()
{
    return true;
}

/*处理信息函数，重写该方法可以对业务数据进行处理，若还需要后续处理流程，则应返回相关数据信息（堆对象）*/
UserData *IpOfFile::ProcMsg(UserData &_poUserData)
{
    GET_REF2DATA(DeviceMsg, msg, _poUserData);
    // 进行相应操作 搜索文件所在地（处理连接请求，等等并根据网络拓扑图）

    // 获取文件名
    char filename[1024];
    MQTT_GetVHeader(msg.ctx, "filename", filename, sizeof(filename));
    // 从redis中查找文件的地址
    std::vector<std::string> params;
    params.emplace_back("host");
    std::string pos = ZinxKernel::Zinx_Hget(filename, params)[0];
    // 填充回复报文
    MQTT *ctx = (MQTT*)malloc(sizeof(MQTT));
    if (0 == pos.compare(""))
    {
        // 未找到文件
        MQTT_Init(ctx, MQTT::UNSUBSCRIBE, 2);
    }
    else
    {
        // 填充文件的位置
        MQTT_Init(ctx, MQTT::SUBSCRIBE, 50);
        MQTT_Fill(ctx, "filehost", pos.c_str());
    }
    auto retmsg = new DeviceMsg(ctx);
    ZinxKernel::Zinx_SendOut(*retmsg, *proto);
    return NULL;
}

/*去初始化函数，类似初始化函数，该函数会在role对象摘除出zinxkernel时调用*/
void IpOfFile::Fini()
{

}
