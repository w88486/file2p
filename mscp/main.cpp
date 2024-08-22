#include<iostream>
#include"MyinsKernel.h"
#include"MChannel.h"
#include"Mhandler.h"
#include"MyinsTcp.h"
#include"FileTcpData.h"
#include"FileCpCli.h"
#include"threadpool.h"

using namespace std;
int main() {
	auto server = new TcpListen(PORT, new FileTcpFact());
	auto client = new FileClientUI();
	MyinsKernel::GetInstance()->AddChannel(server);
	MyinsKernel::GetInstance()->AddTask(client);
	/*auto fifoout = new FIFOChannel("fifoout", false, NULL);
	auto fifoin = new FIFOChannel("fifoin", true, fifoout);
	MyinsKernel::GetInstance()->AddChannel(fifoin);
	MyinsKernel::GetInstance()->AddChannel(fifoout);*/
	MyinsKernel::GetInstance()->run();
	return 0;
}