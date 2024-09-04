#include "../kernel/MyinsTcp.h"
#include "../kernel/threadpool.h"
#include <vector>
#include "../kernel/typedef.h"
#include <bitset>

class FileClientUI: public Request
{
private:
    EVP_MD_CTX *filectx = NULL;
    int fd;
    std::string ip;
    vector<string> md;
    std::bitset<MAX_BLOCK_NUM> md_flag;
    char buf[BUFSIZE];
    int pos = 0;
    /* data */
public:
    void redisplay();
    void upload();
    void download();
    void redownload(DeviceMsg &msg);
    void storemd(DeviceMsg &msg);
    void removemd(DeviceMsg &msg);
    int loadmd(DeviceMsg &msg, char *mdfile, int left_size);
    int recvfile(DeviceMsg &msg, int i, int left_size, EVP_MD_CTX *filectx);
    void combinefile(DeviceMsg &msg);
    void connectToServer();
    void disconnect();
    DeviceMsg *SearchFile(std::string filename);
    //
    void display_file();
    //
    void display_task();

    FileClientUI() {};
    virtual void process();
};