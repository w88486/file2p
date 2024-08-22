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

void FileClientUI::upload()
{
    CmdMsg msg;
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
            msg.type = CmdMsg::REDONWLOAD;
            writemsg(arg.fd, &msg, sizeof(msg));
            reupload(blocksize);
            return;
        }
    }
    // 分片
    int mdtxt;
    msg = splitfile(file, &mdtxt, blocksize);
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

void FileClientUI::download()
{
    // 初始化sha256上下文
    filectx = EVP_MD_CTX_create();
    const EVP_MD *mdal = EVP_sha256();
    EVP_DigestInit(filectx, mdal);

    // 读取文件相关信息
    CmdMsg msg;
    readmsg(arg.fd, &msg, sizeof(msg));
    // 读取md文件
    char mdfile[30];
    sprintf(mdfile, "md%s.txt", arg.ip);
    // 文件分片md数组
    int size = loadmd(arg.fd, mdfile, msg.mdlen, md, msg.blocknum + 1);
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
        size = recvfile(arg.fd, md[i], size, filectx);
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
    if (0 == check(md[msg.blocknum].c_str(), filemd))
    {
        ret = "ERR";
    }
    writemsg(arg.fd, ret, 4);
    // 重传
    redownload(arg.fd);
    // 合并
    combinefile(md, (char*)msg.filename, msg.blocknum);
    EVP_MD_CTX_destroy(filectx);
}