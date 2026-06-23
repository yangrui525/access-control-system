#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CSV_PATH "data/access_log.csv"
#define LINE_BUF 512
void trim(char* s)
{
    int l = strlen(s);
    while(l>0 && (s[l-1] == '\n' || s[l-1] == '\r'))
    {
        s[l-1] = '\0'; l--;
    }
}
int splitCsv(char* line, char** cols, int maxCol)
{
    int cnt =0;
    char* p = strtok(line, ",");
    while(p && cnt < maxCol)
    {
        cols[cnt++] = p;
        p = strtok(NULL, ",");
    }
    return cnt;
}
void printHelp()
{
    printf("使用: ./query [csv路径] [展示条数] [筛选门号]\n");
    printf("示例1 ./query data/access_log.csv 30\n");
    printf("示例2 ./query data/access_log.csv 20 105\n");
}
int main(int argc, char** argv)
{
    char* path = CSV_PATH;
    int limit = 30;
    char* filterDoor = NULL;
    if(argc >=2) path = argv[1];
    if(argc >=3) limit = atoi(argv[2]);
    if(argc >=4) filterDoor = argv[3];
    if(limit <=0)
    {
        printHelp();
        return -1;
    }
    FILE* f = fopen(path, "r");
    if(!f)
    {
        printf("文件打开失败:%s\n", path);
        return -1;
    }
    printf("%-8s %-6s %-8s %-20s %-18s %-20s\n",
           "记录ID","门号","人员ID","刷卡时间","客户端IP","服务接收时间");
    printf("--------------------------------------------------------------------------\n");
    int printCnt =0;
    char line[LINE_BUF];
    while(fgets(line, LINE_BUF, f) && printCnt < limit)
    {
        trim(line);
        if(strstr(line,"record_id") != NULL || strlen(line)==0) continue;
        char* col[8];
        int colNum = splitCsv(line, col, 8);
        if(colNum <6) continue;
        if(filterDoor && strcmp(col[1], filterDoor) !=0) continue;
        printf("%-8s %-6s %-8s %-20s %-18s %-20s\n",
               col[0], col[1], col[2], col[3], col[4], col[5]);
        printCnt++;
    }
    fclose(f);
    if(printCnt ==0)
        printf("无匹配刷卡记录\n");
    return 0;
}
