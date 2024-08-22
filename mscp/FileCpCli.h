#include "../kernel/MyinsTcp.h"
#include "../kernel/threadpool.h"
#include <vector>
#include "../kernel/typedef.h"
#include "FileCpServer.h"

class FileClientUI: public Request, UpLoad
{
private:
    EVP_MD_CTX *filectx = NULL;
    Arg arg = Arg(-1);
    vector<string> md;
    /* data */
public:
    void redisplay();
    void upload();
    void download();
    //
    void display_file();
    //
    void display_task();
    void connectToServer();
    void disconnect();
    FileClientUI() = default;
    virtual void process();
};