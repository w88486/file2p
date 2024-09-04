#include "./kernel/zinx.h"
#include "./kernel/ZinxTCP.h"
#include "protocal.h"
#include <iostream>
using namespace std;

int main(int argc, char const *argv[])
{
    ZinxKernel::ZinxKernelInit();

    auto centrelisten = new ZinxTCPListen(8888, new TcpFact());
    ZinxKernel::Zinx_Add_Channel(*centrelisten);

    ZinxKernel::Zinx_Run();
    return 0;
}
