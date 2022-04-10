#include <exception>
#include <errno.h>
#include <string.h>
#include "conn.h"
#include "log.h"
#include "fdwrapper.h"

conn::conn()
{
    m_srvfd = -1;
    m_clt_buf = new char[ BUF_SIZE ];
    if( !m_clt_buf )
    {
        throw std::exception();
    }
    m_srv_buf = new char[ BUF_SIZE ];
    if( !m_srv_buf )
    {
        throw std::exception();
    }
    reset();
}

conn::~conn()
{
    delete [] m_clt_buf;
    delete [] m_srv_buf;
}

void conn::init_clt( int sockfd, const sockaddr_in& client_addr )
{
    m_cltfd = sockfd;
    m_clt_address = client_addr;
}

void conn::init_srv( int sockfd, const sockaddr_in& server_addr )
{
    m_srvfd = sockfd;
    m_srv_address = server_addr;
}

void conn::reset()
{
    m_clt_read_idx = 0;
    m_clt_write_idx = 0;
    m_srv_read_idx = 0;
    m_srv_write_idx = 0;
    m_srv_closed = false;
    m_cltfd = -1;
    memset( m_clt_buf, '\0', BUF_SIZE );
    memset( m_srv_buf, '\0', BUF_SIZE );
}

RET_CODE conn::read_clt()
{
    int bytes_read = 0;
    while( true )
    {
        if( m_clt_read_idx >= BUF_SIZE )    //如果读入的数据大于BUF_SIZE
        {
            log( LOG_ERR, __FILE__, __LINE__, "%s", "the client read buffer is full, let server write" );
            return BUFFER_FULL;
        }

        // 因为存在分包的问题（recv所读入的并非是size的大小）
        // 因此我们根据recv的返回值进行循环读入，直到读满m_clt_buf或者recv的返回值为0（数据被读完）
        bytes_read = recv( m_cltfd, m_clt_buf + m_clt_read_idx, BUF_SIZE - m_clt_read_idx, 0 );
        if ( bytes_read == -1 )
        {
            // 非阻塞情况下： 
            // EAGAIN表示没有数据可读，请尝试再次调用,而在阻塞情况下，如果被中断，则返回EINTR;
            // EWOULDBLOCK等同于EAGAIN
            if( errno == EAGAIN || errno == EWOULDBLOCK )
            {
                break;
            }
            return IOERR;
        }
        else if ( bytes_read == 0 )
        {
            return CLOSED;
        }

        m_clt_read_idx += bytes_read;
    }
    return ( ( m_clt_read_idx - m_clt_write_idx ) > 0 ) ? OK : NOTHING;
}

RET_CODE conn::read_srv()
{
    int bytes_read = 0;
    while( true )
    {
        if( m_srv_read_idx >= BUF_SIZE )
        {
            log( LOG_ERR, __FILE__, __LINE__, "%s", "the server read buffer is full, let client write" );
            return BUFFER_FULL;
        }

        bytes_read = recv( m_srvfd, m_srv_buf + m_srv_read_idx, BUF_SIZE - m_srv_read_idx, 0 );
        if ( bytes_read == -1 )
        {
            if( errno == EAGAIN || errno == EWOULDBLOCK )
            {
                break;
            }
            return IOERR;
        }
        else if ( bytes_read == 0 )
        {
            log( LOG_ERR, __FILE__, __LINE__, "%s", "the server should not close the persist connection" );
            return CLOSED;
        }

        m_srv_read_idx += bytes_read;
    }
    return ( ( m_srv_read_idx - m_srv_write_idx ) > 0 ) ? OK : NOTHING;
}

RET_CODE conn::write_srv()
{
    int bytes_write = 0;
    while( true )
    {
        if( m_clt_read_idx <= m_clt_write_idx )
        {
            m_clt_read_idx = 0;
            m_clt_write_idx = 0;
            return BUFFER_EMPTY;
        }

        bytes_write = send( m_srvfd, m_clt_buf + m_clt_write_idx, m_clt_read_idx - m_clt_write_idx, 0 );
        if ( bytes_write == -1 )
        {
            if( errno == EAGAIN || errno == EWOULDBLOCK )
            {
                return TRY_AGAIN;
            }
            log( LOG_ERR, __FILE__, __LINE__, "write server socket failed, %s", strerror( errno ) );
            return IOERR;
        }
        else if ( bytes_write == 0 )
        {
            return CLOSED;
        }

        m_clt_write_idx += bytes_write;
    }
}

RET_CODE conn::write_clt()
{
    int bytes_write = 0;
    while( true )
    {
        if( m_srv_read_idx <= m_srv_write_idx )
        {
            m_srv_read_idx = 0;
            m_srv_write_idx = 0;
            return BUFFER_EMPTY;
        }

        bytes_write = send( m_cltfd, m_srv_buf + m_srv_write_idx, m_srv_read_idx - m_srv_write_idx, 0 );
        if ( bytes_write == -1 )
        {
            if( errno == EAGAIN || errno == EWOULDBLOCK )
            {
                return TRY_AGAIN;
            }
            log( LOG_ERR, __FILE__, __LINE__, "write client socket failed, %s", strerror( errno ) );
            return IOERR;
        }
        else if ( bytes_write == 0 )
        {
            return CLOSED;
        }

        m_srv_write_idx += bytes_write;
    }
}
