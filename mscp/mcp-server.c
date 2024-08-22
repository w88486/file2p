#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#define BUFSIZE 1024
#define MAX_BLOCK_NUM 1024
#define FILENAME_SIZE 232
#define TIMEOUT 1000

// 可变数组保存为传输成功的ip

struct list *ip;

// 文件总md
__thread char filemd[SHA256_DIGEST_LENGTH];
// 文件hash生成器
__thread EVP_MD_CTX *filectx;

typedef struct arg
{
    int fd;
    char ip[16];
}Arg;

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


int mgets(char *buf, int size, int fd)
{
    int ret = 0;
    char c;
    while(read(fd, &c, 1))
    {
        buf[ret++] = c;
        if (c == '\n')
        {
            break;
        }
    }
    buf[ret] = '\0';
    return ret;
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

// 转换成字符表示的16进制字节
void xto_char(unsigned char *xmd, char *smd, int len)
{
    char cmd[5];
    for (size_t i = 0; i < len; i++)
    {
        sprintf(cmd, "%0x", xmd[i]);
        strncat(smd, cmd, strlen(cmd) + 1);
    }
}
int check(char *clientmd, unsigned char *servermd)
{
    char md[70];
    xto_char(servermd, md, SHA256_DIGEST_LENGTH);
    return 0 == strncmp(clientmd, md, strlen(md));
}

int loadmd(Arg args, char *mdfile, int left_size, char (*md)[70], int num)
{
    // 打开md文件
    int mdfd = open(mdfile, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (-1 == mdfd)
    {
        fprintf(stderr ,"打开文件%s失败！\n", mdfile);
        write(args.fd, "ERR", 4);
        return -1;
    }
    char buf[BUFSIZE];
    // 返回0对端断开连接
    printf("md文件长度%d\n", left_size);
    int size;
    // 读取md文件
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(args.fd, &readfds);
    if (0 <= select(args.fd + 1, &readfds, NULL, &readfds, NULL))
    {
        while(size = recv(args.fd, &buf, BUFSIZE, MSG_DONTWAIT))
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
        strncpy(md[i], tmp, strlen(tmp));
        int pos = strcspn(md[i], "\n");
        md[i][pos] = '\0';
    }
    
    char *ret = "ACK";
    printf("接收%s文件成功\n", mdfile);
    writemsg(args.fd, ret, 4);
    close(mdfd);
    return size;
}

int recvfile(Arg args, char *outfile, int left_size)
{
    // 初始化hash生成器
    unsigned char md[SHA256_DIGEST_LENGTH];
    EVP_MD_CTX *ctx = EVP_MD_CTX_create();
    const EVP_MD *mdal = EVP_sha256();
    EVP_DigestInit(ctx, mdal);

    int outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    char buf[BUFSIZE];
    printf("文件长度%d\n", left_size);
    int size;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(args.fd, &readfds);
    // 读取文件
    if (0 <= select(args.fd + 1, &readfds, NULL, &readfds, NULL))
    {
        while(size = recv(args.fd, &buf, BUFSIZE, MSG_DONTWAIT))
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
            EVP_DigestUpdate(filectx, buf, size);
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
    if (0 == check(outfile, md))
    {
        ret = "ERR";
        fprintf(stderr, "接收%s的hash值有误！\n", outfile);
    }
    else
    {
        printf("接收%s文件成功\n", outfile);
    }
    writemsg(args.fd, ret, 4);
    close(outfd);
    return size;
}

void redownload(Arg args, messege msg)
{
    while(0 < readmsg(args.fd, &msg, sizeof(msg)))
    {
        if (END == msg.type)
        {
            break;
        }
        recvfile(args, msg.filename, msg.filesize);
    }
    close(args.fd);
}

void combinefile(char (*md)[70], char *filename, int num)
{
    int outfd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    char buf[BUFSIZE];
    int size;
    for (size_t i = 0; i < num; i++)
    {
        int infd = open(md[i], O_RDONLY);
        while(0 != (size = read(infd, buf, BUFSIZE)))
        {
            write(outfd, buf, size);
        }
        close(infd);
    }
    close(outfd);
}

void download(Arg args, messege msg)
{
    // 初始化sha256上下文
    filectx = EVP_MD_CTX_create();
    const EVP_MD *mdal = EVP_sha256();
    EVP_DigestInit(filectx, mdal);
    // 读取md文件
    char mdfile[30];
    sprintf(mdfile, "md%s.txt", args.ip);
    // 文件分片md数组
    char md[msg.blocknum + 1][70];
    int size = loadmd(args, mdfile, msg.mdlen, md, msg.blocknum + 1);
    if (0 == size)
    {
        perror("对端关闭！\n");
        return;
    }
    else if (-1 == size)
    {
        perror("加载md文件失败！\n");
        return;
    }

    // 初始化
    // 下载剩余分片文件
    for (int i = 0; i < msg.blocknum; ++i)
    {
        size = (i >= msg.blocknum - 1) ? (msg.filesize - i * msg.blocksize) : msg.blocksize;
        size = recvfile(args, md[i], size);
        if (0 == size)
        {
            perror("暂停或者对端关闭！\n");
            EVP_MD_CTX_destroy(filectx);
            return;
        }
    }
    char *ret = "ACK";
    int len = SHA256_DIGEST_LENGTH;
    EVP_DigestFinal(filectx, filemd, &len);
    if (0 == check(md[msg.blocknum], filemd))
    {
        ret = "ERR";
    }
    writemsg(args.fd, ret, 4);
    // 重传
    redownload(args, msg);
    // 合并
    combinefile(md, msg.filename, msg.blocknum);
    EVP_MD_CTX_destroy(filectx);
}

void display_file(char *ip)
{

}

void* choice(void *arg)
{
    Arg args = *(Arg*)arg;
    printf("%s连接成功...\n", args.ip);
    
    // 读命令文件名、大小、md大小、块数量
    messege msg;
    int size = readmsg(args.fd, &msg, sizeof(msg));
    switch(msg.type)
    {
        case DISPLAY:
            display_file(args.ip);
            break;
        case UPLOAD:
            download(args, msg);
            break;
        case REUPLOAD:
            redownload(args, msg);
            break;
        default:
    }

    close(args.fd);
    free(arg);
    printf("关闭连接%s\n", args.ip);
    pthread_exit(EXIT_SUCCESS);

}

int main(int argc, char const *argv[])
{
    // 创建套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    // 填写地址
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8889);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    // 绑定地址
    int len = sizeof(server_addr);
    if (-1 == bind(fd, (struct sockaddr*)&server_addr, len))
    {
        perror("绑定失败\n");
        return -1;
    }
    // 忽略子进程相关的信号
    signal(SIGCHLD, SIG_IGN);
    // 监听
    listen(fd, 10);
    // 接收
    while(1)
    {
        Arg *arg = malloc(sizeof(Arg));
        if (-1 != (arg->fd = accept(fd, (struct sockaddr*)&client_addr, &len)))
        {
            // 使用线程做并发
            pthread_t p;
            inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, arg->ip, 16);
            if (pthread_create(&p, NULL, choice, arg))
            {
                perror("创建任务失败！\n");
                continue;
            }
        }
    }
    // 发送
    // 关闭
    close(fd);
    return 0;
}