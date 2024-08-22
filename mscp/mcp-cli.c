#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <hiredis/hiredis.h>
#include <errno.h>
#include <signal.h>
#include "arr.h"
#define BUFSIZE 1024
#define FILENAME_SIZE 232
#define PORT 8889
#define REDIS_PORT 6379
#define MAX_BLOCK_NUM 1024
#define TIMEOUT 1000
struct list *md;

#define BUFSIZE 1024
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
void writehash(int fd, char *filename, int mdtxt)
{
    lseek(fd, 0L, SEEK_SET);
    const EVP_MD *mdal = EVP_sha256();
    EVP_MD_CTX *ctx = EVP_MD_CTX_create();
    if (NULL == ctx)
    {
        fprintf(stderr, "%s初始化上下文失败！\n", filename);
        exit(-1);
    }
    EVP_DigestInit(ctx, mdal);

    char buf[BUFSIZE];
    size_t size;
    while((size = read(fd, buf, BUFSIZE)) != 0)
    {
        EVP_DigestUpdate(ctx, buf, size);
    }

    unsigned char mmd[SHA256_DIGEST_LENGTH];
    char smd[70];
    memset(smd, 0, sizeof(smd));
    unsigned int len = SHA256_DIGEST_LENGTH;
    EVP_DigestFinal(ctx, mmd, &len);

    xto_char(mmd, smd, SHA256_DIGEST_LENGTH);
    add(md, smd);
    smd[strlen(smd) + 1] = '\0';
    smd[strlen(smd)] = '\n';
    write(mdtxt, smd, strlen(smd));
    EVP_MD_CTX_destroy(ctx);
}

void open_new_block(int *out, char *infile, int id, char *filename)
{
    if (-1 != *out)
    {
        close(*out);
    }
    int flen = sprintf(filename, "%s%d", infile, id);
    filename[flen] = '\0';
    *out = open(filename, O_RDWR | O_CREAT | O_CREAT, 0777);
    if (-1 == *out)
    {
        fprintf(stderr, "打开文件%s失败！\n", filename);
        exit(-1);
    }
}

