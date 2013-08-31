/* pollsock.c
   
*/

#include"pollsock.h"
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<errno.h>

//Create a pollsock
void
pollsock_construct(pollsock *s,
                   struct pollfd *pfd, int sendbuf_len, int recvbuf_len)
{
    s->pfd=pfd;
    s->pfd->fd=-1;
    s->pfd->events=0;
    s->pfd->revents=0;
    tcpbuf_construct(&s->sendbuf, sendbuf_len);
    tcpbuf_construct(&s->recvbuf, recvbuf_len);
}

//Destroy a pollsock
void
pollsock_destruct(pollsock *s)
{
    tcpbuf_destruct(&s->sendbuf);
    tcpbuf_destruct(&s->recvbuf);
}

//Reset a pollsock
void
pollsock_reset(pollsock *s)
{
    s->pfd->fd=-1;
    s->pfd->events=0;
    s->pfd->revents=0;
    tcpbuf_dequeue(&s->sendbuf, tcpbuf_filled(&s->sendbuf));
    tcpbuf_dequeue(&s->recvbuf, tcpbuf_filled(&s->recvbuf));
}

//Check the revents of a pollsock for an event: POLLIN or POLLOUT
int
pollsock_revents(const pollsock *s, int event)
{
    return s->pfd->revents&event;
}

//Allocate the socket of a pollsock
int
pollsock_socket(pollsock *s)
{
    s->pfd->fd=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    return s->pfd->fd;
}

//Bind a pollsock to an address
int
pollsock_bind(pollsock *s, const struct sockaddr_in *addr)
{
    return bind(s->pfd->fd, (const struct sockaddr*)addr, sizeof(*addr));
}

#if 0
int
pollsock_bind(pollsock *s, u_int32_t s_addr, u_int16_t port)
{
    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
    addr.sin_addr.s_addr=htonl(s_addr);
    return bind(s->pfd->fd, (const struct sockaddr*)&addr, sizeof(addr));
}
#endif

//Make a pollsock a listen socket
int
pollsock_listen(pollsock *s, int backlog)
{
    int ret;
    if((ret=listen(s->pfd->fd, backlog))==0)
        s->pfd->events|=POLLIN;
    return ret;
}

//Accept a connection on a pollsock
int
pollsock_accept(pollsock *s, pollsock *n,
                struct sockaddr_in *addr, socklen_t *addrlen)
{
    int ret;
    if(addr && addrlen)
        *addrlen=sizeof(*addr);
    ret=accept(s->pfd->fd, (struct sockaddr*)addr, addrlen);
    if(ret!=-1 && n)
        pollsock_accepted(n, ret);
    return ret;
}

//Initialize a connection which was just established
void
pollsock_established(pollsock *s)
{
    s->pfd->events=0;
    if(tcpbuf_filled(&s->sendbuf)>0)
        s->pfd->events|=POLLOUT;
    if(tcpbuf_space(&s->recvbuf)>0)
        s->pfd->events|=POLLIN;
}

//Initialize a connection which was just accepted
void
pollsock_accepted(pollsock *s, int sock)
{
    s->pfd->fd=sock;
    pollsock_established(s);
}

//Connect a pollsock
int
pollsock_connect(pollsock *s, const struct sockaddr_in *addr)
{
    int ret;
    ret=connect(s->pfd->fd, (const struct sockaddr*)addr, sizeof(*addr));
    if(ret==0)
        pollsock_established(s);
    else //need to watch for POLLOUT for successful connection
        s->pfd->events|=POLLOUT;
    return ret;
}

#if 0
int
pollsock_connect(pollsock *s, const char *host, u_int16_t port)
{
    struct sockaddr_in addr;
    struct addrinfo hints, *server;
    const int service_len=6;
    char service[service_len];
    int ret;
    memset(&hints, sizeof(hints), 0);
    hints.ai_flags=AI_CANONNAME;
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=IPPROTO_TCP;
    snprintf(service, service_len, "%u", port);
    ret=getaddrinfo(host, service, &hints, &server);
    if(ret!=0)
        return ret; //error resolving host
    ret=connect(s->pfd->fd, server->ai_addr, server->ai_addrlen);
    freeaddrinfo(server);
    if(ret==0)
    {
        s->pfd->events=0;
        if(tcpbuf_filled(&s->sendbuf)>0)
            s->pfd->events|=POLLOUT;
        if(tcpbuf_spaced(&s->recvbuf)>0)
            s->pfd->events|=POLLIN;
    }
    else //need to watch for POLLOUT for successful connection
        s->pfd->events|=POLLOUT;
    return ret;
}
#endif

