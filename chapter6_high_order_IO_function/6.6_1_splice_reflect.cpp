/*
#include<fcntl.h>
ssize_t splice(int fd_in,loff_t*off_in,int fd_out,loff_t*off_out,size_t len,unsigned int flags);
fd_in参数是待输入数据的文件描述符。
如果fd_in是一个管道文件描述符，那么off_in参数必须被设置为NULL。
如果fd_in不是一个管道文件描述符（比如socket），那么off_in表示从输入数据流的何处开始读取数据。
此时，若off_in被设置为NULL，则表示从输入数据流的当前偏移位置读入；
若off_in不为NULL，则它将指出具体的偏移位置。
fd_out/off_out参数的含义与fd_in/off_in相同，不过用于输出数据流。
len参数指定移动数据的长度；flags参数则控制数据如何移动。

使用splice函数时，fd_in和fd_out必须至少有一个是管道文件描述符。
splice函数调用成功时返回移动字节的数量。
它可能返回0，表示没有数据需要移动，
这发生在从管道中读取数据（fd_in是管道文件描述符）而该管道没有被写入任何数据时。
splice函数失败时返回-1并设置errno。
*/


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
//使用splice函数实现的回射服务器
int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("usage:%s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    // 开始监听
    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (struct sockaddr *)&client, &client_addrlength);
    if (connfd < 0)
    {
        printf("errno is:%d\n", errno);
    }
    else
    {
        int pipefd[2];
        assert(ret != -1);
        ret = pipe(pipefd); /*创建管道*/
        // 下面最后的标志由于实现有bug，所以内核2.6.21后就实际上没有任何效果了
        /*将connfd上流入的客户数据定向到管道中*/
        ret = splice(connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);
        /*将管道的输出定向到connfd客户连接文件描述符*/
        ret = splice(pipefd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);
        close(connfd);
    }
    close(sock);
    return 0;
}
