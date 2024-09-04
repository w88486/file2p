#ifndef TYPEDEF_H
#define TYPEDEF_H
#include <string>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <hiredis/hiredis.h>
#include "MChannel.h"
#include "../kernel/MQTT.h"
#define MAX_BLOCK_NUM 1024
#define FILENAME_SIZE 232
#define TIMEOUT 1000
#define PORT 8888
#define FILEPORT 6666
#define DEFBLOCKSIZE (256 * 1024)

class Arg
{
public:
    int fd;
    std::string ip;
    MChannel *m_channel = NULL;
    Arg(int _fd, std::string _ip = "") : fd(_fd), ip(_ip) {};
};
class DeviceMsg : public ByteMsg
{
public:
    MQTT *ctx = NULL;
    DeviceMsg(std::string _str) : ByteMsg(_str, *(new SysIOMsg(SysIOMsg::OUT)))
    {
        ctx = (MQTT *)malloc(sizeof(MQTT));
        MQTT_Init(ctx, MQTT::Reserved, 50);
        MQTT_Convert(ctx, _str.c_str(), _str.length() + 1);
    }
    DeviceMsg(ByteMsg _byte) : ByteMsg(_byte)
    {
        ctx = (MQTT*)malloc(sizeof(MQTT));
        MQTT_Init(ctx, MQTT::Reserved, 50);
        MQTT_Convert(ctx, _byte.content.c_str(), _byte.content.length() + 1);
    }
    DeviceMsg(MQTT *_ctx) : ByteMsg(std::string((char *)_ctx->buf), *(new SysIOMsg(SysIOMsg::OUT)))
    {
        ctx = (MQTT*)malloc(sizeof(MQTT));
        MQTT_Init(ctx, _ctx->type, _ctx->capacity);
        MQTT_Convert(ctx, (char *)_ctx->buf, _ctx->size);
    }
    virtual ~DeviceMsg()
    {
        // if (NULL != ctx)
        // {
        //     MQTT_Fini(ctx);
        // }
    }
    std::string filename;
    int blocknum;
    int blocksize;
    int filesize;
    int filenum;
    int mdlen;
    int fd;
    std::string ip;
};
#endif