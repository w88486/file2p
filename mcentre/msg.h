#ifndef MSG_H
#define MSG_H
#include "./kernel/zinx.h"
#include "./kernel/MQTT.h"

class DeviceMsg : public UserData
{
public:
    MQTT *ctx = NULL;
    DeviceMsg(string _str)
    {
        ctx = (MQTT*)malloc(sizeof(MQTT));
        MQTT_Init(ctx, MQTT::Reserved, 50);
        MQTT_Convert(ctx, _str.c_str(), _str.length() + 1);
    }
    DeviceMsg(MQTT *_ctx)
    {
        ctx = _ctx;
    }
    virtual ~DeviceMsg()
    {
        if (NULL != ctx)
        {
            MQTT_Fini(ctx);
        }
    }
};

#endif // MSG_H
