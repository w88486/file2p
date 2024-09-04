#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "MQTT.h"
int level[4] = {127, 3777, 7777777};
void ito_char(int src, char *dest, int len)
{
    char cmd[] = "0123456789";
    int bit;
    int i = 0;
    while (src)
    {
        bit = src % 10;
        dest[i++] = cmd[bit];
        src /= 10;
    }
}

int MQTT_Fill(MQTT *ctx, const char *key, const char *value)
{
    // 检查是否有足够空间,并扩容
    if (0 == check_capcity(ctx, strlen(key) + strlen(value) + 4))
    {
        return 0;
    }
    int ret = 0;
    // 是key:value形式
    if (NULL != key)
    {
        ret = sprintf((char *)&(ctx->buf[ctx->size]), "%s", key);
        ctx->size += ret;
        ctx->left_byte += ret;
    }
    if (NULL != value)
    {
        // 如果不是键值对就不填充冒号
        const char *format = (NULL != key) ? ":%s\r\n" : "%s";
        ret = sprintf((char *)&(ctx->buf[ctx->size]), format, value);
        ctx->size += ret;
        ctx->left_byte += ret;
    }
    // 剩余长度需要更多字节存储 127 3777 7777777
    if (ctx->left_byte > level[ctx->left_byte_len - 1])
    {
        memmove(ctx->buf + ctx->left_byte_len + 1, ctx->buf + ctx->left_byte_len + 2, ctx->left_byte);
        ctx->buf[ctx->left_byte_len] |=  0x80;
        ++ctx->left_byte_len;
    }
    // 填充剩余长度
    for (int i = 0; i < ctx->left_byte_len; ++i)
    {
        ctx->buf[i + 1] = (ctx->left_byte >> (7 * i)) & 0x7f;
    }
    return 1;
}
int MQTT_GetVHeader(MQTT *ctx, const char *key, char *value, int len)
{
    unsigned char *pos = (unsigned char *)strstr((char *)ctx->buf, key);
    // 找不到key
    if (NULL == value)
    {
        return 0;
    }
    pos += strlen(key);
    // 不含冒号或没有value
    if (NULL == pos || NULL == pos + 1 || pos[0] != ':')
    {
        return 0;
    }
    // copy 值
    ++pos;
    int i = 0;
    while (pos[i] != '\r' && pos + i < ctx->buf + ctx->size)
    {
        value[i] = pos[i];
        ++i;
    }
    value[i] = '\0';
    return 1;
}

void MQTT_Convert(MQTT *ctx, const char *str, int len)
{
    ctx->left_byte = 0;
    ctx->left_byte_len = 1;
    if (NULL != str)
    {
        // 解析剩余长度
        for (int i = 0; i < 4; ++i)
        {
            ctx->left_byte |= (str[i + 1] & 0x7f) << (7 * i);
            // 如果当前字节的最高位为1，则后面的字节也是表示剩余字节的一部分
            if (0 != (str[i + 1] & 0x80))
            {
                ++ctx->left_byte_len;
            }
            else
            {
                break;
            }
        }
        // 报文总长度
        ctx->size = ctx->left_byte + 2;
        check_capcity(ctx, ctx->size);
        // 缓存
        if (ctx->size < len)
        {
            strncpy((char *)ctx->buf, (char *)str, ctx->size);
        }
        // 解析类型
        ctx->type = (MQTT::TYPE)(str[0] & 0x0f);
    }
}
void MQTT_Init(MQTT *ctx, int type, int cap)
{
    // 设置类型、分配cap大小的空间
    ctx->type = (MQTT::TYPE)type;
    ctx->capacity = cap;
    ctx->buf = (unsigned char *)calloc(cap, sizeof(unsigned char));
    // 初始时只有报文头部2字节
    ctx->size = 2;
    ctx->left_byte = 0;
    // 填充类型
    ctx->buf[0] = (unsigned char)type;
    ctx->left_byte_len = 1;
}
void MQTT_Fini(MQTT *ctx)
{
    if (NULL != ctx && NULL != ctx->buf)
    {
        free(ctx->buf);
        ctx->buf = NULL;
    }
    if (NULL != ctx)
    {
        free(ctx);
        ctx = NULL;
    }
}
int check_capcity(MQTT *ctx, int len)
{
    if (ctx->size + len >= ctx->capacity)
    {
        return change_capcity(ctx, len);
    }
    return 1;
}
int change_capcity(MQTT *ctx, int len)
{
    void *tmp = realloc(ctx->buf, ctx->capacity + (ctx->capacity >> 1) + len);
    if (NULL == tmp)
    {
        return 0;
    }
    // 更新为原来大小的1.5倍多len
    ctx->capacity += (ctx->capacity >> 1) + len;
    return 1;
}

void MQTT_FlushBuf(MQTT *ctx)
{
    ctx->size = 2;
    ctx->left_byte = 0;
    ctx->left_byte_len = 1;
}

int MQTT_FillType(MQTT *ctx, MQTT::TYPE type)
{
    ctx->type = type;
    ctx->buf[0] = (type & 0x0f);
    return 1;
}