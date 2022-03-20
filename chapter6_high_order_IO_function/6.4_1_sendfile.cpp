/*
#include<sys/sendfile.h>
ssize_t sendfile(int out_fd,int in_fd,off_t* offset,size_t count);

in_fd参数是待读出内容的文件描述符，out_fd参数是待写入内容的文件描述符。
offset参数指定从读入文件流的哪个位置开始读，如果为空，则使用读入文件流默认的起始位置。
count参数指定在文件描述符in_fd和out_fd之间传输的字节数。
sendfile成功时返回传输的字节数，失败则返回-1并设置errno。

该函数的man手册明确指出，in_fd必须是一个支持类似mmap函数的文件描述符，即它必须指向真实的文件，
不能是socket和管道；而out_fd则必须是一个socket。
由此可见，sendfile几乎是专门为在网络上传输文件而设计的。

下面的代码清单6-3利用sendfile函数将服务器上的一个文件传送给客户端。
*/

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/sendfile.h>
// 该函数用sendfile传送文件
// 相比6.3_1，以下代码没有为目标文件分配任何用户空间的缓存，也没有执行读取文件的操作，
// 但同样实现了文件的发送，其效率显然要高得多。

int main(int argc, char *argv[])
{
    if (argc <= 3)
    {
        printf("usage:%s ip_address port_number filename\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    // 第三个参数当作要传输的文件名
    const char *file_name = argv[3];
    int filefd = open(file_name, O_RDONLY);
    assert(filefd > 0);         // 打开失败需要报错
    struct stat stat_buf;       // 该结构体存储有文件大小、链接数、用户权限等各种信息
    fstat(filefd, &stat_buf);   // 将filefd的文件属性存储入stat_buf结构体

    // IPv4socket结构体
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    // 将用字符串表示的IP地址ip转换成用网络字节序整数表示的IP地址
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);

    // 接受连接，client为用来获取连接的远端socket地址，其长度由第三个参数指出，注意后两个传的都是指针
    // 这里sockaddr是通用的socket结构体类型
    // accept成功会返回一个新的连接socket，该socket唯一标识了被接受的这个连接
    // accept成功返回0，失败返回-1并置error
    int connfd = accept(sock, (struct sockaddr *)&client, &client_addrlength);

    if (connfd < 0)
    {
        printf("errno is:%d\n", errno);
    }
    else
    {
        // 这里第三个参数设置为NULL似乎是比0清楚许多
        sendfile(connfd, filefd, NULL, stat_buf.st_size);
        close(connfd);
    }
    close(sock);
    return 0;
}