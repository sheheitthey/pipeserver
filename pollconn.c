#include"pollconn.h"
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/socket.h>
#include<netdb.h>

static int pollconn_fsend(pollconn *c);
static int pollconn_frecv(pollconn *c);
static int pollconn_disc(pollconn *c);

//Set the saddr and daddr after a connection has been established
static void pollconn_setaddr(pollconn *c);

//Create a pollconn
void
pollconn_construct(pollconn *c,
                        struct pollfd *pfd, int sendbuf_len, int recvbuf_len)
{
    pollsock_construct(&c->s, pfd, sendbuf_len, recvbuf_len);
    pollconn_reset(c);
}

//Destroy a pollconn
void
pollconn_destruct(pollconn *c)
{
    pollconn_reset(c);
    pollsock_destruct(&c->s);
}

//Reset a pollconn
void
pollconn_reset(pollconn *c)
{
    if(pollsock_sock(&c->s)!=-1)
        pollsock_close(&c->s);
    pollsock_reset(&c->s);
    c->saddr.sin_family=AF_INET;
    c->saddr.sin_port=htons(0);
    c->saddr.sin_addr.s_addr=htonl(0);
    c->saddr_len=sizeof(c->saddr);
    c->daddr.sin_family=AF_INET;
    c->daddr.sin_port=htons(0);
    c->daddr.sin_addr.s_addr=htonl(0);
    c->daddr_len=sizeof(c->daddr);
    c->flags.bound=0;
    c->flags.connected=0;
    c->flags.connecting=0;
    c->flags.listening=0;
    c->flags.recvdone=0;
    c->flags.senddone=0;
    c->flags.finsent=0;
    c->flags.did_accept=0;
    c->flags.did_bind=0;
    c->flags.did_close=0;
    c->flags.did_connect=0;
    c->flags.did_recv=0;
    c->flags.did_send=0;
    c->flags.did_shutdown=0;
    c->flags.saddr_dirty=0;
    c->ret_accept=0;
    c->ret_send=0;
    c->ret_recv=0;
    c->err_accept=0;
    c->err_bind=0;
    c->err_close=0;
    c->err_connect=0;
    c->err_recv=0;
    c->err_send=0;
    c->err_shutdown=0;
}

//Check the revents of a pollconn for an event: POLLIN or POLLOTU
int
pollconn_revents(const pollconn *c, int event)
{
    return pollsock_revents(&c->s, event);
}

//Set the local address
void
pollconn_set_saddr(pollconn *c, u_int32_t ip_addr, u_int16_t port)
{
    c->saddr.sin_addr.s_addr=htonl(ip_addr);
    c->saddr.sin_port=htons(port);
    c->flags.saddr_dirty=1;
}

//Set the remote address
void
pollconn_set_daddr(pollconn *c, u_int32_t ip_addr, u_int16_t port)
{
    c->daddr.sin_addr.s_addr=htonl(ip_addr);
    c->daddr.sin_port=htons(port);
}

//Set the remote address by using DNS resolution
int
pollconn_resolve(pollconn *c, const char *host, u_int16_t port)
{
    const int service_len=6;
    char service[service_len];
    struct addrinfo hints, *server;
    const struct sockaddr_in *addr;
    int ret;
    memset(&hints, sizeof(hints), 0);
    hints.ai_flags=AI_CANONNAME;
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=IPPROTO_TCP;
    snprintf(service, service_len, "%u", port);
    ret=getaddrinfo(host, service, &hints, &server);
    if(ret!=0)
        return ret;
    addr=(const struct sockaddr_in*)server->ai_addr;
    c->daddr.sin_addr.s_addr=addr->sin_addr.s_addr;
    c->daddr.sin_port=addr->sin_port;
    freeaddrinfo(server);
    return ret;
}

//Enqueue data to send on a pollconn
int
pollconn_enqueue(pollconn *c, int len, const u_char *data)
{
    if(c->flags.senddone)
        return 0;
    return pollsock_enqueue(&c->s, len, data);
}

//Dequeue data received on a pollconn
int
pollconn_dequeue(pollconn *c, int len)
{
    return pollsock_dequeue(&c->s, len);
}

//Signal a connection ready to close (nothing left to send)
void
pollconn_shutdown(pollconn *c)
{
    c->flags.senddone=1;
}

