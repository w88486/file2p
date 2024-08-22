#include "MyinsTcp.h"
#include "threadpool.h"
#include <vector>

class FileClientUI: public Request
{
private:
    Arg arg = Arg(-1);
    vector<string> md;
    /* data */
public:
    void redisplay();
    void upload();
    void download();
    void display_file();
    void display_task();
    void connectToServer();
    void disconnect();
    int checkupload(string filename);
    void reupload(long int blocksize);
    FileClientUI() = default;
    virtual void process();
};