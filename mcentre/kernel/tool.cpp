#include "tool.h"
namespace mytool {

int mgets(char *buf, int size, int fd)
{
    int ret = 0;
    char c;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    if (0 <= select(fd + 1, &readfds, NULL, NULL, NULL))
    {
        while(recv(fd, &c, 1, MSG_DONTWAIT))
        {
            buf[ret++] = c;
            if (c == '\n')
            {
                break;
            }
            if (ret >= size -1)
            {
                break;
            }
        }
        buf[ret] = '\0';
    }
    return ret;
}

int writemsg(int fd, void *msg, int left_byte)
{
    int pos = 0;
    int size;
    errno = 0;
    // 文件写描述符
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    int con;
    // 设置超时退出循环
    struct timeval timeout = {TIMEOUT, 0};
    if (0 < (con = select(fd + 1, NULL, &fds, &fds, &timeout)))
    {
        while(left_byte > 0)
        {
            size = send(fd, (char*)msg + pos, left_byte, MSG_DONTWAIT | MSG_NOSIGNAL);
            if (EPIPE == errno)
            {
                break;
            }
            if (-1 == size)
            {
                continue;
            }
            pos += size;
            left_byte -= size;
        }
    }
    return 0 == con ? con : pos;
}
int readmsg(int fd, void *msg, int left_byte)
{
    int pos = 0;
    int size = 0;
    // 重置各条件
    fd_set readfds;
    errno = 0;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    int con;
    // 设置超时退出循环
    struct timeval timeout = {TIMEOUT, 0};
    if (0 < (con = select(fd + 1, &readfds, NULL, &readfds, &timeout)))
    {
        while(left_byte > 0)
        {
            size = recv(fd, (char*)msg + pos, left_byte, MSG_DONTWAIT);
            if (-1 == size && EAGAIN == errno)
            {
                errno = 0;
                continue;
            }
            else if (0 == size)
            {
                break;
            }
            else if (-1 != size)
            {
                pos += size;
                left_byte -= size;
            }
        }
    }
    return 0 == con ? con : pos;
}
void xto_char(unsigned char *xmd, char *smd, int len)
{
    char cmd[5];
    for (int i = 0; i < len; i++)
    {
        sprintf(cmd, "%0x", xmd[i]);
        strncat(smd, cmd, strlen(cmd) + 1);
    }
}

int check(const char *clientmd, unsigned char *servermd)
{
    char md[70];
    xto_char(servermd, md, SHA256_DIGEST_LENGTH);
    return 0 == strncmp(clientmd, md, strlen(md));
}

int find_nnull(std::vector<std::string> &l)
{
    auto i = std::find_if(l.begin(), l.end(), [](std::string s){return s.empty();});
    if (i == l.end())
    {
        return -1;
    }
    else
    {
        return i - l.begin();
    }
}

void printT(int i, int sum)
{
    int n = (i * 100) / sum;
    printf("%d%%", n);
    while(n-- > 0)
    {
        printf("=");
    }
}

}
