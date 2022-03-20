/*
tee函数在两个管道文件描述符之间复制数据，也是零拷贝操作。
它不消耗数据，因此源文件描述符上的数据仍然可以用于后续的读操作。

#include<fcntl.h>
ssize_t tee(int fd_in,int fd_out,size_t len,unsigned int flags);
该函数的参数的含义与splice相同（但fd_in和fd_out必须都是管道文件描述符）。
tee函数成功时返回在两个文件描述符之间复制的数据数量（字节数）。
返回0表示没有复制任何数据。tee失败时返回-1并设置errno。
*/

// 以下代码利用tee函数和splice函数，
// 实现了Linux下tee程序（同时输出数据到终端和文件的程序，不要和tee函数混淆）的基本功能。
// filename:tee.cpp
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
/*
// 同时输出数据到终端和文件的程序
// 代码原理：
// 标准输入--->stdout[1]--->stdout[0]--(tee)-->file[1]--->file[0]--->filefd
//                            |
//                        （splice）
//                            |
//                            V
//                          标准输出
// tee先使用，由于tee不消耗数据所以slice还可以将stdout的内容重定向到标准输出
*/
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("usage:%s<file>\n", argv[0]);
        return 1;
    }

    int filefd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
    assert(filefd > 0);
    int pipefd_stdout[2];
    int ret = pipe(pipefd_stdout);
    assert(ret != -1);
    int pipefd_file[2];
    ret = pipe(pipefd_file);
    assert(ret != -1);

    /*将标准输入内容输入管道pipefd_stdout*/
    ret = splice(STDIN_FILENO, NULL, pipefd_stdout[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
    assert(ret != -1);

    // 注意以下用的tee函数，其不会消耗管道中的数据
    /*将管道pipefd_stdout的输出复制到管道pipefd_file的输入端*/
    ret = tee(pipefd_stdout[0], pipefd_file[1], 32768, SPLICE_F_NONBLOCK);
    assert(ret != -1);

    // splice和tee用的是同一个源管道
    /*将管道pipefd_file的输出定向到文件描述符filefd上，从而将标准输入的内容写入文件*/
    ret = splice(pipefd_file[0], NULL, filefd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
    assert(ret != -1);

    /*将管道pipefd_stdout的输出定向到标准输出，其内容和写入文件的内容完全一致*/
    ret = splice(pipefd_stdout[0], NULL, STDOUT_FILENO, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
    assert(ret != -1);
    close(filefd);
    close(pipefd_stdout[0]);
    close(pipefd_stdout[1]);
    close(pipefd_file[0]);
    close(pipefd_file[1]);
    return 0;
}
