#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET sock_t;
#define CLOSE_SOCK closesocket
#define SLEEP_SEC(x) Sleep(x*1000)
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int sock_t;
#define CLOSE_SOCK close
#define SLEEP_SEC(x) sleep(x)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define BUF_SIZE 512
void getTimeStr(char* buf, int len)
{
    time_t t = time(NULL);
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", &tm);
}
sock_t connectServer(char* host, char* port)
{
    struct addrinfo hint, *res, *p;
    memset(&hint,0,sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    sock_t sock = INVALID_SOCKET;
    int ret = getaddrinfo(host, port, &hint, &res);
    if(ret !=0) return INVALID_SOCKET;
    for(p=res;p;p=p->ai_next)
    {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(sock == INVALID_SOCKET) continue;
        if(connect(sock, p->ai_addr, p->ai_addrlen)==0) break;
        CLOSE_SOCK(sock);
        sock = INVALID_SOCKET;
    }
    freeaddrinfo(res);
    return sock;
}
void buildCardMsg(char* out, char* door)
{
    char timeBuf[32];
    getTimeStr(timeBuf, 32);
    int uid = rand() % 1000000;
    sprintf(out, "%s,%06d,%s\n", door, uid, timeBuf);
}
int checkUnlockCmd(char* recv, char* door)
{
    char expect[64];
    sprintf(expect, "^^%s,UNLOCK,OPEN##", door);
    char temp[64];
    strcpy(temp, recv);
    char* p = strchr(temp, '\n');
    if(p) *p = '\0';
    p = strchr(temp, '\r');
    if(p) *p = '\0';
    return strcmp(temp, expect) == 0;
}
void printHelp()
{
    printf("使用格式: ./client 间隔秒 门号 服务IP 端口\n");
    printf("示例: ./client 4 105 127.0.0.1 9001\n");
}
int main(int argc, char** argv)
{
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif
    if(argc <3)
    {
        printHelp();
        return -1;
    }
    int interval = atoi(argv[1]);
    char* door = argv[2];
    char* host = "127.0.0.1";
    char* port = "9001";
    if(argc>=4) host = argv[3];
    if(argc>=5) port = argv[4];
    if(strlen(door)!=3 || !isdigit(door[0]) || !isdigit(door[1]) || !isdigit(door[2]))
    {
        printf("门号必须是3位数字\n");
        return -1;
    }
    srand((unsigned)time(NULL) ^ getpid());
    printf("门禁终端启动 门号:%s 发送间隔:%ds 服务器:%s:%s\n", door, interval, host, port);
    while(1)
    {
        sock_t sock = connectServer(host, port);
        if(sock == INVALID_SOCKET)
        {
            printf("连接失败，3秒后重试\n");
            SLEEP_SEC(3);
            continue;
        }
        printf("成功连接服务端\n");
        while(1)
        {
            char sendBuf[128];
            buildCardMsg(sendBuf, door);
            int sendRet = send(sock, sendBuf, strlen(sendBuf), 0);
            if(sendRet == SOCKET_ERROR)
            {
                printf("发送失败，断开重连\n");
                break;
            }
            printf("发送刷卡记录: %s", sendBuf);
            char recvBuf[BUF_SIZE];
            int recvNum = recv(sock, recvBuf, BUF_SIZE-1, 0);
            if(recvNum <=0)
            {
                printf("服务端断开，重连\n");
                break;
            }
            recvBuf[recvNum] = '\0';
            if(checkUnlockCmd(recvBuf, door))
            {
                printf("=====门%s开锁成功=====", door);
            }else{
                printf("收到未知指令:%s\n", recvBuf);
            }
            SLEEP_SEC(interval);
        }
        CLOSE_SOCK(sock);
        SLEEP_SEC(3);
    }
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
