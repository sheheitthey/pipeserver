/* pollsock.h
   structure for representing a TCP socket for use with poll()

   should be nothing more than wrappers around syscalls to take care of
   buffering and the fact that we're using poll()
*/

#ifndef __POLLSOCK_H
#define __POLLSOCK_H

#include<sys/types.h> 
#include<netinet/in.h>
#include<netinet/tcp.h>
#include<netdb.h>
#include<sys/poll.h>
#include"tcpbuf.h"

typedef struct pollsock
{
    struct pollfd *pfd; //A pointer to the pfd entry for this socket
                        //in some pollfd array
    tcpbuf sendbuf;
    tcpbuf recvbuf;
    
} pollsock;

//Create a pollsock
void pollsock_construct(pollsock *s,
                        struct pollfd *pfd, int sendbuf_len, int recvbuf_len);

//Destroy a pollsock
void pollsock_destruct(pollsock *s);

//Reset a pollsock
void pollsock_reset(pollsock *s);

//Check the revents of a pollsock for an event: POLLIN or POLLOUT
int pollsock_revents(const pollsock *s, int event);

//Allocate the socket of a pollsock
int pollsock_socket(pollsock *s);

//Bind a pollsock to an address
int pollsock_bind(pollsock *s, const struct sockaddr_in *addr);

//Make a pollsock a listen socket
int pollsock_listen(pollsock *s, int backlog);

//Accept a connection on a pollsock
int pollsock_accept(pollsock *s, pollsock *n,
                    struct sockaddr_in *addr, socklen_t *addrlen);
//Returns the fd for the accepted socket or -1 on error
//If n is not NULL, will call accepted() on n

//Initialize a connection which was just established
void pollsock_established(pollsock *s);

//Initialize a connection which was just accepted
void pollsock_accepted(pollsock *s, int sock);

//Connect a pollsock
int pollsock_connect(pollsock *s, const struct sockaddr_in *addr);

//Send on a pollsock
int pollsock_send(pollsock *s);

//Receive on a pollsock
int pollsock_recv(pollsock *s);

//Shut down the send side of a pollsock
int pollsock_shutdown(pollsock *s);

//Fully close a pollsock
int pollsock_close(pollsock *s);

//Enqueue data to send on a pollsock
int pollsock_enqueue(pollsock *s, int len, const u_char *data);

//Dequeue data received on a pollsock
int pollsock_dequeue(pollsock *s, int len);

//Pointer to the actual data in the sendbuf
const u_char* pollsock_sendbuf_data(const pollsock *s);

//Pointer to the actual data in the recvbuf
const u_char* pollsock_recvbuf_data(const pollsock *s);

//How much of the send buffer is filled
int pollsock_sendbuf_filled(const pollsock *s);

//How much of the recv buffer is filled
int pollsock_recvbuf_filled(const pollsock *s);

//How much of the send buffer is available
int pollsock_sendbuf_space(const pollsock *s);

//How much of the recv buffer is available
int pollsock_recvbuf_space(const pollsock *s);

//Whether the send buffer is full
int pollsock_sendbuf_full(const pollsock *s);

//Whether the recv buffer is full
int pollsock_recvbuf_full(const pollsock *s);

//Whether the send buffer is empty
int pollsock_sendbuf_empty(const pollsock *s);

//Whether the recv buffer is empty
int pollsock_recvbuf_empty(const pollsock *s);

//The actual socket (file descriptor) of a pollsock
int pollsock_sock(const pollsock *s);

//Get the local address of a pollsock
int pollsock_sockname(const pollsock *s,
                      struct sockaddr_in *name, socklen_t *namelen);

//Get the remote address of a pollsock
int pollsock_peername(const pollsock *s,
                      struct sockaddr_in *name, socklen_t *namelen);

#endif