messege splitfile(char *infile, int *mdtxt, long blocksize)
{
    char mdfilename[strlen(infile) + 10];
    sprintf(mdfilename, "%s.md.txt", infile);
    *mdtxt = open(mdfilename, O_CREAT | O_RDWR | O_TRUNC, 0777);
    // 返回值
    messege msg = {UPLOAD, blocksize = blocksize};
    // 当前输出文件要输出的大小
    int left_byte = 0;
    // 缓冲区当前读的位置 缓冲区当前容纳的字节数
    int buf_pos = 0;
    
    // 输入输出流
    int in;
    int out = -1;
    // 片编号
    static int id = 0;

    // 输出文件名
    char filename[strlen(infile) + 10];
    // 文件名长度
    int flen;

    // 每次读的块大小
    int size = 0;
    // 片数量
    long int blocknum;

    // 缓冲区大小
    char buf[BUFSIZE];

    in = open(infile, O_RDWR);
    if (-1 == in)
    {
        fprintf(stderr, "文件打开错误\n");
        return msg;
    }
    // 获取文件大小
    long int filesize = lseek(in, 0L, SEEK_END);
    lseek(in, 0L, SEEK_SET);
    blocknum = ceil((double)filesize / blocksize);
    printf("文件大小：%ld 分片数：%ld\n", filesize, blocknum);

    // 打开第一个片
    open_new_block(&out, infile, id, filename);
    left_byte = blocksize;
    while((size = read(in, buf + buf_pos, BUFSIZE)) != 0)
    {
        buf_pos += size;
        // 第id个块写入完毕
        if (buf_pos >= left_byte)
        {
            // 写入
            write(out, buf, left_byte);
            // 
            writehash(out, filename, *mdtxt);
            rename(filename, get(md, id));
            printf("block %d 已经输出完毕\n", id);
            ++id;
            // 将剩下的字节存回缓冲区
            buf_pos -= left_byte;
            memmove(buf, buf + left_byte, buf_pos);
            // 复位剩余一个片的字节数
            left_byte = blocksize;
            // 打开新的片
            open_new_block(&out, infile, id, filename);
        }
        else
        {
            size = size < buf_pos ? buf_pos : size;
            // 正常写
            write(out, buf, size);
            left_byte -= size;
            buf_pos = 0;
        }
    }
    // 最后一个块没写完
    if (left_byte > 0)
    {
        write(out, buf, buf_pos);
        writehash(out, filename, *mdtxt);
        rename(filename, get(md, id));
        printf("block %d 已经输出完毕\n", id);
    }
    writehash(in, infile, *mdtxt);
    msg.blocknum = blocknum;
    msg.filesize = lseek(in, 0L, SEEK_END);
    msg.mdlen = lseek(*mdtxt, 0L, SEEK_END);
    msg.blocksize = blocksize;
    lseek(*mdtxt, 0L, SEEK_SET);
    strncpy(msg.filename, infile, strlen(infile) + 1);
    close(out);
    close(in);
    return msg;
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

void reupload(int fd, int blocksize)
{
    char ret[6];
    // 重传传输失败的文件
    int i;
    messege msg;
    msg.type = SPLITFILE;
    while(-1 != (i = find_nnull(md)))
    {
        char *file = get(md, i);
        // 填充消息
        msg.filesize = lseek(fd, 0L, SEEK_END);
        strncpy(msg.filename, file, strlen(file) + 1);

        if (0 == writemsg(fd, &msg, sizeof(msg)))
        {
            // 超时了
            break;
        }
        int fds = open(file, O_RDONLY);
        sendfile(fd, fds, 0L, blocksize);
        readmsg(fd, ret, 6);
        if (0 == strncmp(ret, "ACK", 3))
        {
            printf("重发送%s成功\n", file);
            delete(md, i);
        }
        else
        {
            fprintf(stderr, "重发送%s失败！\n", file);
        }
        close(fds);
    }
    msg.type = END;
    writemsg(fd, &msg, sizeof(msg));
}

int checkupload(const char* ip, const char* filename)
{
    // run 139.159.195.171 redis-stable.tar.gz
    redisContext *ctx = redisConnect("127.0.0.1", REDIS_PORT);
    if (NULL == ctx)
    {
        perror("打开redis失败！\n");
        exit(EXIT_FAILURE);
    }
    redisReply *res;
    res = redisCommand(ctx, "smembers %s%s", ip, filename);
    for (int i = 0; i < res->elements; i++)
    {
        add(md, res->element[i]->str);
    }
    freeReplyObject(res);
    redisFree(ctx);
    return res->elements;
}
int main(int argc, char const *argv[])
{
    messege msg;
    if (argc < 3)
    {
        fprintf(stderr, "使用client ip file [blocksize]\n");
        return -1;
    }
    // 判断转换是否成功的标志
    char *ret;
    // 片大小 nk
    long int blocksize = ((NULL == argv[3]) ? 1024 : strtol(argv[3], &ret, 10)) * 1024;
    if (argv[3] && argv[3] == ret)
    {
        fprintf(stderr, "参数 blocksize 格式错误\n");
        return -1;
    }

    // 创建套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    // 填写地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (0 == inet_pton(AF_INET, argv[1], &server_addr.sin_addr.s_addr))
    {
        perror("地址转换失败！\n");
        return -1;
    }

    int len = sizeof(server_addr);
    // 连接
    if (-1 == connect(fd, (struct sockaddr*)&server_addr, len))
    {
        fprintf(stderr, "连接失败\n");
        return -1;
    }

    md = malloc(sizeof(struct list));
    init(md);
    // 在redis中检查是否暂停或终止后的请求，需要重传 run 139.159.195.171 redis-stable.tar.gz
    if (checkupload(argv[1], argv[2]))
    {
        msg.blocksize = blocksize;
        strncpy(msg.filename, argv[2], strlen(argv[2]) + 1);
        msg.type = REDONWLOAD;
        writemsg(fd, &msg, sizeof(msg));
        reupload(fd, blocksize);
        close(fd);
        exit(EXIT_SUCCESS);
    }
    // 分片
    int mdtxt;
    msg = splitfile(argv[2], &mdtxt, blocksize);
    // 发送命令
    writemsg(fd, &msg, sizeof(msg));
    // 发送摘要
    sendfile(fd, mdtxt, 0L, msg.mdlen);
    char rett[6];
    readmsg(fd, rett, 4);
    if (0 == strncmp(rett, "ACK", 3))
    {
        printf("发送摘要成功\n");
    }
    else
    {
        fprintf(stderr, "发送摘要失败\n");
        close(fd);
        close(mdtxt);
        fini(md);
        exit(EXIT_FAILURE);
    }
    // 发送片 run 139.159.195.171 redis-stable.tar.gz
    for (int i = 0; i < msg.blocknum; ++i)
    {
        int infd = open(get(md, i), O_RDONLY);
        size_t ssize = sendfile(fd, infd, 0L, msg.blocksize);
        // 接收结果判断是否成功
        readmsg(fd, rett, 4);
        if (0 == strncmp(rett, "ACK", 3))
        {
            printT(i + 1, msg.blocknum);
            printf("发送%s成功\n", get(md, i));
            delete(md, i);
        }
        else
        {
            // 输出进度
            printT(i + 1, msg.blocknum);
            fprintf(stderr, "发送%s出错", get(md, i));
        }
        close(infd);
    }
    readmsg(fd, rett, 4);
    if (0 != strncmp(rett, "ACK", 3))
    {
        fprintf(stderr, "文件%s总hash值有误！\n", msg.filename);
    }
    delete(md, msg.blocknum);
    // 重传
    reupload(fd, blocksize);
    // 关闭
    close(fd);
    close(mdtxt);
    fini(md);
    return 0;
}
