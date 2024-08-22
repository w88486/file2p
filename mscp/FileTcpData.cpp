#include "FileTcpData.h"
#include "FileControl.h"
TcpData* FileTcpFact::CreateTcpChannel(Arg _arg)
{
    FileTcpData* channel = new FileTcpData(_arg);
    // 
    channel->m_out = new TcpControl(_arg);
    return channel;
}

Mhandler* FileTcpData::GetNextStage(MyinsMsg* _input)
{
    return m_out;
}
