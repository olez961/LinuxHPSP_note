/*
用户信息对于服务器程序的安全性来说是很重要的，比如大部分服务器就必须以root身份启动，但不能以root身份运行。
下面这一组函数可以
获取和设置当前进程的真实用户ID（UID）、有效用户ID（EUID）、真实组ID（GID）和有效组ID（EGID）：
#include<sys/types.h>
#include<unistd.h>
uid_t getuid();         // 获取真实用户ID
uid_t geteuid();        // 获取有效用户ID
gid_t getgid();         // 获取真实组ID
gid_t getegid();        // 获取有效组ID
int setuid(uid_t uid);  // 设置真实用户ID
int seteuid(uid_t uid); // 设置有效用户ID
int setgid(gid_t gid);  // 设置真实组ID
int setegid(gid_t gid); // 设置有效组ID
需要指出的是，一个进程拥有两个用户ID：UID和EUID。
EUID存在的目的是方便资源访问：它使得运行程序的用户拥有该程序的有效用户的权限。

比如su程序，任何用户都可以使用它来修改自己的账户信息，但修改账户时su程序不得不访问/etc/passwd文件，
而访问该文件是需要root权限的。那么以普通用户身份启动的su程序如何能访问/etc/passwd文件呢？

窍门就在EUID。用ls命令可以查看到，su程序的所有者是root，并且它被设置了set-user-id标志。
这个标志表示，任何普通用户运行su程序时，其有效用户就是该程序的所有者root。
那么，根据有效用户的含义，任何运行su程序的普通用户都能够访问/etc/passwd文件。
有效用户为root的进程称为特权进程（privileged processes）。
EGID的含义与EUID类似：给运行目标程序的组用户提供有效组的权限。
*/


#include<unistd.h>
#include<stdio.h>
// 以下代码展示了如何将以root身份启动的进程切换为以一个普通用户身份运行。
static bool switch_to_user(uid_t user_id, gid_t gp_id)
{
    /*先确保目标用户不是root*/
    if ((user_id == 0) && (gp_id == 0))
    {
        return false;
    }
    /*确保当前用户是合法用户：root或者目标用户*/
    gid_t gid = getgid();
    uid_t uid = getuid();
    // 下面这个逻辑表达式为真说明当前用户既是root组又是目标用户
    // 若目标用户有root权限，此处应该为真，函数返回false
    if (((gid != 0) || (uid != 0)) &&((gid != gp_id) || (uid != user_id)))
    {
        return false;
    }
    // 能走到这一步说明上面逻辑表达式错了，那就只有root和目标用户两种可能性了
    // ，而且上面还排除了目标用户拥有root权限的可能性
    /*如果不是root，则已经是目标用户*/
    if (uid != 0)
    {
        return true;
    }
    // 以下两函数失败返回-1
    /*切换到目标用户*/
    if ((setgid(gp_id) <0) || (setuid(user_id) <0))
    {
        return false;
    }
    return true;
}
