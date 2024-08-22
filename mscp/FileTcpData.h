#pragma once
#include "MyinsTcp.h"
class FileTcpData :
    public TcpData
{
public:
    FileTcpData(Arg _arg) : TcpData(_arg){};
    // 通过 TcpData 继承
    Mhandler* GetNextStage(MyinsMsg* _input) override;
};

class FileTcpFact :
    public TcpChannelFact
{
public:
    FileTcpFact() = default;

    // 通过 TcpChannelFact 继承
    TcpData* CreateTcpChannel(Arg _arg) override;
};

