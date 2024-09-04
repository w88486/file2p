#include "FileCpServer.h"
#include "../kernel/Myins.h"
#include "../kernel/typedef.h"
#include "../kernel/tool.h"
#include <cmath>
#include <sys/sendfile.h>

extern int level[4];

MyinsMsg *FileCpServer::InternelHandle(MyinsMsg *_input)
{
    GET_REF_FROM_MSG(ByteMsg, byte, *_input);
    auto msg = new DeviceMsg(byte);
    switch (msg->ctx->type)
    {
    case MQTT::SUBSCRIBE:
        m_out = new UpLoad(arg);
        break;
    case MQTT::SUBRECV:
        m_out = new ReUpLoad(arg);
        break;
    default:
        break;
    }
    return msg;
}

Mhandler *FileCpServer::GetNext(MyinsMsg *_input)
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
    while ((size = read(fd, buf, BUFSIZE)) != 0)
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
    int flen = sprintf(filename, "%s%d", infile.c_str(), id);
    filename[flen] = '\0';
    *out = open(filename, O_RDWR | O_CREAT | O_CREAT, 0777);
    if (-1 == *out)
    {
        fprintf(stderr, "打开文件%s失败！\n", filename);
        exit(-1);
    }
}

DeviceMsg *UpLoad::splitfile(string infile, int *mdtxt, long blocksize)
{
    auto ctx = (MQTT *)malloc(sizeof(MQTT));
    // 构建的返回值
    MQTT_Init(ctx, MQTT::PUBLISH, 128);

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
    // 每次读的块大小
    int size = 0;
    // 片数量
    long int blocknum;
    // 缓冲区大小
    char buf[BUFSIZE];

    // 打开摘要
    char mdfilename[infile.length() + 10];
    sprintf(mdfilename, "%s.md.txt", infile.c_str());
    *mdtxt = open(mdfilename, O_CREAT | O_RDWR | O_TRUNC, 0777);
    // 打开要传输的文件
    in = open(infile.c_str(), O_RDWR);
    if (-1 == in)
    {
        fprintf(stderr, "文件打开错误\n");
        MQTT_FillType(ctx, MQTT::ERROR);
        return new DeviceMsg(ctx);
    }
    // 获取文件大小
    long int filesize = lseek(in, 0L, SEEK_END);
    lseek(in, 0L, SEEK_SET);
    blocknum = ceil((double)filesize / blocksize);
    printf("文件大小：%ld 分片数：%ld\n", filesize, blocknum);

    // 打开第一个片
    open_new_block(&out, infile, id, filename);
    left_byte = blocksize;
    while ((size = read(in, buf + buf_pos, BUFSIZE)) != 0)
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

    MQTT_Fill(ctx, "filename", infile.c_str());
    char tmp[20];
    sprintf(tmp, "%ld", blocksize);
    MQTT_Fill(ctx, "blocksize", tmp);
    // 填充摘要长度
    int tmpnum = lseek(*mdtxt, 0L, SEEK_END);
    sprintf(tmp, "%d", tmpnum);
    MQTT_Fill(ctx, "mdlen", tmp);
    // 填充剩余返回值
    sprintf(tmp, "%ld", blocknum);
    MQTT_Fill(ctx, "blocknum", tmp);
    sprintf(tmp, "%ld", filesize);
    MQTT_Fill(ctx, "filesize", tmp);
    auto msg = new DeviceMsg(ctx);
    msg->blocknum = blocknum;
    msg->blocksize = blocksize;
    msg->filesize = filesize;
    msg->mdlen = tmpnum;

    lseek(*mdtxt, 0L, SEEK_SET);
    close(out);
    close(in);
    return msg;
}
MyinsMsg *UpLoad::InternelHandle(MyinsMsg *_input)
{
    GET_REF_FROM_MSG(DeviceMsg, msg, *_input);
    char filename[512];
    MQTT_GetVHeader(msg.ctx, "filename", filename, sizeof(filename));

    msg.blocksize = DEFBLOCKSIZE;

    // 分片
    int mdtxt;
    msg = *splitfile(filename, &mdtxt, msg.blocksize);
    // 发送相关信息
    writemsg(arg.fd, msg.ctx->buf, msg.ctx->size);
    if (MQTT::ERROR == msg.ctx->type)
    {
        return NULL;
    }
    // 发送摘要
    size_t ssize = sendfile(arg.fd, mdtxt, 0L, msg.mdlen);
    // 发送片 run 139.159.195.171 redis-stable.tar.gz
    for (int i = 0; i < msg.blocknum; ++i)
    {
        // 问题
        int infd = open(arg.m_channel->md[i].c_str(), O_RDONLY);
        ssize = sendfile(arg.fd, infd, 0L, msg.blocksize);
        close(infd);
    }
    shutdown(arg.fd, SHUT_WR);
    return NULL;
}
Mhandler *UpLoad::GetNext(MyinsMsg *_input)
{
    return NULL;
}

MyinsMsg *ReUpLoad::InternelHandle(MyinsMsg *_input)
{
    GET_REF_FROM_MSG(DeviceMsg, msg, *_input);
    char tmp[512];
    MQTT_GetVHeader(msg.ctx, "filename", tmp, sizeof(tmp));
    int mdtxt = open(tmp, O_RDONLY);
    if (-1 == mdtxt)
    {
        MQTT_FillType(msg.ctx, MQTT::ERROR);
        // 发送相关信息
        writemsg(arg.fd, msg.ctx->buf, 5);
        return NULL;
    }

    int filesize = lseek(mdtxt, 0L, SEEK_END);
    lseek(mdtxt, 0L, SEEK_SET);

    // 填充长度
    msg.ctx->left_byte = filesize;
    for (int i = 0; i < 4; ++i)
    {
        if (filesize > 0x7f)
        {
            msg.ctx->buf[i + 1] = (filesize & 0x7f) | 0x80;
            filesize >>= 7;
        }
        else
        {
            msg.ctx->buf[i + 1] = (filesize & 0x7f);
            break;
        }
    }
    MQTT_FillType(msg.ctx, MQTT::SUBACK);

    // 发送相关信息
    writemsg(arg.fd, msg.ctx->buf, 5);
    // 发送文件
    sendfile(arg.fd, mdtxt, 0L, msg.ctx->left_byte);
    return NULL;
}
Mhandler *ReUpLoad::GetNext(MyinsMsg *_input)
{
    return NULL;
}