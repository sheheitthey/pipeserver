/* lineclient.h
   A TCP client which sends and receives logical packets delimited by \n.
*/

#ifndef __LINECLIENT_H
#define __LINECLIENT_H

#include"pollconn.h"

typedef enum lineclient_event
{
    LINECLIENT_NULLEVENT,
    LINECLIENT_CLOSED,
    LINECLIENT_ERROR,
    LINECLIENT_SHUTDOWN,
    LINECLIENT_LINE
} lineclient_event;

typedef struct lineclient
{
    pollconn c;
    int next_pos;
    int closed;
    int shutdown;
    int error;
    char delim;
} lineclient;

//Construct a lineclient
void lineclient_construct(lineclient *lc, struct pollfd *pfd,
                          int sendbuf_len, int recvbuf_len);

//Destruct a lineclient
void lineclient_destruct(lineclient *lc);

//Set the remote address by using DNS resolution
int lineclient_resolve(lineclient *lc, const char *host, u_int16_t port);

//Connect a lineclient
int lineclient_connect(lineclient *lc);

//Operate a lineclient
void lineclient_operate(lineclient *lc);

//Enqueue a line
int lineclient_enqueue(lineclient *lc, const char *data, int len);
//NOTE: The line should not contain the delimiter
//Returns -1 on failure

//Dequeue a line or event
int lineclient_dequeue(lineclient *lc);

//Signal a connection ready to close
int lineclient_shutdown(lineclient *lc);

//Immediately close a connection
int lineclient_close(lineclient *lc);

//Find the next event on a client
lineclient_event lineclient_getevent(const lineclient *lc);

//Store a pointer to the first complete line for a client
int lineclient_getline(const lineclient *lc, const char **data, int *len);
//Returns 1 if a line is found, 0 if not, -1 if error

//Find out the errno of an error which occurred on a connection
int lineclient_geterror(const lineclient *lc);

//Access the actual pollconn for a lineclient, usually for logging purposes
const pollconn* lineclient_conn(const lineclient *lc);

#endif
