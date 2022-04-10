#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <vector>

#include "log.h"
#include "conn.h"
#include "mgr.h"
#include "processpool.h"

using std::vector;

static const char* version = "1.0";

// __FILE__:用以指示本行语句所在源文件的文件名
// __LINE__:用以指示本行语句在源文件中的位置信息

static void usage( const char* prog )
{
    log( LOG_INFO, __FILE__, __LINE__,  "usage: %s [-h] [-v] [-f config_file]", prog );
}

int main( int argc, char* argv[] )
{
    char cfg_file[1024];
    memset( cfg_file, '\0', 100 );
    int option;
    while ( ( option = getopt( argc, argv, "f:xvh" ) ) != -1 )
    {
        switch ( option )
        {
            case 'x':
            {
                set_loglevel( LOG_DEBUG );
                break;
            }
            case 'v':
            {
                log( LOG_INFO, __FILE__, __LINE__, "%s %s", argv[0], version );
                return 0;
            }
            case 'h':
            {
                usage( basename( argv[ 0 ] ) );
                return 0;
            }
            case 'f':
            {
                memcpy( cfg_file, optarg, strlen( optarg ) );
                break;
            }
            case '?':
            {
                log( LOG_ERR, __FILE__, __LINE__, "un-recognized option %c", option );
                usage( basename( argv[ 0 ] ) );
                return 1;
            }
        }
    }    

    if( cfg_file[0] == '\0' )
    {
        log( LOG_ERR, __FILE__, __LINE__, "%s", "please specifiy the config file" );
        return 1;
    }
    int cfg_fd = open( cfg_file, O_RDONLY );
    if( !cfg_fd )
    {
        log( LOG_ERR, __FILE__, __LINE__, "read config file met error: %s", strerror( errno ) );
        return 1;
    }
    struct stat ret_stat;
    if( fstat( cfg_fd, &ret_stat ) < 0 )
    {
        log( LOG_ERR, __FILE__, __LINE__, "read config file met error: %s", strerror( errno ) );
        return 1;
    }
    // buf用于存储配置文件的信息
    char* buf = new char [ret_stat.st_size + 1];
    memset( buf, '\0', ret_stat.st_size + 1 );
    ssize_t read_sz = read( cfg_fd, buf, ret_stat.st_size );
    if ( read_sz < 0 )
    {
        log( LOG_ERR, __FILE__, __LINE__, "read config file met error: %s", strerror( errno ) );
        return 1;
    }
    // 以下用于解析xml文件，host在前面的mgr.h文件中定义
    vector< host > balance_srv; // 负载均衡服务器
    vector< host > logical_srv; // 逻辑服务器
    host tmp_host;
    memset( tmp_host.m_hostname, '\0', 1024 );
    char* tmp_hostname;
    char* tmp_port;
    char* tmp_conncnt;
    bool opentag = false;
    char* tmp = buf;    // tmp指向config.xml文件的内容
    char* tmp2 = NULL;
    char* tmp3 = NULL;
    char* tmp4 = NULL;
    // 在源字符串tmp中查找第一个"\n"字符的位置，若未找到则返回一个空指针
    // 下面的host获取采用了状态机的思想来读取xml的信息，可能无法处理空格
    // 看博客有推荐tinyxml这个库，读取起来会更舒服
    while( tmp2 = strpbrk( tmp, "\n" ) )
    {
        *tmp2++ = '\0';
        // 找第一个目标串在模式串中出现的位置
        // strstr(str1,str2)函数用于判断字符串str2是否是str1的子串。
        // 如果是，则该函数返回str2在str1首次出现的地址，否则返回null
        if( strstr( tmp, "<logical_host>" ) )
        {
            if( opentag )
            {
                log( LOG_ERR, __FILE__, __LINE__, "%s", "parse config file failed" );
                return 1;
            }
            opentag = true;
        }
        else if( strstr( tmp, "</logical_host>" ) ) // 有/符号
        {
            if( !opentag )
            {
                log( LOG_ERR, __FILE__, __LINE__, "%s", "parse config file failed" );
                return 1;
            }
            logical_srv.push_back( tmp_host );
            memset( tmp_host.m_hostname, '\0', 1024 );
            opentag = false;    // 读取完毕一个逻辑主机地址后，将标签关闭
        }
        else if( tmp3 = strstr( tmp, "<name>" ) )
        {
            tmp_hostname = tmp3 + 6;    // 跳过"<name>"这些字符
            tmp4 = strstr( tmp_hostname, "</name>" );
            if( !tmp4 )
            {
                log( LOG_ERR, __FILE__, __LINE__, "%s", "parse config file failed" );
                return 1;
            }
            *tmp4 = '\0';   // 跳过"</name>"这些字符，提前截断
            memcpy( tmp_host.m_hostname, tmp_hostname, strlen( tmp_hostname ) );
        }
        else if( tmp3 = strstr( tmp, "<port>" ) )
        {
            tmp_port = tmp3 + 6;
            tmp4 = strstr( tmp_port, "</port>" );
            if( !tmp4 )
            {
                log( LOG_ERR, __FILE__, __LINE__, "%s", "parse config file failed" );
                return 1;
            }
            *tmp4 = '\0';
            tmp_host.m_port = atoi( tmp_port );
        }
        else if( tmp3 = strstr( tmp, "<conns>" ) )
        {
            tmp_conncnt = tmp3 + 7;
            tmp4 = strstr( tmp_conncnt, "</conns>" );
            if( !tmp4 )
            {
                log( LOG_ERR, __FILE__, __LINE__, "%s", "parse config file failed" );
                return 1;
            }
            *tmp4 = '\0';
            tmp_host.m_conncnt = atoi( tmp_conncnt );
        }
        else if( tmp3 = strstr( tmp, "Listen" ) )
        {
            tmp_hostname = tmp3 + 6;
            tmp4 = strstr( tmp_hostname, ":" );
            if( !tmp4 )
            {
                log( LOG_ERR, __FILE__, __LINE__, "%s", "parse config file failed" );
                return 1;
            }
            *tmp4++ = '\0';
            tmp_host.m_port = atoi( tmp4 ); // 端口号
            memcpy( tmp_host.m_hostname, tmp3, strlen( tmp3 ) );
            balance_srv.push_back( tmp_host );  // 复制当前主机地址，存入负载均衡服务器列表中
            memset( tmp_host.m_hostname, '\0', 1024 );  // 将tmp清空，准备读取下一个服务器信息
        }
        // 回到了第二行，跳过了一个回车
        tmp = tmp2;
    }

    if( balance_srv.size() == 0 || logical_srv.size() == 0 )
    {
        log( LOG_ERR, __FILE__, __LINE__, "%s", "parse config file failed" );
        return 1;
    }
    const char* ip = balance_srv[0].m_hostname;
    int port = balance_srv[0].m_port;

    int listenfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert( listenfd >= 0 );
 
    int ret = 0;
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    ret = bind( listenfd, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret != -1 );

    ret = listen( listenfd, 5 );
    assert( ret != -1 );

    //memset( cfg_host.m_hostname, '\0', 1024 );
    //memcpy( cfg_host.m_hostname, "127.0.0.1", strlen( "127.0.0.1" ) );
    //cfg_host.m_port = 54321;
    //cfg_host.m_conncnt = 5;
    processpool< conn, host, mgr >* pool = processpool< conn, host, mgr >::create( listenfd, logical_srv.size() );
    if( pool )
    {
        pool->run( logical_srv );
        delete pool;
    }

    close( listenfd );
    return 0;
}
