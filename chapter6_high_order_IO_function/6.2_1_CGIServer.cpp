#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
// CGI服务器基本原理
// 将服务器上的输出发送到客户端

// 两个参数一个是输入参数个数，一个是储存输入参数的字符数组
int main(int argc, char *argv[])
{
    // 希望输入至少两个参数（除文件名外的两个参数）
    if (argc <= 2)
    {
        // 希望输入格式是 ip地址 端口号，下面的basename返回文件名，如usr\c，会返回c
        // 如果argv[0]最后一位为/，则返回空，'.'和'..'返回相同字符串
        printf("usage:%s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];   // 将第一个传入的参数放入一个字符数组
    int port = atoi(argv[2]);   // 将字符串转换为整型
    

    // 以下三部分用来初始化socket结构体
    struct sockaddr_in address; // IPv4专用socket结构体，IPv6专用为sockaddr_in6

    // bzero(void *s, int n)等价于memset((void*)s, 0, size_t n)，是一个C语言函数，但不是标准库函数
    // 没有在ANSI中定义，所以其移植性不是很好，建议memset代替
    bzero(&address, sizeof(address));   // 用于将内存块的前n个字节清零
    address.sin_family = AF_INET;       // 地址族，AF是address family缩写，INET表示TCP/IPv4

    // 将用字符串表示的IP地址ip转换成用网络字节序整数表示的IP地址，并将结果保存到第三个参数指定的内存中
    // 第一个参数指定协议族
    inet_pton(AF_INET, ip, &address.sin_addr);
    // host to network short将主机字节序转化为网络字节序，大端字节序指整数高位存储在低位地址
    address.sin_port = htons(port);     // PC大多采用小端字节序，大端字节序又称网络字节序

    // PF指protocol family协议族，第一个参数指出协议族，第二个参数指出类型，下面是TCP流类型
    // 若是UDP则是SOCK_UGRAM数据报服务，第三个参数一般置0，因为前两个参数已经决定了协议类型
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);  // 上述函数调用成功返回一个socket文件描述符，失败返回-1并置error

    // 命名socket，命名指将一个socket与socket地址绑定，以下将address地址分配给未命名的sockfd文件描述符
    // 常见两种错误：
    // 1、EACCES，绑定的端口是受保护的地址，仅超级用户能够访问，如0-1023的知名服务端口
    // 2、EADDRINUSE，被绑定的地址正在使用中
    int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);  // bind成功返回0，失败返回-1并置error

    // 第一个参数表示被监听的socket，第二个参数提示内核监听队列最大长度，linux中处于完全连接的socket上限为5 + 1
    ret = listen(sock, 5);
    assert(ret != -1);  // 成功返回0，失败返回-1并置error

    // 以下新建一个客户端socket结构体
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);

    // 接受连接，client为用来获取连接的远端socket地址，其长度由第三个参数指出，注意后两个传的都是指针
    // 这里sockaddr是通用的socket结构体类型
    // accept成功会返回一个新的连接socket，该socket唯一标识了被接受的这个连接
    // accept成功返回0，失败返回-1并置error
    int connfd = accept(sock, (struct sockaddr *)&client, &client_addrlength);
    if (connfd < 0)
    {
        // 这里的error还会输出提示信息
        printf("errno is:%d\n", errno);
    }
    else
    {
        // 关闭标准输出STDOUT_FILENO，该值是1
        close(STDOUT_FILENO);
        // 复制socket文件描述符connfd
        // dup函数创建一个新的文件描述符，该文件描述符与原有文件描述符指向相同的文件、管道或者网络连接
        dup(connfd);    // 由于dup总是返回系统中最小的可用文件描述符，所以这里返回值是1，即标准输出
        // 此时标准输出直接被发送到socket中，因此该输出被客户端获得而不是输出在服务器程序的终端上
        printf("abcd\n");   // 此即CGI服务器的基本工作原理
        close(connfd);  // 关闭socket
    }
    close(sock);
    return 0;
}
