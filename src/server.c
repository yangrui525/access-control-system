#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET sock_t;
typedef CRITICAL_SECTION mutex_t;
#define CLOSE_SOCK closesocket
#define SLEEP_MS(x) Sleep(x)
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
typedef int sock_t;
typedef pthread_mutex_t mutex_t;
#define CLOSE_SOCK close
#define SLEEP_MS(x) usleep(x*1000)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#define BUF_LEN 1024
#define LINE_MAX 256
#define CSV_PATH "data/access_log.csv"
mutex_t fileMutex;

void mutexInit(mutex_t* m)
{
#ifdef _WIN32
    InitializeCriticalSection(m);
#else
    pthread_mutex_init(m, NULL);
#endif
}
void mutexLock(mutex_t* m)
{
#ifdef _WIN32
    EnterCriticalSection(m);
#else
    pthread_mutex_lock(m);
#endif
}
void mutexUnLock(mutex_t* m)
{
#ifdef _WIN32
    LeaveCriticalSection(m);
#else
    pthread_mutex_unlock(m);
#endif
}
void makeDir()
{
#ifdef _WIN32
    _mkdir("data");
#else
    mkdir("data", 0755);
#endif
}
void getNowTime(char* buf, int len)
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
int isAllDigit(char* s, int len)
{
    for(int i=0;i<len;i++)
        if(!isdigit((unsigned char)s[i])) return 0;
    return 1;
}
int checkTimeStr(char* t)
{
    if(strlen(t) != 19) return 0;
    if(t[4]!='-' || t[7]!='-' || t[10]!=' ' || t[13]!=':' || t[16]!=':')
        return 0;
    return isAllDigit(t,4) && isAllDigit(t+5,2) && isAllDigit(t+8,2)
        && isAllDigit(t+11,2) && isAllDigit(t+14,2) && isAllDigit(t+17,2);
}
int parseCardMsg(char* msg, char* door, char* uid, char* timeStr)
{
    char extra;
    int ret = sscanf(msg, "%3[0-9],%6[0-9],%19[0-9 :-]%c", door, uid, timeStr, &extra);
    if(ret !=3) return 0;
    if(strlen(door)!=3 || strlen(uid)!=6) return 0;
    if(!isAllDigit(door,3) || !isAllDigit(uid,6)) return 0;
    if(!checkTimeStr(timeStr)) return 0;
    return 1;
}
long getNewId()
{
    FILE* f = fopen(CSV_PATH, "r");
    if(!f) return 1;
    char line[LINE_MAX];
    long id =0;
    while(fgets(line, LINE_MAX, f))
    {
        if(strstr(line,"record_id") != NULL) continue;
        id++;
    }
    fclose(f);
    return id+1;
}
void saveLog(char* door, char* uid, char* cardT, char* clientAddr, char* raw)
{
    mutexLock(&fileMutex);
    makeDir();
    FILE* f = fopen(CSV_PATH, "a");
    if(!f) {mutexUnLock(&fileMutex); return;}
    char recvT[32];
    getNowTime(recvT,32);
    long newId = getNewId();
    fprintf(f,"%ld,%s,%s,%s,%s,%s,%s\n",
            newId, door, uid, cardT, clientAddr, recvT, raw);
    fclose(f);
    mutexUnLock(&fileMutex);
}
void buildUnlockCmd(char* out, char* door)
{
    sprintf(out, "^^%s,UNLOCK,OPEN##\n", door);
}
void trimLine(char* s)
{
    int l = strlen(s);
    while(l>0 && (s[l-1] == '\n' || s[l-1] == '\r'))
    {
        s[l-1] = '\0'; l--;
    }
}
typedef struct{
    sock_t sock;
    char addr[64];
}ClientCtx;
void handleClient(ClientCtx* ctx)
{
    char buf[BUF_LEN];
    char lineBuf[LINE_MAX];
    int lineLen =0;
    printf("新客户端接入: %s\n", ctx->addr);
    while(1)
    {
        int recvNum = recv(ctx->sock, buf, BUF_LEN, 0);
        if(recvNum <=0) break;
        for(int i=0;i<recvNum;i++)
        {
            char c = buf[i];
            if(c == '\n')
            {
                lineBuf[lineLen] = '\0';
                trimLine(lineBuf);
                if(strlen(lineBuf)>0)
                {
                    char door[4], uid[7], timeS[20];
                    if(parseCardMsg(lineBuf, door, uid, timeS))
                    {
                        saveLog(door, uid, timeS, ctx->addr, lineBuf);
                        char cmd[64];
                        buildUnlockCmd(cmd, door);
                        send(ctx->sock, cmd, strlen(cmd), 0);
                        trimLine(cmd);
                        printf("存储记录 | 返回指令: %s | 客户端:%s\n", cmd, ctx->addr);
                    }else{
                        printf("非法报文: %s 客户端:%s\n", lineBuf, ctx->addr);
                    }
                }
                lineLen =0;
            }else if(lineLen < LINE_MAX-1){
                lineBuf[lineLen++] = c;
            }else lineLen=0;
        }
    }
    printf("客户端断开: %s\n", ctx->addr);
    CLOSE_SOCK(ctx->sock);
    free(ctx);
}
#ifdef _WIN32
DWORD WINAPI clientThread(LPVOID p)
{
    handleClient((ClientCtx*)p);
    return 0;
}
#else
void* clientThread(void* p)
{
    handleClient((ClientCtx*)p);
    return NULL;
}
#endif
void startThread(ClientCtx* ctx)
{
#ifdef _WIN32
    HANDLE h = CreateThread(NULL,0,clientThread,ctx,0,NULL);
    if(h) CloseHandle(h);
#else
    pthread_t tid;
    pthread_create(&tid, NULL, clientThread, ctx);
    pthread_detach(tid);
#endif
}
sock_t createServerSock(char* host, char* port)
{
    struct addrinfo hint, *res, *p;
    memset(&hint,0,sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;
    sock_t server = INVALID_SOCKET;
    int ret = getaddrinfo(host, port, &hint, &res);
    if(ret !=0) return INVALID_SOCKET;
    int opt =1;
    for(p=res;p;p=p->ai_next)
    {
        server = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(server == INVALID_SOCKET) continue;
        setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        if(bind(server, p->ai_addr, p->ai_addrlen)==0 && listen(server, 16)==0)
            break;
        CLOSE_SOCK(server);
        server = INVALID_SOCKET;
    }
    freeaddrinfo(res);
    return server;
}
void initCsv()
{
    makeDir();
    FILE* f = fopen(CSV_PATH, "r");
    if(f){fclose(f); return;}
    f = fopen(CSV_PATH, "w");
    if(f)
    {
        fprintf(f,"record_id,door_id,user_id,card_time,client_ip,server_recv_time,raw_msg\n");
        fclose(f);
    }
}
int main(int argc, char** argv)
{
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif
    char* host = "0.0.0.0";
    char* port = "9001";
    if(argc>=2) host = argv[1];
    if(argc>=3) port = argv[2];
    mutexInit(&fileMutex);
    initCsv();
    sock_t server = createServerSock(host, port);
    if(server == INVALID_SOCKET)
    {
        printf("服务端套接字创建失败\n");
        return -1;
    }
    printf("门禁服务端启动 %s:%s 日志路径:%s\n", host, port, CSV_PATH);
    while(1)
    {
        struct sockaddr_storage addr;
#ifdef _WIN32
        int addrLen = sizeof(addr);
#else
        socklen_t addrLen = sizeof(addr);
#endif
        sock_t cliSock = accept(server, (struct sockaddr*)&addr, &addrLen);
        if(cliSock == INVALID_SOCKET) continue;
        ClientCtx* ctx = (ClientCtx*)malloc(sizeof(ClientCtx));
        ctx->sock = cliSock;
        char ip[64], prt[16];
        getnameinfo((struct sockaddr*)&addr, addrLen, ip, 64, prt, 16, NI_NUMERICHOST|NI_NUMERICSERV);
        sprintf(ctx->addr, "%s:%s", ip, prt);
        startThread(ctx);
    }
    CLOSE_SOCK(server);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
