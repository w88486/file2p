#include "FileTcpData.h"
#include "FileCpServer.h"
TcpData* FileTcpFact::CreateTcpChannel(Arg _arg)
{
    FileTcpData* channel = new FileTcpData(_arg);
    // 关联通道和处理类
    _arg.m_channel = channel;
    channel->m_out = new FileCpServer(_arg);
    return channel;
}

Mhandler* FileTcpData::GetNextStage(MyinsMsg* _input)
{
    return m_out;
}
