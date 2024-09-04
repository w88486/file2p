#ifndef TCPDATA_H
#define TCPDATA_H
#include "./kernel/ZinxTCP.h"
#include "./kernel/MQTT.h"
#include "./kernel/tool.h"
#include "protocal.h"
#include <sys/epoll.h>
using namespace mytool;
class TcpData : public ZinxTcpData
{
    friend TcpFact;
    Iprotocol *next = NULL;
public:
    TcpData(int _fd) : ZinxTcpData(_fd) {}
    virtual AZinxHandler * GetInputNextStage(BytesMsg & _oInput);
    void SendToDevice(MQTT *ctx);
};

#endif // TCPDATA_H
