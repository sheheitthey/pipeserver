/* pollconn.h
   Structure for managing the state of a connection

   adds state and logging to pollsock (as well as managing state)

   TODO: - ensure that only operate() will cause a syscall to be called? or at
           least for connect()...may make things cleaner (already done?)
         - bulletproof everything

   TODO: - although not strictly needed, make it convenient to see if a
           connection JUST got connected after operate() or pollout()

   NOTE: - operations which do NOT depend on poll() returning
           (namely shutdown()) may be delayed indefinitely, depending on the
           timeout value given to poll(). To avoid this, use either a small
           enough timeout or call operate() or send() after calling shutdown().
*/

#ifndef __POLLCONN_H
#define __POLLCON_H

#include"pollsock.h"

typedef struct pollconn_flags
{
    unsigned int bound         :1;
    unsigned int connected     :1;
    unsigned int connecting    :1;
    unsigned int listening     :1;
    unsigned int recvdone      :1;
    unsigned int senddone      :1;
    unsigned int finsent       :1;
    unsigned int did_accept    :1;
    unsigned int did_bind      :1;
    unsigned int did_close     :1;
    unsigned int did_connect   :1;
    unsigned int did_recv      :1;
    unsigned int did_send      :1;
    unsigned int did_shutdown  :1;
    unsigned int saddr_dirty   :1;
    unsigned int padding       :17;
} pollconn_flags;

typedef struct pollconn
{
    pollsock s;
    struct sockaddr_in saddr; //source address
    socklen_t saddr_len;
    struct sockaddr_in daddr; //destination address
    socklen_t daddr_len;
    pollconn_flags flags;
    int ret_accept;
    int ret_send;
    int ret_recv;
    int err_accept;
    int err_bind;
    int err_close;
    int err_connect;
    int err_recv;
    int err_send;
    int err_shutdown;
} pollconn;

//Create a pollconn
void pollconn_construct(pollconn *c,
                        struct pollfd *pfd, int sendbuf_len, int recvbuf_len);

//Destroy a pollconn
void pollconn_destruct(pollconn *c);

//Reset a pollconn
void pollconn_reset(pollconn *c);
//If pollconn is listening or connected, will close and reopen the socket

//Check the revents of a pollconn for an event: POLLIN or POLLOUT
int pollconn_revents(const pollconn *c, int event);

//Functions which are guaranteed not to invoke socket functions
//For example, send(), recv(), bind(), listen(), accept(), shutdown(), close()
//These can be invoked later with pollin(), pollout(), or just operate()

//Set the local address
void pollconn_set_saddr(pollconn *c, u_int32_t ip_addr, u_int16_t port);
//Causes a bind() to eventually happen

//Set the remote address
void pollconn_set_daddr(pollconn *c, u_int32_t ip_addr, u_int16_t port);
//Causes a connect() to eventually happen, listen sockets shouldn't use this

//Set the remote address by using DNS resolution
int pollconn_resolve(pollconn *c, const char *host, u_int16_t port);
//Causes a connect() to eventually happen, listen sockets shouldn't use this

//Enqueue data to send on a pollconn
int pollconn_enqueue(pollconn *c, int len, const u_char *data);

//Dequeue data received on a pollconn
int pollconn_dequeue(pollconn *c, int len);

//Signal a connection ready to close (nothing left to send, or no more listen)
void pollconn_shutdown(pollconn *c);


//Functions for listen sockets (they do socket operations)

//Make a listen pollconn
int pollconn_listen(pollconn *c);

//Accept on a listen pollconn
int pollconn_accept(pollconn *c, pollconn *n);
//Returns the socket descriptor or -1 on error
//If n is not NULL, calls accepted() on n

//Initialize a recently accepted connection
void pollconn_accepted(pollconn *c, int sock);


//Functions which actually do socket operations

//Allocate a socket for a pollonn
int pollconn_socket(pollconn *c);

//Bind a pollconn
int pollconn_bind(pollconn *c);

