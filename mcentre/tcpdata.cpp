#include "tcpdata.h"
void TcpData::SendToDevice(MQTT *ctx)
{
    writemsg(getfd(), ctx->buf, ctx->size);
}

AZinxHandler * TcpData::GetInputNextStage(BytesMsg & _oInput)
{
    return next;
}
