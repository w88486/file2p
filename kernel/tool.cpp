#include "tool.h"
#include "typedef.h"

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
    for (size_t i = 0; i < len; i++)
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

int recvfile(int fd, std::string outfile, int left_size, EVP_MD_CTX *filectx)
{
    // 初始化hash生成器
    unsigned char md[SHA256_DIGEST_LENGTH];
    EVP_MD_CTX *ctx = EVP_MD_CTX_create();
    const EVP_MD *mdal = EVP_sha256();
    EVP_DigestInit(ctx, mdal);

    int outfd = open(outfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
    char buf[BUFSIZE];
    printf("文件长度%d\n", left_size);
    int size;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    // 读取文件
    if (0 <= select(fd + 1, &readfds, NULL, &readfds, NULL))
    {
        while(size = recv(fd, &buf, BUFSIZE, MSG_DONTWAIT))
        {
            if (EAGAIN == errno)
            {
                errno = 0;
                continue;
            }
            else if (-1 == size)
            {
                continue;
            }
            else if (0 == size)
            {
                EVP_MD_CTX_destroy(ctx);
                return size;
            }
            printf("read %d left size %d\n", size, left_size);
            EVP_DigestUpdate(ctx, buf, size);
            if (NULL != filectx)
            {
                EVP_DigestUpdate(filectx, buf, size);
            }
            write(outfd, buf, size);
            left_size -= size;
            if (0 >= left_size)
            {
                break;
            }
        }
    }
    // 返回结果
    char *ret = "ACK";
    unsigned int len = SHA256_DIGEST_LENGTH;
    EVP_DigestFinal(ctx, md, &len);
    if (0 == check(outfile.c_str(), md))
    {
        ret = "ERR";
        fprintf(stderr, "接收%s的hash值有误！\n", outfile);
    }
    else
    {
        printf("接收%s文件成功\n", outfile);
    }
    writemsg(fd, ret, 4);
    close(outfd);
    return size;
}

int loadmd(int fd, char *mdfile, int left_size, std::vector<std::string> &md, int num)
{
    // 打开md文件
    int mdfd = open(mdfile, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (-1 == mdfd)
    {
        fprintf(stderr ,"打开文件%s失败！\n", mdfile);
        write(fd, "ERR", 4);
        return -1;
    }
    char buf[BUFSIZE];
    // 返回0对端断开连接
    printf("md文件长度%d\n", left_size);
    int size;
    // 读取md文件
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    if (0 <= select(fd + 1, &readfds, NULL, &readfds, NULL))
    {
        while(size = recv(fd, &buf, BUFSIZE, MSG_DONTWAIT))
        {
            if (EAGAIN == errno)
            {
                errno = 0;
                continue;
            }
            else if (-1 == size)
            {
                continue;
            }
            else if (0 == size)
            {
                return size;
            }
            write(mdfd, buf, size);
            left_size -= size;
            if (0 >= left_size)
            {
                break;
            }
        }
    }
    lseek(mdfd, 0L, SEEK_SET);
    // 将md文件加载到数组中
    for (int i = 0; i < num; i++)
    {
        char tmp[70];
        mgets(tmp, 70, mdfd);
        md.push_back(tmp);
    }
    
    char *ret = "ACK";
    printf("接收%s文件成功\n", mdfile);
    writemsg(fd, ret, 4);
    close(mdfd);
    return size;
}

void redownload(int fd)
{
    CmdMsg msg;
    while(0 < readmsg(fd, &msg, sizeof(msg)))
    {
        if (CmdMsg::END == msg.type)
        {
            break;
        }
        recvfile(fd, (char*)msg.filename, msg.filesize, NULL);
    }
}

void combinefile(std::vector<std::string> md, const char* filename, int num)
{
    int outfd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    char buf[BUFSIZE];
    int size;
    for (size_t i = 0; i < num; i++)
    {
        int infd = open(md[i].c_str(), O_RDONLY);
        while(0 != (size = read(infd, buf, BUFSIZE)))
        {
            write(outfd, buf, size);
        }
        close(infd);
    }
    close(outfd);
}
