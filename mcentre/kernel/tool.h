#ifndef TOOL_H
#define TOOL_H
#include <string>
#include <vector>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/select.h>
#include <algorithm>
#include <fcntl.h>
#include <sys/stat.h>
#define BUFSIZE 1024
#define TIMEOUT 1000

class Arg
{
public:
    int fd;
    std::string ip;
    Arg(int _fd, std::string _ip = "") : fd(_fd) , ip(_ip){};
};

namespace mytool {
int mgets(char *buf, int size, int fd);
int writemsg(int fd, void *msg, int left_byte);
int readmsg(int fd, void *msg, int left_byte);
void xto_char(unsigned char *xmd, char *smd, int len);
int check(const char *clientmd, unsigned char *servermd);
int find_nnull(std::vector<std::string> &l);
void printT(int i, int sum);
}
#endif
