#include "FileCpCli.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "typedef.h"
#include <fcntl.h>
#include <unistd.h>
#include <hiredis/hiredis.h>
#include <cstring>
#include <openssl/evp.h>
#include <sys/sendfile.h>
void FileClientUI::redisplay()
{
    system("clear");
    if (-1 != arg.fd)
    {
        printf("当前连接服务器地址：\n");
        cout << arg.ip << endl;
    }
    printf("请输入选项：\n");
    printf("-------------------------------\n"
           "\t1. 上传文件\n"
           "\t2. 下载文件\n"
           "\t3. 显示文件\n"
           "\t4. 显示当前任务\n"
           "\t5. 连接服务器\n"
           "\t6. 断开当前连接\n"
           "\tq. 退出程序\n"
           "-------------------------------\n");
}
void FileClientUI::process()
{

    int ch;
    for (;;)
    {
        redisplay();
        ch = getchar();
        getchar();
        switch (ch)
        {
        case '1':
            upload();
            break;
        case '2':
            download();
            break;
        case '3':
            display_file();
            break;
        case '4':
            display_task();
            break;
        case '5':
            connectToServer();
            break;
        case '6':
            disconnect();
            break;
        default:
            fprintf(stderr, "输入错误！");
        }
        printf("按'q'键返回...");
        while (getchar() != 'q')
            ;
        getchar();
    }
}

void FileClientUI::connectToServer()
{
    // ip地址
    string ip;
    printf("请输入服务器地址：\n");
    cin >> ip;

    // 创建套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    // 填写地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (0 == inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr.s_addr))
    {
        perror("地址转换失败！\n");
        close(fd);
        return;
    }
    int len = sizeof(server_addr);
    // 连接
    if (-1 == connect(fd, (struct sockaddr *)&server_addr, len))
    {
        fprintf(stderr, "连接失败\n");
        close(fd);
        return;
    }
    arg.fd = fd;
    arg.ip = ip;
}

void FileClientUI::disconnect()
{
    if (-1 == arg.fd)
    {
        fprintf(stderr, "未连接服务器\n");
        return;
    }
    close(arg.fd);
    arg.fd = -1;
}

void FileClientUI::reupload(long int blocksize)
{
    char ret[6];
    // 重传传输失败的文件
    int i;
    messege msg;
    msg.type = messege::SPLITFILE;
    while (-1 != (i = find_nnull(&md)))
    {
        // 填充消息
        int fd = open(md[i].c_str(), O_RDONLY);
        if (-1 == fd)
        {
            fprintf(stderr, "缺失文件：%s\n", md[i].c_str());
            return;
        }
        msg.filesize = lseek(fd, 0L, SEEK_END);
        strncpy((char *)msg.filename, md[i].c_str(), md[i].length() + 1);
        if (0 == writemsg(arg.fd, &msg, sizeof(msg)))
        {
            // 超时了
            break;
        }
        sendfile(arg.fd, fd, 0L, blocksize);
        readmsg(arg.fd, ret, 6);
        if (0 == strncmp(ret, "ACK", 3))
        {
            printf("重发送%s成功\n", md[i]);
            md[i] = "";
        }
        else
        {
            fprintf(stderr, "重发送%s失败！\n", md[i]);
        }
    }
    msg.type = messege::END;
    writemsg(arg.fd, &msg, sizeof(msg));
}

int FileClientUI::checkupload(string filename)
{
    // run 139.159.195.171 redis-stable.tar.gz
    redisContext *ctx = redisConnect("127.0.0.1", 6379);
    if (NULL == ctx)
    {
        perror("打开redis失败！\n");
        return -1;
    }
    redisReply *res;
    res = (redisReply *)redisCommand(ctx, "smembers %s%s", arg.ip, filename.c_str());
    for (int i = 0; i < res->elements; i++)
    {
        md.emplace_back(res->element[i]->str);
    }
    freeReplyObject(res);
    redisFree(ctx);
    return res->elements;
}

void FileClientUI::upload()
{
    messege msg;
    string file;
    long int blocksize = 256;
    int blocknum = 1;
    printf("请输入文件名：\n");
    cin >> file;
    printf("请输入分片大小："
           "\n分片范围\t(0 ~ filesize)\n"
           "\n不分片输入0\n");
    cin >> blocksize;
    if (0 != blocksize)
    {
        blocknum = 1;
    }
    blocksize *= 1024;
    // 在redis中检查是否暂停或终止后的请求，需要重传 run 139.159.195.171 redis-stable.tar.gz
    if (checkupload(file) > 0)
    {
        int ch;
        printf("是否继续任务(y/n, 输入n重新上传):\n");
        getchar();
        ch = getchar();
        if ('y' == ch)
        {
            msg.blocksize = blocksize;
            strncpy((char *)msg.filename, file.c_str(), file.length() + 1);
            msg.type = messege::DONWLOAD;
            writemsg(arg.fd, &msg, sizeof(msg));
            reupload(blocksize);
            return;
        }
    }
    // 分片
    int mdtxt;
    msg = splitfile(file.c_str(), &mdtxt, blocksize);
    // 发送命令
    writemsg(arg.fd, &msg, sizeof(msg));
    // 发送摘要
    sendfile(arg.fd, mdtxt, 0L, msg.mdlen);
    char rett[6];
    readmsg(arg.fd, rett, 4);
    if (0 == strncmp(rett, "ACK", 3))
    {
        printf("发送摘要成功\n");
    }
    else
    {
        fprintf(stderr, "发送摘要失败\n");
        arg.fd = -1;
        close(mdtxt);
        return;
    }
    // 发送片 run 139.159.195.171 redis-stable.tar.gz
    for (int i = 0; i < msg.blocknum; ++i)
    {
        int infd = open(md[i].c_str(), O_RDONLY);
        size_t ssize = sendfile(arg.fd, infd, 0L, msg.blocksize);
        // 接收结果判断是否成功
        readmsg(arg.fd, rett, 4);
        if (0 == strncmp(rett, "ACK", 3))
        {
            printT(i + 1, msg.blocknum);
            printf("发送%s成功\n", md[i]);
            md[i] = "";
        }
        else
        {
            // 输出进度
            printT(i + 1, msg.blocknum);
            fprintf(stderr, "发送%s出错", md[i]);
        }
        close(infd);
    }
    readmsg(arg.fd, rett, 4);
    if (0 != strncmp(rett, "ACK", 3))
    {
        fprintf(stderr, "文件%s总hash值有误！\n", msg.filename);
    }
    md[msg.blocknum] = "";
    // 重传
    reupload(blocksize);
    // 关闭
    close(mdtxt);
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

messege splitfile(const char *infile, int *mdtxt, long blocksize)
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