#include <string>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <hiredis/hiredis.h>
#define MAX_BLOCK_NUM 1024
#define FILENAME_SIZE 232
#define TIMEOUT 1000
#define PORT 8888

class Arg
{
public:
    int fd;
    std::string ip;
    MChannel *m_channel = NULL;
    Arg(int _fd, std::string _ip = "") : fd(_fd) , ip(_ip){};
};
class CmdMsg : public MyinsMsg
{
public:
    enum {
        UPLOAD = 0,
        REUPLOAD,
        DONWLOAD,
        REDONWLOAD,
        DISPLAY,
        SPLITFILE,
        END
    }type;
    long int filesize;
    int mdlen;
    int blocknum;
    int blocksize;
    unsigned char filename[FILENAME_SIZE];
    CmdMsg(string);
    CmdMsg() = default;
};


// typedef struct messege
// {
//     enum {
//         UPLOAD = 0,
//         REUPLOAD,
//         DONWLOAD,
//         REDONWLOAD,
//         DISPLAY,
//         SPLITFILE,
//         END
//     }type;
//     long int filesize;
//     int mdlen;
//     int blocknum;
//     int blocksize;
//     unsigned char filename[FILENAME_SIZE];
// }messege;