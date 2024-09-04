#ifndef MQTT_H
#define MQTT_H

#define MAXCON 1000
#define BUFSIZE 1024
#define REDISPORT 6379

typedef unsigned short unint16;
typedef unsigned char unint8;

typedef struct mqtt
{
    enum TYPE
    {
        Reserved = 0,
        CONNECT,
        CONNACK,
        PUBLISH,
        PUBACK,
        PUBREC,
        PUBREL,
        PUBCOMP,
        SUBSCRIBE,// 查找文件所在ip
        SUBACK,
        UNSUBSCRIBE,// 文件未找到
        UNSUBACK,
        SUBRECV,
        ERROR,
        DISCONNECT,
    }type;
    int left_byte;
    int left_byte_len;
    unsigned char* buf;
    int size;
    int capacity; 
}MQTT;
int MQTT_Fill(MQTT *ctx, const char *key, const char *value);
int MQTT_FillType(MQTT *ctx, MQTT::TYPE);
int MQTT_GetVHeader(MQTT *ctx, const char *key, char *value, int len);
void MQTT_Init(MQTT *ctx, int type, int cap);
void MQTT_Fini(MQTT *ctx);
int check_capcity(MQTT *ctx, int len);
int change_capcity(MQTT *ctx, int len);
void MQTT_Convert(MQTT *ctx, const char *value, int len);
void MQTT_FlushBuf(MQTT *);

#endif
