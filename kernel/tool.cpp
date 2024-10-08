#include "tool.h"
#include "typedef.h"
#include <cstdio>

#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define GRAY "\033[0;90m"
#define RESET "\033[0m"

int mgets(char *buf, int size, int fd)
{
    int ret = 0;
    char c;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    if (0 <= select(fd + 1, &readfds, NULL, NULL, NULL))
    {
        while (recv(fd, &c, 1, MSG_DONTWAIT))
        {
            buf[ret++] = c;
            if (c == '\n')
            {
                break;
            }
            if (ret >= size - 1)
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
        while (left_byte > 0)
        {
            size = send(fd, (char *)msg + pos, left_byte, MSG_DONTWAIT | MSG_NOSIGNAL);
            if (EPIPE == errno)
            {
                con = -1;
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
    return 0 != con ? con : pos;
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
        while (left_byte > 0)
        {
            size = recv(fd, (char *)msg + pos, left_byte, MSG_DONTWAIT);
            if (-1 == size && EAGAIN == errno)
            {
                errno = 0;
                continue;
            }
            else if (0 == size)
            {
                con = -1;
                break;
            }
            else if (-1 != size)
            {
                pos += size;
                left_byte -= size;
            }
        }
    }
    return 0 != con ? con : pos;
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

int find_nnull(std::bitset<MAX_BLOCK_NUM> &l)
{
    int i = l._Find_first();
    if (i == l.size())
    {
        return -1;
    }
    else
    {
        return i;
    }
}

void printT(int i, int sum, std::bitset<MAX_BLOCK_NUM> &l)
{
    int n = (i * 100) / sum;
    printf("%d%%", n);
    while (n-- > 0)
    {
        printf("=");
    }
       printf("\n");
    for (int j = 0; j < sum; j++)
    {
        if (j < i)
        {
            if (l[j])
            {
                printf("%sE%s ", RED, RESET);
            }
            else
            {
                printf("%sC%s ", GREEN, RESET);
            }
        }
        else
        {
            printf("%sU%s ", GRAY, RESET);
        }
        if (i % 50 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
}
