/* File Name: client2.cpp */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXLINE 4096

int str_to_number(const char* str);

int main(int argc, char** argv)
{
    int sockfd, n, rec_len;
    char recvline[4096], sendline[4096];
    char buf[MAXLINE];
    struct sockaddr_in servaddr;

    if(argc != 3) {
        printf("usage: ./client <ipaddress> <port>\n");
        exit(0);
    }

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("create socket error : %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(str_to_number(argv[2]));

    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        printf("inet_pton error for %s\n", argv[1]);
        exit(0);
    }

    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("connect error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    printf("send msg to server: \n");
    fgets(sendline, MAXLINE, stdin);
    if(send(sockfd, sendline, sizeof(sendline), 0) < 0) {
        printf("send msg error: %s(error: %d)\n", strerror(errno), errno);
        exit(0);
    }

    if((rec_len = recv(sockfd, recvline, sizeof(recvline), 0)) == -1) {
        perror("recv error");
        exit(1);
    }

    buf[rec_len] = '\0';    // 不截断的话可能会将上次读到的数据也输出出来
    printf("Recieved : %s ", recvline);

    close(sockfd);
    exit(0);
}

int str_to_number(const char* str) {
    int i, len, num = 0;
    len = strlen(str);

    for(i = 0; i < len; ++i) {
        num = num * 10 + str[i] - '0';
    }
    return num;
}