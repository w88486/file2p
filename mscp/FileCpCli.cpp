#include "FileCpCli.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/sendfile.h>
#include <cstdio>
#include <cstdlib>
#include "../kernel/tool.h"
#include "../kernel/MQTT.h"
#include "../kernel/MyinsKernel.h"
void FileClientUI::redisplay()
{
    system("clear");
    if (-1 != fd)
    {
        printf("当前连接服务器地址：\n");
        cout << ip << endl;
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
        case 'q':
            MyinsKernel::GetInstance()->SetKernelStop();
            return;
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
    printf("请输入中央服务器地址：\n");
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
    this->fd = fd;
    this->ip = ip;
}

void FileClientUI::disconnect()
{
    if (-1 == fd)
    {
        fprintf(stderr, "未连接服务器\n");
        return;
    }
    close(fd);
    fd = -1;
}

void FileClientUI::upload()
{
}

void FileClientUI::download()
{
    string file;
    printf("请输入文件名：\n");
    cin >> file;
    // 初始化sha256上下文
    filectx = EVP_MD_CTX_create();
    const EVP_MD *mdal = EVP_sha256();
    EVP_DigestInit(filectx, mdal);

    //  搜索文件
    auto msg = SearchFile(file);
    if (MQTT::ERROR == msg->ctx->type)
    {
        return;
    }

    if (msg->ctx->type == MQTT::SUBRECV)
    {
        redownload(*msg);
        return;
    }

    // 发送命令
    writemsg(msg->fd, msg->ctx->buf, msg->ctx->size);
    // 读取相关信息
    char buf[BUFSIZE];
    if (readmsg(msg->fd, buf, 5) <= 0)
    {
        fprintf(stderr, "发送报文出错！\n");
        return;
    }
    MQTT_Convert(msg->ctx, buf, sizeof(buf));
    if (msg->ctx->type == MQTT::ERROR)
    {
        fprintf(stderr, "文件不存在！\n");
        return;
    }
    if (0 >= readmsg(msg->fd, buf + 5, msg->ctx->left_byte - 4 + msg->ctx->left_byte_len))
    {
        fprintf(stderr, "发送报文出错！\n");
        return;
    }
    MQTT_Convert(msg->ctx, buf, sizeof(buf));

    // 读取分片数
    char tmp[20];
    MQTT_GetVHeader(msg->ctx, "blocknum", tmp, sizeof(tmp));
    msg->blocknum = atoi(tmp);
    msg->filenum = msg->blocknum;
    // 读取md文件长度
    MQTT_GetVHeader(msg->ctx, "mdlen", tmp, sizeof(tmp));
    msg->mdlen = atoi(tmp);
    // 读取文件长度
    MQTT_GetVHeader(msg->ctx, "filesize", tmp, sizeof(tmp));
    msg->filesize = atoi(tmp);
    // 读取分片大小
    MQTT_GetVHeader(msg->ctx, "blocksize", tmp, sizeof(tmp));
    msg->blocksize = atoi(tmp);

    // 读取md文件
    char mdfile[30];
    sprintf(mdfile, "md%s%s.txt", msg->filename.c_str(), msg->ip.c_str());
    int size = loadmd(*msg, mdfile, msg->mdlen);
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
    // 下载剩余分片文件
    for (int i = 0; i < msg->blocknum; ++i)
    {
        size = (i >= msg->blocknum - 1) ? (msg->filesize - i * msg->blocksize) : msg->blocksize;
        size = recvfile(*msg, i, size, filectx);
        // 输出进度
        printT(i + 1, msg->blocknum, md_flag);
        if (0 == size)
        {
            perror("对端关闭！\n");
            EVP_MD_CTX_destroy(filectx);
            storemd(*msg);
            return;
        }
    }
    unsigned char filemd[SHA256_DIGEST_LENGTH];
    unsigned int len = SHA256_DIGEST_LENGTH;
    EVP_DigestFinal(filectx, filemd, &len);
    if (0 == check(md[msg->blocknum].c_str(), filemd))
    {
        storemd(*msg);
        return;
    }
    // 重传
    redownload(*msg);
    EVP_MD_CTX_destroy(filectx);
    delete msg;
}

void FileClientUI::redownload(DeviceMsg &msg)
{
    bool flag = false;
    // 读取文件 
    char buf[BUFSIZE];
    int i;
    for (i = 0; i < msg.blocknum; ++i)
    {
        if (md_flag[i])
        {
            MQTT_FlushBuf(msg.ctx);
            MQTT_FillType(msg.ctx, MQTT::SUBRECV);
            MQTT_Fill(msg.ctx, "filename", md[i].c_str());
            writemsg(msg.fd, msg.ctx->buf, msg.ctx->size);
            readmsg(msg.fd, buf, 5);
            // 解析剩余长度
            msg.ctx->left_byte = 0;
            for (int j = 0; j < 4; ++j)
            {
                msg.ctx->left_byte |= (buf[j + 1] & 0x7f) << (7 * j);
                // 如果当前字节的最高位为1，则后面的字节也是表示剩余字节的一部分
                if (0 == (buf[j + 1] & 0x80))
                {
                    break;
                }
            }
            if ((buf[0] & 0x0f) == MQTT::ERROR)
            {
                fprintf(stderr, "文件不存在！\n");
                storemd(msg);
                return;
            }
            recvfile(msg, i, msg.ctx->left_byte, NULL);
        }
        if (md_flag[i])
        {
            flag = true;
        }
    }
    if (flag)
    {
        storemd(msg);
    }
    else
    {
        removemd(msg);
        combinefile(msg);
    }
}

//
void FileClientUI::display_file()
{
}
//
void FileClientUI::display_task()
{
}

DeviceMsg* FileClientUI::SearchFile(std::string file)
{
    md.clear();
    md_flag.reset();
    md_flag.set();
    // 读取文件相关信息
    MQTT *ctx = (MQTT *)malloc(sizeof(MQTT));
    MQTT_Init(ctx, MQTT::SUBSCRIBE, 128);
    auto msg = new DeviceMsg(ctx);
    msg->filename = file;
    // 问题 redis-stable.tar.gz
    // 在redis中检查是否暂停或终止后的请求，需要重传 run 139.159.195.171
    redisContext *redisctx = redisConnect("127.0.0.1", 6379);
    if (NULL == redisctx)
    {
        MQTT_FillType(msg->ctx, MQTT::ERROR);
        return msg;
    }
    redisReply *res;
    res = (redisReply *)redisCommand(redisctx, "smembers file:%s", file.c_str());
    // 返回 ip md0 md1 ...
    if (0 != res->elements)
    {
        msg->blocknum = 0;
        for (int i = 0; i < res->elements; i++)
        {
            if (0 == strncmp(res->element[i]->str, "md:", 3))
            {
                md.emplace_back(res->element[i]->str + 3);
                ++msg->blocknum;
            }
            else if (0 == strncmp(res->element[i]->str, "host:", 5))
            {
                msg->ip = std::string(res->element[i]->str + 5, res->element[i]->len - 5);
            }
            else if (0 == strncmp(res->element[i]->str, "blocknum:", 9))
            {
                msg->filenum = atoi(res->element[i]->str + 9);
            }
        }
        MQTT_FillType(msg->ctx, MQTT::SUBRECV);
        msg->fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in server;
        server.sin_port = htons(FILEPORT);
        server.sin_family = AF_INET;
        if (0 == inet_pton(AF_INET, msg->ip.c_str(), &server.sin_addr.s_addr))
        {
            MQTT_FillType(msg->ctx, MQTT::ERROR);
            perror("地址转换失败！\n");
            close(msg->fd);
            return msg;
        }
        if (0 != connect(msg->fd, (struct sockaddr *)&server, sizeof(server)))
        {
            MQTT_FillType(msg->ctx, MQTT::ERROR);
            fprintf(stderr, "连接目标节点失败！\n");
            close(msg->fd);
            return msg;
        }
        freeReplyObject(res);
        redisFree(redisctx);
        return msg;
    }
    freeReplyObject(res);
    redisFree(redisctx);

    // 本地redis查不到记录，则是新文件的下载，先从中央服务器获取文件信息
    MQTT_Fill(msg->ctx, "filename", file.c_str());
    // 发送命令,查找文件所在地 redis-stable.tar.gz
    writemsg(fd, msg->ctx->buf, msg->ctx->size);

    // 读取文件相关信息的报文
    char buf[BUFSIZE];
    int ret = readmsg(fd, buf, 5);
    if (ret <= 0)
    {
        fprintf(stderr, "发送报文出错！\n");
        msg->ctx->type = MQTT::ERROR;
        return msg;
    }
    MQTT_Convert(msg->ctx, buf, sizeof(buf));
    if (msg->ctx->type == MQTT::ERROR)
    {
        fprintf(stderr, "文件不存在！\n");
        return msg;
    }
    ret = readmsg(fd, buf + 5, msg->ctx->left_byte - 4 + msg->ctx->left_byte_len);
    if (ret <= 0)
    {
        fprintf(stderr, "发送报文出错！\n");
        msg->ctx->type = MQTT::ERROR;
        return msg;
    }
    MQTT_Convert(msg->ctx, buf, sizeof(buf));

    char tmp[20];
    // 读取文件所在ip
    MQTT_GetVHeader(msg->ctx, "filehost", tmp, sizeof(tmp));
    msg->ip = std::string(tmp, strlen(tmp));

    msg->fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_port = htons(FILEPORT);
    server.sin_family = AF_INET;
    if (0 == inet_pton(AF_INET, tmp, &server.sin_addr.s_addr))
    {
        MQTT_FillType(msg->ctx, MQTT::ERROR);
        perror("地址转换失败！\n");
        close(msg->fd);
        return msg;
    }
    if (0 != connect(msg->fd, (struct sockaddr *)&server, sizeof(server)))
    {
        MQTT_FillType(msg->ctx, MQTT::ERROR);
        fprintf(stderr, "连接目标节点失败！\n");
        close(msg->fd);
    }
    MQTT_Fill(msg->ctx, "filename", file.c_str());
    return msg;
}

int FileClientUI::recvfile(DeviceMsg &msg, int i, int left_size, EVP_MD_CTX *filectx)
{
    // 初始化hash生成器
    unsigned char mmd[SHA256_DIGEST_LENGTH];
    EVP_MD_CTX *ctx = EVP_MD_CTX_create();
    const EVP_MD *mdal = EVP_sha256();
    EVP_DigestInit(ctx, mdal);

    int outfd = open(md[i].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
    printf("文件长度%d\n", left_size);
    int size;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(msg.fd, &readfds);
    // 读取文件  redis-stable.tar.gz
    if (0 <= select(msg.fd + 1, &readfds, NULL, &readfds, NULL))
    {
        while (0 != (size = recv(msg.fd, buf + pos, BUFSIZE - pos, MSG_DONTWAIT)))
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
            if (left_size <= size)
            {
                write(outfd, buf, left_size);
                EVP_DigestUpdate(ctx, buf, left_size);
                if (NULL != filectx)
                {
                    EVP_DigestUpdate(filectx, buf, left_size);
                }
                pos = size - left_size;
                memmove(buf, buf + left_size, pos);
                break;
            }
            write(outfd, buf, size + pos);
            EVP_DigestUpdate(ctx, buf, size + pos);
            if (NULL != filectx)
            {
                EVP_DigestUpdate(filectx, buf, size + pos);
            }
            left_size -= size + pos;
            pos = 0;
        }
    }

    unsigned int len = SHA256_DIGEST_LENGTH;
    EVP_DigestFinal(ctx, mmd, &len);
    if (0 == check(md[i].c_str(), mmd))
    {
        fprintf(stderr, "接收%s的hash值有误！\n", md[i].c_str());
        size = -1;
    }
    else
    {
        md_flag.reset(i);
        printf("接收%s文件成功\n", md[i].c_str());
    }
    close(outfd);
    return size;
}

void FileClientUI::storemd(DeviceMsg &msg)
{
    // 存储md
    redisContext *redisctx = redisConnect("127.0.0.1", 6379);
    if (NULL == redisctx)
    {
        return;
    }
    for (int i = 0; i <= msg.blocknum; ++i)
    {
        if (md_flag[i])
        {
            redisCommand(redisctx, "sadd file:%s host:%s md:%s", msg.filename.c_str(), msg.ip.c_str(), md[i].c_str());
        }
    }
    redisFree(redisctx);
}
void FileClientUI::removemd(DeviceMsg &msg)
{
    // 删除md
    redisContext *redisctx = redisConnect("127.0.0.1", 6379);
    if (NULL == redisctx)
    {
        return;
    }
    redisCommand(redisctx, "del file:%s", msg.filename.c_str());
    redisFree(redisctx);
}

int FileClientUI::loadmd(DeviceMsg &msg, char *mdfile, int left_size)
{
    // 打开md文件
    int mdfd = open(mdfile, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (-1 == mdfd)
    {
        return -1;
    }
    // 返回0对端断开连接
    printf("md文件长度%d\n", left_size);

    int size = 0;
    // 读取md文件
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(msg.fd, &readfds);
    if (0 <= select(msg.fd + 1, &readfds, NULL, &readfds, NULL))
    {
        while (size = recv(msg.fd, buf + pos, BUFSIZE - pos, MSG_DONTWAIT))
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
            if (left_size <= size)
            {
                write(mdfd, buf, left_size);
                pos = size - left_size;
                memmove(buf, buf + left_size, pos);
                break;
            }
            write(mdfd, buf, size + pos);
            left_size -= size + pos;
            pos = 0;
        }
    }
    lseek(mdfd, 0L, SEEK_SET);
    auto mdstream = fdopen(mdfd, "r");

    // 将md文件加载到数组中
    for (int i = 0; i < msg.blocknum + 1; i++)
    {
        char tmp[70];
        fgets(tmp, 70, mdstream);
        tmp[strlen(tmp) - 1] = '\0';
        md.push_back(tmp);
    }
    printf("接收%s文件成功\n", mdfile);
    close(mdfd);
    return size;
}

void FileClientUI::combinefile(DeviceMsg &msg)
{
    // redis-stable.tar.gz
    int outfd = open(msg.filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
    char buf[BUFSIZE];
    sprintf(buf, "md%s%s.txt", msg.filename.c_str(), msg.ip.c_str());
    auto mdstream = fopen(buf, "r");
    int size;
    for (size_t i = 0; i < msg.filenum; i++)
    {
        fgets(buf, 70, mdstream);
        buf[strlen(buf) - 1] = '\0';
        int infd = open(buf, O_RDONLY);
        while (0 != (size = read(infd, buf, BUFSIZE)))
        {
            write(outfd, buf, size);
        }
        close(infd);
    }
    close(outfd);
}