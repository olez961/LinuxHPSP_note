/* File Name: server.cpp */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DEFAULT_PORT 8000
#define MAXLINE 4096
// 可以通过client.cpp，client2.cpp以及telnet与同主机上的此服务器通讯
// 格式为telnet 127.0.0.1 8000

int main(int argc, char **argv)
{
    int socket_fd, connect_fd;
    struct sockaddr_in servaddr;
    char buff[4096], sendBuff[MAXLINE];
    int n;
    // 初始化socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    // 初始化
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    // IP地址设置成INADDR_ANY，让系统自动获取本机的IP地址
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(DEFAULT_PORT); // 设置端口

    // 将本地地址绑定到所创建的套接字上
    if ((bind(socket_fd, (struct sockaddr *)&servaddr, sizeof(servaddr))) == -1)
    {
        printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    // 开始监听是否有客户端连接
    if (listen(socket_fd, 10) == -1)
    {
        printf("listen socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    printf("=======waiting for client's request=======\n");
    int cnt = 1;
    while (1)
    {
        // 阻塞直到有客户端连接，不然浪费CPU资源
        if ((connect_fd = accept(socket_fd, (struct sockaddr *)NULL, NULL)) == -1)
        {
            printf("accept socket error: %s(errno: %d)\n", strerror(errno), errno);
            continue; // 注意这里要接着等待
        }
        while (1)
        {
            // 接受客户端传过来的数据
            n = recv(connect_fd, buff, MAXLINE, 0);
            memset(sendBuff, '\0', sizeof(sendBuff));
            sprintf(sendBuff, "Hello, you are connected! This is communication %d.\n\n", cnt);
            // 向客户端发送数据
            if ((send(connect_fd, sendBuff, MAXLINE, 0)) == -1)
            {
                close(socket_fd);   // 出错顺便关闭socket，否则下次启动程序还要等一会
                perror("send error");
                close(connect_fd);
                exit(0);
            }

            buff[n] = '\0';
            printf("recv msg from client: %s\n", buff);
            memset(buff, '\0', sizeof(buff));
            // close(connect_fd);
            ++cnt;
            if(cnt % 5 == 0) {
                close(connect_fd);
                break;
            }
        }
        if(cnt == 20) {
            break;
        }
    }
    close(socket_fd);
}