//Connect a pollconn
int pollconn_connect(pollconn *c);

//Send data on a pollconn
int pollconn_send(pollconn *c);

//Receive data on a pollconn
int pollconn_recv(pollconn *c);

//Immediately close a pollconn
int pollconn_close(pollconn *c);

//What to do on POLLIN for a non-listen pollconn
void pollconn_pollin(pollconn *c);

//What to do on a POLLOUT for a non-listen pollconn
void pollconn_pollout(pollconn *c);

//Does everything that needs to be done with a pollconn
void pollconn_operate(pollconn *c);

//Pointer to the actual data in the sendbuf
const u_char* pollconn_sendbuf_data(const pollconn *c);

//Pointer to the actual data in the recvbuf
const u_char* pollconn_recvbuf_data(const pollconn *c);

//How much of the send buffer is filled
int pollconn_sendbuf_filled(const pollconn *c);

//How much of the recv buffer is filled
int pollconn_recvbuf_filled(const pollconn *c);

//How much of the send buffer is available
int pollconn_sendbuf_space(const pollconn *c);

//How much of the recv buffer is available
int pollconn_recvbuf_space(const pollconn *c);

//Whether the send buffer is full
int pollconn_sendbuf_full(const pollconn *c);

//Whether the recv buffer is full
int pollconn_recvbuf_full(const pollconn *c);

//Whether the send buffer is empty
int pollconn_sendbuf_empty(const pollconn *c);

//Whether the recv buffer is empty
int pollconn_recvbuf_empty(const pollconn *c);

//Whether a pollconn is ready (listening or connected)
int pollconn_ready(const pollconn *c);

//Whether a pollconn has been established and closed
int pollconn_closed(const pollconn *c);
//Does not apply to listen sockets

//Whether a pollconn has been completely processed, ready for reset or destruct
int pollconn_done(const pollconn *c);

//The actual socket (file descriptor) of a pollconn
int pollconn_sock(const pollconn *c);

//The local IP address of a pollconn
u_int32_t pollconn_src_ip(const pollconn *c);

//The local port of a pollconn
u_int16_t pollconn_src_port(const pollconn *c);

//The remote IP address of a pollconn
u_int32_t pollconn_dst_ip(const pollconn *c);

//The remote port of a pollconn
u_int32_t pollconn_dst_port(const pollconn *c);

//Whether operate() called accept()
unsigned int pollconn_did_accept(const pollconn *c);

//Whether operate() called bind()
unsigned int pollconn_did_bind(const pollconn *c);

//Whether operate(), pollin(), or pollout() called close()
unsigned int pollconn_did_close(const pollconn *c);

//Whether operate() called connect()
unsigned int pollconn_did_connect(const pollconn *c);

//Whether operate() or pollin() called recv()
unsigned int pollconn_did_recv(const pollconn *c);

//Whether operate() or pollout() called send()
unsigned int pollconn_did_send(const pollconn *c);

//Whether operate() or pollout() called shutdown()
unsigned int pollconn_did_shutdown(const pollconn *c);

//Value of errno last time accept() was called
unsigned int pollconn_err_accept(const pollconn *c);

//Value of errno last time bind() was called
unsigned int pollconn_err_bind(const pollconn *c);

//Value of errno last time close() was called
unsigned int pollconn_err_close(const pollconn *c);

//Value of errno last time connect() was called
unsigned int pollconn_err_connect(const pollconn *c);

//Value of errno last time recv() was called
unsigned int pollconn_err_recv(const pollconn *c);

//Value of errno last time send() was called
unsigned int pollconn_err_send(const pollconn *c);

//Value of errno last time shutdown() was called
unsigned int pollconn_err_shutdown(const pollconn *c);

//The accepted socket last time accept() was called
int pollconn_accept_sock(const pollconn *c);

//The number of bytes received last time recv() was called
int pollconn_recv_bytes(const pollconn *c);

//The number of bytes sent last time send() was called
int pollconn_send_bytes(const pollconn *c);

#endif
