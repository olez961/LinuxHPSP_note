#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // close定义头文件
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdio>
#include <iostream>
#include <errno.h>

using namespace std;

#define MAX_LEN 4096

int str_to_number(const char* str);

int main(int argc, char** argv)
{
    int sk;
    char buf[MAX_LEN], recvBuf[MAX_LEN];
    struct sockaddr_in ser_addr;    //是用于指定对方(目的主机)信息
    struct sockaddr_in loc_addr;    //可以用来指定一些本地的信息，比如指定端口进行通信，而不是让系统随机分配
    int ser_addr_len,loc_addr_len;
    int ret,count;
    struct in_addr addr;

    if (argc != 3)
    {
        printf("Error: the number of args must be 3\n");
        exit(0);
    }

    //配置服务器信息
    bzero(&ser_addr, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;                                //设置为IPV4通信
    ser_addr.sin_addr.s_addr = inet_addr(argv[1]);                //设置目的ip
    ser_addr.sin_port = htons(str_to_number(argv[2]));            //设置目的端口去链接服务器
    ser_addr_len = sizeof(ser_addr);
    cout << inet_addr(argv[1]) << endl;
    cout << htons(str_to_number(argv[2])) << endl;
    cout << htons(8000) << endl;

    sk = socket(AF_INET, SOCK_STREAM, 0);                        //设置UDP报文传输    0表示默认    SOCK_DGRAM 默认使用UDP
    //其中第三位 0 是调用方式标志位，设置socket通方式，比如非阻塞
    if(sk<0)
    {
        printf("socket create failure\n");
        return -1;
    }

    if((connect(sk, (struct sockaddr*)&ser_addr, sizeof(ser_addr))) < 0) {
        printf("connect error: %s(errno: %d)\n", strerror(errno), errno);
    } else {
        printf("connected!\n");
    }
    for (;;)
    {
        printf("Input info:>>>");
        scanf("%s", buf);
        if (!strcmp(buf, "quit"))
            break;
        //sendto(sk, buf, sizeof(buf), 0, (struct sockaddr*)&ser_addr, ser_addr_len);
        send(sk, buf, sizeof(buf), 0);

        //memset(buf, '\0', sizeof(buf));
        //count = recvfrom(sk,recvBuf,sizeof(recvBuf),0,(struct sockaddr*)&loc_addr,(socklen_t *)&loc_addr_len);
        count = recv(sk,recvBuf,sizeof(recvBuf),0);
        if (count==-1)
        {
            printf("receive data failure\n");
            return -1;
        }
        addr.s_addr = ser_addr.sin_addr.s_addr;
        printf("Receive info: %s from %s %d\n", recvBuf,inet_ntoa(addr),ser_addr.sin_port);
    }

    printf("communicate end\n");
    close(sk);
    return 0;
}

int str_to_number(const char* str)
{
    int i,len, num = 0;
    len= strlen(str);

    for (i = 0; i < len;i++)
        num = num * 10 + str[i] - '0';

    return num;
}