//Make a listen pollconn
int
pollconn_listen(pollconn *c)
{
    int ret;
    ret=pollsock_listen(&c->s, SOMAXCONN);
    if(ret==0)
        c->flags.listening=1;
    return ret;
}

//Accept on a listen pollconn
int
pollconn_accept(pollconn *c, pollconn *n)
{
    int ret;
    ret=pollsock_accept(&c->s, NULL, NULL, NULL);
    c->ret_accept=ret;
    if(ret!=-1)
    {
        c->err_accept=0;
        if(n)
            pollconn_accepted(n, ret);
    }
    else
        c->err_accept=errno;
    return ret;
}

//Initialize a recently accepted connection
void pollconn_accepted(pollconn *c, int sock)
{
    pollsock_accepted(&c->s, sock);
    c->flags.connected=1;
    c->flags.connecting=0;
    c->flags.listening=0;
    c->flags.recvdone=0;
    c->flags.senddone=0;
    pollconn_setaddr(c);
}

//Allocate a socket for a pollconn
int pollconn_socket(pollconn *c)
{
    int ret;
    ret=pollsock_socket(&c->s);
    if(ret!=-1)
    {
        if(fcntl(pollsock_sock(&c->s), F_SETFL, O_NONBLOCK)==-1)
        {
            pollsock_close(&c->s);
            return -1;
        }
    }
    return ret;
}

//Bind a pollconn
int pollconn_bind(pollconn *c)
{
    int ret;
    c->flags.saddr_dirty=0;
    ret=pollsock_bind(&c->s, &c->saddr);
    if(ret==0)
    {
        c->err_bind=0;
        c->flags.bound=1;
    }
    else
        c->err_bind=errno;
    return ret;
}

//Connect a pollconn
int
pollconn_connect(pollconn *c)
{
    int ret;
    c->flags.connecting=1;
    ret=pollsock_connect(&c->s, &c->daddr);
    if(ret==0)
    {
        c->err_connect=0;
        c->flags.connected=1;
        c->flags.connecting=0;
        c->flags.listening=0;
        c->flags.recvdone=0;
        c->flags.senddone=0;
        pollconn_setaddr(c);
    }
    else
        c->err_connect=errno;
    return ret;
}

//Send data on a pollconn
int
pollconn_send(pollconn *c)
{
    int ret;
    ret=pollsock_send(&c->s);
    c->ret_send=ret;
    c->flags.did_shutdown=0;
    c->flags.did_close=0;
    if(ret>=0)
    {
        c->err_send=0;
        if(c->flags.senddone
           && pollsock_sendbuf_empty(&c->s)
           && !c->flags.finsent)
        {
            c->flags.did_shutdown=1;
            pollconn_fsend(c);
        }
    }
    else
        c->err_send=errno;
    return ret;
}

//Receive data on a pollconn
int
pollconn_recv(pollconn *c)
{
    int ret;
    ret=pollsock_recv(&c->s);
    c->ret_recv=ret;
    c->flags.did_close=0;
    if(ret>=0)
    {
        c->err_recv=0;
        if(ret==0)
        {
            c->flags.recvdone=1;
            pollconn_frecv(c);
        }
    }
    else
        c->err_recv=errno;
    return ret;
}

//Immediately close a pollconn
int
pollconn_close(pollconn *c)
{
    int ret;
    ret=pollsock_close(&c->s);
    if(ret==0)
        c->err_close=0;
    else
        c->err_close=errno;
    c->flags.bound=0;
    c->flags.connected=0;
    c->flags.listening=0;
    c->flags.recvdone=1;
    c->flags.senddone=1;
    c->flags.finsent=1;
    return ret;
}

//What to do on POLLIN for a non-listen pollconn
void
pollconn_pollin(pollconn *c)
{
    c->flags.did_recv=0;
    if(!pollsock_revents(&c->s, POLLIN))
        return;
    c->flags.did_recv=1;
    pollconn_recv(c);
}

//What to do on a POLLOUT for a non-listen pollconn
void
pollconn_pollout(pollconn *c)
{
    int ret;
    c->flags.did_send=0;
    if(!pollsock_revents(&c->s, POLLOUT))
        return;
    c->flags.did_send=1;
    ret=pollconn_send(c);
    if(!c->flags.connected && ret!=-1)
    {
        pollsock_established(&c->s);
        c->flags.connected=1;
        c->flags.connecting=0;
        c->flags.listening=0;
        c->flags.recvdone=0;
        c->flags.senddone=0;
        pollconn_setaddr(c);
    }
}

