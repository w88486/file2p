#include "FileCpServer.h"
#include "../kernel/Myins.h"
#include "../kernel/typedef.h"
#include "../kernel/tool.h"
#include <cmath>
#include <sys/sendfile.h>

MyinsMsg* FileCpServer::InternelHandle(MyinsMsg* _input)
{
    GET_REF_FROM_MSG(ByteMsg, byte, *_input);
    auto msg = new CmdMsg(byte.content);
    switch(msg->type)
    {
        case CmdMsg::DISPLAY:
            m_out = new DisplayFile(arg);
            break;
        case CmdMsg::UPLOAD:
            m_out = new DownLoad(arg);
            break;
        case CmdMsg::REUPLOAD:
            m_out = new RedownLoad(arg);
            break;
        case CmdMsg::DONWLOAD:
            m_out = new UpLoad(arg);
            break;
        default:
    }
    return msg;
}
Mhandler* FileCpServer::GetNext(MyinsMsg* _input)
{
    return m_out;
}


MyinsMsg* DownLoad::InternelHandle(MyinsMsg* _input)
{
    GET_REF_FROM_MSG(CmdMsg, msg, *_input);
    // 初始化sha256上下文
    filectx = EVP_MD_CTX_create();
    const EVP_MD *mdal = EVP_sha256();
    EVP_DigestInit(filectx, mdal);
    // 读取md文件
    char mdfile[30];
    sprintf(mdfile, "md%s.txt", arg.ip);
    // 文件分片md数组
    arg.m_channel->md.resize(msg.blocknum + 1);
    int size = loadmd(arg.fd, mdfile, msg.mdlen, arg.m_channel->md, msg.blocknum + 1);
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
        size = recvfile(arg.fd, arg.m_channel->md[i], size, filectx);
        if (0 == size)
        {
            perror("暂停或者对端关闭！\n");
            EVP_MD_CTX_destroy(filectx);
            return;
        }
    }
    char *ret = "ACK";
    unsigned char filemd[SHA256_DIGEST_LENGTH];
    unsigned int len = SHA256_DIGEST_LENGTH;
    EVP_DigestFinal(filectx, filemd, &len);
    if (0 == check(arg.m_channel->md[msg.blocknum].c_str(), filemd))
    {
        ret = "ERR";
    }
    writemsg(arg.fd, ret, 4);
    EVP_MD_CTX_destroy(filectx);
    return &msg;
}
Mhandler* DownLoad::GetNext(MyinsMsg* _input)
{
    return m_out;
}

// 重传
MyinsMsg* RedownLoad::InternelHandle(MyinsMsg* _input)
{
    CmdMsg buf;
    // 可能有问题
    while(0 < readmsg(arg.fd, &buf, sizeof(buf)))
    {
        if (CmdMsg::END == buf.type)
        {
            break;
        }
        recvfile(arg.fd, (char*)buf.filename, buf.filesize, NULL);
    }
    return _input;
}
Mhandler* RedownLoad::GetNext(MyinsMsg* _input)
{
    return m_out;
}

// 合并
MyinsMsg* CombineFile::InternelHandle(MyinsMsg* _input)
{
    GET_REF_FROM_MSG(CmdMsg, msg, *_input);
    combinefile(arg.m_channel->md, (char*)msg.filename, msg.blocknum);
}
Mhandler* CombineFile::GetNext(MyinsMsg* _input)
{
    return m_out;
}

void UpLoad::writehash(int fd, string filename, int mdtxt)
{
    lseek(fd, 0L, SEEK_SET);
    const EVP_MD *mdal = EVP_sha256();
    EVP_MD_CTX *ctx = EVP_MD_CTX_create();
    if (NULL == ctx)
    {
        fprintf(stderr, "%s初始化上下文失败！\n", filename.c_str());
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
    arg.m_channel->md.push_back(smd);
    smd[strlen(smd) + 1] = '\0';
    smd[strlen(smd)] = '\n';
    write(mdtxt, smd, strlen(smd));
    EVP_MD_CTX_destroy(ctx);
}

void UpLoad::open_new_block(int *out, string infile, int id, char *filename)
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

CmdMsg UpLoad::splitfile(string infile, int *mdtxt, long blocksize)
{
    char mdfilename[infile.length() + 10];
    sprintf(mdfilename, "%s.md.txt", infile.c_str());
    *mdtxt = open(mdfilename, O_CREAT | O_RDWR | O_TRUNC, 0777);
    // 返回值
    CmdMsg msg;
    msg.type = CmdMsg::UPLOAD;
    msg.blocksize = blocksize;
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
    char filename[infile.length() + 10];
    // 文件名长度
    int flen;

    // 每次读的块大小
    int size = 0;
    // 片数量
    long int blocknum;

    // 缓冲区大小
    char buf[BUFSIZE];

    in = open(infile.c_str(), O_RDWR);
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
            rename(filename, arg.m_channel->md[id].c_str());
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
        rename(filename, arg.m_channel->md[id].c_str());
        printf("block %d 已经输出完毕\n", id);
    }
    writehash(in, infile, *mdtxt);
    msg.blocknum = blocknum;
    msg.filesize = lseek(in, 0L, SEEK_END);
    msg.mdlen = lseek(*mdtxt, 0L, SEEK_END);
    msg.blocksize = blocksize;
    lseek(*mdtxt, 0L, SEEK_SET);
    strncpy((char*)msg.filename, infile.c_str(), infile.length() + 1);
    close(out);
    close(in);
    return msg;
}

