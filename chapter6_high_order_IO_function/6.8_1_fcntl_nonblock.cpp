/*
fcntl函数，正如其名字（file control）描述的那样，提供了对文件描述符的各种控制操作。
另外一个常见的控制文件描述符属性和行为的系统调用是ioctl，而且ioctl比fcntl能够执行更多的控制。
但是，对于控制文件描述符常用的属性和行为，fcntl函数是由POSIX规范指定的首选方法。

#include<fcntl.h>
int fcntl(int fd,int cmd,…);
fd参数是被操作的文件描述符，cmd参数指定执行何种类型的操作。
根据操作类型的不同，该函数可能还需要第三个可选参数arg。

fcntl函数成功时的返回值有许多种情况，失败则返回-1并设置errno。
*/

// 在网络编程中，fcntl函数通常用来将一个文件描述符设置为非阻塞的，如下所示

// 将文件描述符设置为非阻塞的
#include<fcntl.h>
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);      /*获取文件描述符旧的状态标志*/
    int new_option = old_option | O_NONBLOCK; /*设置非阻塞标志*/
    fcntl(fd, F_SETFL, new_option);
    return old_option; /*返回文件描述符旧的状态标志，以便日后恢复该状态标志*/
}