//Does everything that needs to be done with a pollconn
void
pollconn_operate(pollconn *c)
{
    int ret;
    c->flags.did_accept=0;
    c->flags.did_bind=0;
    c->flags.did_close=0;
    c->flags.did_connect=0;
    c->flags.did_recv=0;
    c->flags.did_send=0;
    c->flags.did_shutdown=0;
    if(!pollconn_ready(c))
    {
        //if(c->saddr.sin_port!=htons(0)
        //   && c->saddr.sin_addr.s_addr!=htonl(INADDR_ANY))
        if(c->flags.saddr_dirty)
        {
            c->flags.did_bind=1;
            ret=pollconn_bind(c);
            // everything after this redundant?
            if(ret!=-1)
                c->err_bind=0;
            else
                c->err_bind=errno;
        }
        if(c->daddr.sin_port!=htons(0)
           && c->daddr.sin_addr.s_addr!=htonl(INADDR_ANY)
           && !c->flags.connecting)
        {
            c->flags.did_connect=1;
            ret=pollconn_connect(c);
            // everything after this redundant?
            if(ret!=-1)
                c->err_connect=0;
            else
                c->err_connect=errno;
        }
        if(pollconn_revents(c, POLLOUT))
            pollconn_pollout(c);
    }
    else if(c->flags.listening)
    {
        if(pollconn_revents(c, POLLIN))
        {
            c->flags.did_accept=1;
            pollconn_accept(c, NULL);
        }
    }
    else
    {
        if(pollconn_revents(c, POLLOUT))
            pollconn_pollout(c);
        else if(c->flags.senddone
                && pollsock_sendbuf_empty(&c->s)
                && !c->flags.finsent)
            pollconn_fsend(c);
        if(pollconn_revents(c, POLLIN))
            pollconn_pollin(c);
    }
}

//Pointer to the actual data in the sendbuf
const u_char*
pollconn_sendbuf_data(const pollconn *c)
{
    return pollsock_sendbuf_data(&c->s);
}

//Pointer to the actual data in the recvbuf
const u_char*
pollconn_recvbuf_data(const pollconn *c)
{
    return pollsock_recvbuf_data(&c->s);
}

//How much of the send buffer is filled
int
pollconn_sendbuf_filled(const pollconn *c)
{
    return pollsock_sendbuf_filled(&c->s);
}

//How much of the recv buffer is filled
int
pollconn_recvbuf_filled(const pollconn *c)
{
    return pollsock_recvbuf_filled(&c->s);
}

//How much of the send buffer is available
int
pollconn_sendbuf_space(const pollconn *c)
{
    return pollsock_sendbuf_space(&c->s);
}

//How much of the recv buffer is available
int
pollconn_recvbuf_space(const pollconn *c)
{
    return pollsock_recvbuf_space(&c->s);
}

//Whether the send buffer is full
int
pollconn_sendbuf_full(const pollconn *c)
{
    return pollsock_sendbuf_full(&c->s);
}

//Whether the recv buffer is full
int
pollconn_recvbuf_full(const pollconn *c)
{
    return pollsock_recvbuf_full(&c->s);
}

//Whether the send buffer is empty
int
pollconn_sendbuf_empty(const pollconn *c)
{
    return pollsock_sendbuf_empty(&c->s);
}

//Whether the recv buffer is empty
int
pollconn_recvbuf_empty(const pollconn *c)
{
    return pollsock_recvbuf_empty(&c->s);
}

//Whether a pollconn is ready (listening or connected)
int
pollconn_ready(const pollconn *c)
{
    return (c->flags.connected || c->flags.listening);
}

//Whether a pollconn has been established and closed
int
pollconn_closed(const pollconn *c)
{
    if(c->flags.senddone && c->flags.recvdone)
        return 1;
    return 0;
}

//Whether a pollconn has been completely processed, ready for reset or destruct
int
pollconn_done(const pollconn *c)
{
    if(pollconn_closed(c) && pollconn_recvbuf_empty(c))
        return 1;
    return 0;
}

