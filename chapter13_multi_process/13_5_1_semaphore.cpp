#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// 第一个能被我运行出来的程序，值得纪念
// 输出结果如下
/*
ubuntu2004@ubuntu2004-virtual-machine:
    ~/LinuxHPSP_note/chapter13_multi_process$ g++ -g 13_5_1_semaphore.cpp -o test
ubuntu2004@ubuntu2004-virtual-machine:
    ~/LinuxHPSP_note/chapter13_multi_process$ ./test
parent try to get binary sem
parent get the sem and would release it after 5 seconds
child try to get binary sem
child get the sem and would release it after 5 seconds
*/
// 以下联合体用于semctl函数中
union semun
{
     int val;                   /*用于SETVAL命令*/
     struct semid_ds* buf;      /*用于IPC_STAT和IPC_SET命令*/
     unsigned short int* array; /*用于GETALL和SETALL命令*/
     struct seminfo* __buf;     /*用于IPC_INFO命令*/
};

// 实现一次pv操作
void pv( int sem_id, int op )
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;      // 信号量集中信号量的编号，0表示信号集中的第一个信号量
    sem_b.sem_op = op;      // 指定操作类型，可以是正数、0、负数
    // 以下参数可选值是IPC_NOWAIT和SEM_UNDO。
    // IPC_NOWAIT的含义是，无论信号量操作是否成功，semop调用都将立即返回，这类似于非阻塞I/O操作。
    // SEM_UNDO的含义是，当进程退出时取消正在进行的semop操作。
    sem_b.sem_flg = SEM_UNDO;
    // semop系统调用用于改变信号量的值，即执行PV操作
    // 第一个参数是信号集标识符号
    // 第二个参数指向一个sembuf结构体类型的数组（在本例中该数组只有一个元素），就是上面定义的那个
    // 第三个参数指定要操作的执行个数，即第二个参数指向数组中的元素个数
    // 该系统调用对sem_b数组中的元素依次进行操作，且该过程是原子操作
    // 若成功返回0，失败返回-1并置error，失败时数组中的所有操作都不会被执行
    semop( sem_id, &sem_b, 1 );
}

int main( int argc, char* argv[] )
{
    // semget系统调用创建一个新的信号量集，或者获取一个已经存在的信号量集
    // 函数第一个参数是key，表示键值，类似于文件描述符，来标识全局唯一的信号量集
    // 使用IPC_PRIVATE作为键值（其值为0），semget会创建一个新的信号量
    // 第二个参数指出信号量集中信号量的个数
    // 第三个参数指定一组标志，该标志含义和系统调用open的mode参数相同
    int sem_id = semget( IPC_PRIVATE, 1, 0666 );

    union semun sem_un;
    sem_un.val = 1;
    // semctl系统调用允许调用者对信号量进行直接控制
    // sem_id参数是semget返回的信号量集标识符，用以指定被操作的信号量集
    // 第二个参数sem_num指定被操作的信号量在信号量集中的编号
    // 第三个参数指定要执行的命令，有的命令需要调用者传递第四个参数
    // 第四个参数的类型由用户自己定义，不过上面已经给出了官方的推荐格式
    // 此处的第三个参数SETVAL表示将信号量的semval设置为sem_un.val
    // 同时内核中的semid_ds.sem_ctime被更新
    semctl( sem_id, 0, SETVAL, sem_un );

    // 创建子进程，注意fork出来的子进程直接从fork后的语句开始执行，且一般子进程先执行
    pid_t id = fork();
    // id小于零说明出错了
    if( id < 0 )
    {
        return 1;
    }
    // 进入子进程
    else if( id == 0 )
    {
        printf( "child try to get binary sem\n" );
        // 进行一个PV操作，以下表示一次请求操作
        pv( sem_id, -1 );
        printf( "child get the sem and would release it after 5 seconds\n" );
        sleep( 5 );
        // 以下表示一次释放操作
        pv( sem_id, 1 );
        exit( 0 );
    }
    // 父进程执行（因为fork在父进程中返回的是子进程的PID，不出错的话都是大于0的）
    else
    {
        printf( "parent try to get binary sem\n" );
        pv( sem_id, -1 );
        printf( "parent get the sem and would release it after 5 seconds\n" );
        sleep( 5 );
        pv( sem_id, 1 );
    }

    // 等待子进程运行结束，第一个参数说明waitpid等待的是子进程结束
    // 第二个参数指示子进程的退出信息储存的位置（一个指针）
    // 第三个参数是选项字段，为0好像是忽略该值的意思
    waitpid( id, NULL, 0 );
    // 以下第三个参数表示立即移除信号量集，并唤醒所有等待该信号量集的进程
    semctl( sem_id, 0, IPC_RMID, sem_un );
    return 0;
}