void UpLoad::reupload(long int blocksize)
{
    char ret[6];
    // 重传传输失败的文件
    int i;
    CmdMsg msg;
    msg.type = CmdMsg::SPLITFILE;
    while (-1 != (i = find_nnull(arg.m_channel->md)))
    {
        // 填充消息
        int fd = open(arg.m_channel->md[i].c_str(), O_RDONLY);
        if (-1 == fd)
        {
            fprintf(stderr, "缺失文件：%s\n", arg.m_channel->md[i].c_str());
            return;
        }
        msg.filesize = lseek(fd, 0L, SEEK_END);
        strncpy((char *)msg.filename, arg.m_channel->md[i].c_str(), arg.m_channel->md[i].length() + 1);
        if (0 == writemsg(arg.fd, &msg, sizeof(msg)))
        {
            // 超时了
            break;
        }
        sendfile(arg.fd, fd, 0L, blocksize);
        readmsg(arg.fd, ret, 6);
        if (0 == strncmp(ret, "ACK", 3))
        {
            printf("重发送%s成功\n", arg.m_channel->md[i]);
            arg.m_channel->md[i] = "";
        }
        else
        {
            fprintf(stderr, "重发送%s失败！\n", arg.m_channel->md[i]);
        }
    }
    msg.type = CmdMsg::END;
    writemsg(arg.fd, &msg, sizeof(msg));
}

int UpLoad::checkupload(string filename)
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
        arg.m_channel->md.emplace_back(res->element[i]->str);
    }
    freeReplyObject(res);
    redisFree(ctx);
    return res->elements;
}
MyinsMsg* UpLoad::InternelHandle(MyinsMsg* _input)
{
    GET_REF_FROM_MSG(CmdMsg, msg, *_input);
    // 在redis中检查是否暂停或终止后的请求，需要重传 run 139.159.195.171 redis-stable.tar.gz
    if (checkupload((char*)msg.filename) > 0)
    {
        int ch;
        msg.type = CmdMsg::DONWLOAD;
        writemsg(arg.fd, &msg, sizeof(msg));
        reupload(msg.blocksize);
        return;
    }
    // 分片
    int mdtxt;
    msg = splitfile((char*)msg.filename, &mdtxt, msg.blocksize);
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
        int infd = open(arg.m_channel->md[i].c_str(), O_RDONLY);
        size_t ssize = sendfile(arg.fd, infd, 0L, msg.blocksize);
        // 接收结果判断是否成功
        readmsg(arg.fd, rett, 4);
        if (0 == strncmp(rett, "ACK", 3))
        {
            printT(i + 1, msg.blocknum);
            printf("发送%s成功\n", arg.m_channel->md[i]);
            arg.m_channel->md[i] = "";
        }
        else
        {
            // 输出进度
            printT(i + 1, msg.blocknum);
            fprintf(stderr, "发送%s出错", arg.m_channel->md[i]);
        }
        close(infd);
    }
    readmsg(arg.fd, rett, 4);
    if (0 != strncmp(rett, "ACK", 3))
    {
        fprintf(stderr, "文件%s总hash值有误！\n", msg.filename);
    }
    arg.m_channel->md[msg.blocknum] = "";
    // 重传
    reupload(msg.blocksize);
}
Mhandler* UpLoad::GetNext(MyinsMsg* _input)
{
    return m_out;
}

MyinsMsg* DisplayFile::InternelHandle(MyinsMsg* _input)
{

}
Mhandler* DisplayFile::GetNext(MyinsMsg* _input)
{
    return m_out;
}