//The actual socket (file descriptor) of a pollconn
int pollconn_sock(const pollconn *c)
{
    return pollsock_sock(&c->s);
}

//The local IP address of a pollconn
u_int32_t
pollconn_src_ip(const pollconn *c)
{
    return c->saddr.sin_addr.s_addr;
}

//The local port of a pollconn
u_int16_t
pollconn_src_port(const pollconn *c)
{
    return c->saddr.sin_port;
}

//The remote IP address of a pollconn
u_int32_t
pollconn_dst_ip(const pollconn *c)
{
    return c->daddr.sin_addr.s_addr;
}

//The remote port of a pollconn
u_int32_t
pollconn_dst_port(const pollconn *c)
{
    return c->daddr.sin_port;
}

//Whether operate() called accept()
unsigned int
pollconn_did_accept(const pollconn *c)
{
    return c->flags.did_accept;
}

//Whether operate() called bind()
unsigned int
pollconn_did_bind(const pollconn *c)
{
    return c->flags.did_bind;
}

//Whether operate(), pollin(), or pollout() called close()
unsigned int
pollconn_did_close(const pollconn *c)
{
    return c->flags.did_close;
}

//Whether operate() called connect()
unsigned int
pollconn_did_connect(const pollconn *c)
{
    return c->flags.did_connect;
}

//Whether operate() or pollin() called recv()
unsigned int
pollconn_did_recv(const pollconn *c)
{
    return c->flags.did_recv;
}

//Whether operate() or pollout() called send()
unsigned int
pollconn_did_send(const pollconn *c)
{
    return c->flags.did_send;
}

//Whether operate() or pollout() called shutdown()
unsigned int
pollconn_did_shutdown(const pollconn *c)
{
    return c->flags.did_shutdown;
}

//Value of errno last time accept() was called
unsigned int
pollconn_err_accept(const pollconn *c)
{
    return c->err_accept;
}

//Value of errno last time bind() was called
unsigned int
pollconn_err_bind(const pollconn *c)
{
    return c->err_bind;
}

//Value of errno last time close() was called
unsigned int
pollconn_err_close(const pollconn *c)
{
    return c->err_close;
}

//Value of errno last time connect() was called
unsigned int
pollconn_err_connect(const pollconn *c)
{
    return c->err_connect;
}

//Value of errno last time recv() was called
unsigned int
pollconn_err_recv(const pollconn *c)
{
    return c->err_recv;
}

//Value of errno last time send() was called
unsigned int
pollconn_err_send(const pollconn *c)
{
    return c->err_send;
}

//Value of errno last time shutdown() was called
unsigned int
pollconn_err_shutdown(const pollconn *c)
{
    return c->err_shutdown;
}

//The accepted socket last time accept() was called
int
pollconn_accept_sock(const pollconn *c)
{
    return c->ret_accept;
}

//The number of bytes received last time recv() was called
int
pollconn_recv_bytes(const pollconn *c)
{
    return c->ret_recv;
}

//The number of bytes sent last time send() was called
int
pollconn_send_bytes(const pollconn *c)
{
    return c->ret_send;
}

static int
pollconn_fsend(pollconn *c)
{
    int ret;
    ret=pollsock_shutdown(&c->s);
    if(ret==0)
    {
        c->flags.finsent=1;
        c->err_shutdown=0;
        if(c->flags.recvdone)
        {
            c->flags.did_close=1;
            return pollconn_disc(c);
        }
    }
    else
        c->err_shutdown=errno;
    return ret;
}

static int
pollconn_frecv(pollconn *c)
{
    if(c->flags.senddone && pollsock_sendbuf_empty(&c->s))
    {
        c->flags.did_close=1;
        return pollconn_disc(c);
    }
    return 0;
}

static int
pollconn_disc(pollconn *c)
{
    int ret;
    ret=pollsock_close(&c->s);
    if(ret==0)
        c->err_close=0;
    else
        c->err_close=errno;
    c->flags.connected=0;
    c->flags.bound=0;
    c->flags.connecting=0;
    return ret;
}

static void
pollconn_setaddr(pollconn *c)
{
    pollsock_sockname(&c->s, &c->saddr, &c->saddr_len);
    pollsock_peername(&c->s, &c->daddr, &c->daddr_len);
}
