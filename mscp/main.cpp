#include<iostream>
#include"../kernel/MyinsTcp.h"
#include"FileTcpData.h"
#include"FileCpCli.h"
#include"../kernel/threadpool.h"

using namespace std;
int main() {
	auto server = new TcpListen(FILEPORT, new FileTcpFact());
	auto client = new FileClientUI();
	MyinsKernel::GetInstance()->AddChannel(server);
	MyinsKernel::GetInstance()->AddTask(client);
	MyinsKernel::GetInstance()->run();
	return 0;
}