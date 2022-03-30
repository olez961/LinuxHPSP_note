#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

static const int CONTROL_LEN = CMSG_LEN( sizeof(int) );

// 子进程向父进程发送自己打开的文件描述符
/*发送文件描述符，fd参数是用来传递信息的UNIX域socket，fd_to_send参数是待发送的文件描述符*/
void send_fd( int fd, int fd_to_send )
{
    struct iovec iov[1];
    struct msghdr msg;  // 用于sendmsg和recvmsg的结构体
    char buf[0];

    iov[0].iov_base = buf;
    iov[0].iov_len = 1;
    msg.msg_name    = NULL; /*socket地址*/
    msg.msg_namelen = 0;    /*socket地址的长度*/
    msg.msg_iov     = iov;  /*分散的内存块，见后文*/
    msg.msg_iovlen = 1;     /*分散内存块的数量*/

    cmsghdr cm;     // 用于存储辅助数据对象信息的数据结构
    cm.cmsg_len = CONTROL_LEN;
    cm.cmsg_level = SOL_SOCKET;
    cm.cmsg_type = SCM_RIGHTS;
    *(int *)CMSG_DATA( &cm ) = fd_to_send;
    msg.msg_control = &cm;  /*设置辅助数据*/  /*指向辅助数据的起始位置*/
    msg.msg_controllen = CONTROL_LEN;       /*辅助数据的大小*/

    // 以下函数为通用数据读写函数，和其对应的还有recvmsg
    // 这一对系统调用不仅能用于TCP数据，还能用于UDP数据报
    // 第一个参数指定被操作的目标socket
    // 第二个参数是msghdr结构体类型的指针
    // 即上面初始化的那些变量，该结构体内部还有一个结构体，即上面的cmsghdr
    // 该结构体的msg_flags参数用于复制函数中的flags参数，并在调用过程中更新
    // 第三个参数即上面提到的flags参数
    sendmsg( fd, &msg, 0 );
}

/*接收目标文件描述符*/
int recv_fd( int fd )
{
    struct iovec iov[1];    // 每一个结构体封装一块内存的起始位置和长度，共两个参数
    struct msghdr msg;
    char buf[0];

    iov[0].iov_base = buf;  // 内存块起始地址
    iov[0].iov_len = 1;     // 内存长度
    msg.msg_name    = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov     = iov;
    msg.msg_iovlen = 1;

    cmsghdr cm;
    msg.msg_control = &cm;
    msg.msg_controllen = CONTROL_LEN;

    recvmsg( fd, &msg, 0 );

    int fd_to_read = *(int *)CMSG_DATA( &cm );
    return fd_to_read;
}

int main()
{
    int pipefd[2];
    int fd_to_pass = 0;

    /*创建父、子进程间的管道，文件描述符pipefd[0]和pipefd[1]都是UNIX域socket*/
    int ret = socketpair( PF_UNIX, SOCK_DGRAM, 0, pipefd );
    assert( ret != -1 );

    pid_t pid = fork();
    assert( pid >= 0 );

    if ( pid == 0 )
    {
        // 双方都需要关闭管道的一头才能正常通信，且通信是半双工的
        close( pipefd[0] );
        // open第三个参数仅当创建文件时使用，用于指定文件的访问权限
        // 可以看到这里是将一个新的文件描述符传给了fd_to_pass
        // 即使开启root权限似乎也无法创建新文件
        fd_to_pass = open( "test.txt", O_RDWR, 0666 );
        /*子进程通过管道将文件描述符发送到父进程。
        如果文件test.txt打开失败，则子进程将标准输入文件描述符发送到父进程*/
        send_fd( pipefd[1], ( fd_to_pass > 0 ) ? fd_to_pass : 0 );
        close( fd_to_pass );
        // 可以看到子进程在这里就退出了
        exit( 0 );
    }

    // 如上所述，父进程也需要关闭管道的一头
    close( pipefd[1] );
    fd_to_pass = recv_fd( pipefd[0] );  /*父进程从管道接收目标文件描述符*/
    char buf[1024];
    memset( buf, '\0', 1024 );
    // 第三个参数指出欲读取的字节上限
    read( fd_to_pass, buf, 1024 );  /*读目标文件描述符，以验证其有效性*/
    printf( "I got fd %d and data %s\n", fd_to_pass, buf );
    close( fd_to_pass );
}
