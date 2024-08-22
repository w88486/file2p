#include <string>
#include "tool.h"
#define BUFSIZE 1024
#define MAX_BLOCK_NUM 1024
#define FILENAME_SIZE 232
#define TIMEOUT 1000
#define PORT 8888

class Arg
{
public:
    int fd;
    string ip;
    Arg(int _fd, string _ip = "") : fd(_fd) , ip(_ip){};
};

typedef struct messege
{
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
}messege;