//Send on a pollsock
int
pollsock_send(pollsock *s)
{
    int ret;
    ret=send(s->pfd->fd, s->sendbuf.data, s->sendbuf.pos, 0);
    if(ret>=0)
    {
        tcpbuf_dequeue(&s->sendbuf, ret);
        if(tcpbuf_empty(&s->sendbuf))
            s->pfd->events&=~POLLOUT;
    }
    return ret;
}

//Receive on a pollsock
int
pollsock_recv(pollsock *s)
{
    int ret, buflen;
    u_char *buf;
    buflen=tcpbuf_alloc(&s->recvbuf, tcpbuf_space(&s->recvbuf), &buf);
    ret=recv(s->pfd->fd, buf, buflen, 0);
    if(ret>=0)
    {
        tcpbuf_dealloc(&s->recvbuf, buflen-ret);
        if(tcpbuf_full(&s->recvbuf) || ret==0)
            s->pfd->events&=~POLLIN;
    }
    // TODO: dealloc since nothing was read
    return ret;
}

//Shut down the send side of a pollsock
int
pollsock_shutdown(pollsock *s)
{
    int ret;
    ret=shutdown(s->pfd->fd, SHUT_WR);
    if(ret==0)
        s->pfd->events&=~POLLOUT;
    return ret;
}

//Fully close a pollsock
int
pollsock_close(pollsock *s)
{
    int ret;
    ret=close(s->pfd->fd);
    s->pfd->fd=-1;
    s->pfd->events=0;
    s->pfd->revents=0;
    return ret;
}

//Enqueue data to send on a pollsock
int
pollsock_enqueue(pollsock *s, int len, const u_char *data)
{
    int ret;
    ret=tcpbuf_enqueue(&s->sendbuf, len, data);
    if(tcpbuf_filled(&s->sendbuf)>0)
        s->pfd->events|=POLLOUT;
    return ret;
}

//Dequeue data received on a pollsock
int
pollsock_dequeue(pollsock *s, int len)
{
    int ret;
    ret=tcpbuf_dequeue(&s->recvbuf, len);
    if(tcpbuf_space(&s->recvbuf)>0)
        s->pfd->events|=POLLIN;
    return ret;
}

//Pointer to the actual data in the sendbuf
const u_char*
pollsock_sendbuf_data(const pollsock *s)
{
    return tcpbuf_data(&s->sendbuf);
}
//Pointer to the actual data in the recvbuf
const u_char*
pollsock_recvbuf_data(const pollsock *s)
{
    return tcpbuf_data(&s->recvbuf);
}

//How much of the send buffer is filled
int
pollsock_sendbuf_filled(const pollsock *s)
{
    return tcpbuf_filled(&s->sendbuf);
}

//How much of the recv buffer is filled
int
pollsock_recvbuf_filled(const pollsock *s)
{
    return tcpbuf_filled(&s->recvbuf);
}

//How much of the send buffer is available
int
pollsock_sendbuf_space(const pollsock *s)
{
    return tcpbuf_space(&s->sendbuf);
}

//How much of the recv buffer is available
int
pollsock_recvbuf_space(const pollsock *s)
{
    return tcpbuf_space(&s->recvbuf);
}

//Whether the send buffer is full
int
pollsock_sendbuf_full(const pollsock *s)
{
    return tcpbuf_full(&s->sendbuf);
}

//Whether the recv buffer is full
int
pollsock_recvbuf_full(const pollsock *s)
{
    return tcpbuf_full(&s->recvbuf);
}

//Whether the send buffer is empty
int
pollsock_sendbuf_empty(const pollsock *s)
{
    return tcpbuf_empty(&s->sendbuf);
}

//Whether the recv buffer is empty
int
pollsock_recvbuf_empty(const pollsock *s)
{
    return tcpbuf_empty(&s->recvbuf);
}

//The actual socket (file descriptor) of a pollsock
int
pollsock_sock(const pollsock *s)
{
    return s->pfd->fd;
}

//Get the local address of a pollsock
int
pollsock_sockname(const pollsock *s,
                  struct sockaddr_in *name, socklen_t *namelen)
{
    return getsockname(s->pfd->fd, (struct sockaddr*)name, namelen);
}

//Get the remote address of a pollsock
int
pollsock_peername(const pollsock *s,
                  struct sockaddr_in *name, socklen_t *namelen)
{
    return getpeername(s->pfd->fd, (struct sockaddr*)name, namelen);
}
