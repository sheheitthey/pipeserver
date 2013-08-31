/* lineserver.h
   A TCP server which sends and receives logical packets delimited by \n.
   The various events from all connections (namely, lines) are serialized.
*/

#ifndef __LINESERVER_H
#define __LINESERVER_H

#include"pollset.h"

typedef struct lineserver_client
{
    pollset_handle h;
    int next_pos;
    int closed;
    int shutdown;
    int error;
} lineserver_client;

typedef enum lineserver_event
{
    LINESERVER_NULLEVENT,
    //LINESERVER_ACCEPTED,
    LINESERVER_CLOSED,
    LINESERVER_ERROR,
    LINESERVER_SHUTDOWN,
    LINESERVER_LINE
} lineserver_event;

typedef struct lineserver
{
    pollset ps;
    pollconn ls;
    lineserver_client *clients;
    int num_clients;
    int max_clients;
    int next_conn_pos;
    char delim;
} lineserver;

//Create a lineserver
void lineserver_construct(lineserver *l, struct pollfd *ufds, int len,
                          int sendbuf_len, int recvbuf_len);

//Destroy a lineserver
void lineserver_destruct(lineserver *l);

//Cause a lineserver to start listening
int lineserver_listen(lineserver *l, u_int16_t port);

//Cause a server to bind to an interface rather than INADDR_ANY
int lineserver_bind(lineserver *l, u_int32_t ip_addr);

//Operate all the clients on the server
void lineserver_operate(lineserver *l);

//Accept a connection, if there is one
pollset_handle lineserver_accept(lineserver *l);
//Returns a handle to the accepted connection, -1 if none

//Enqueue a line
int lineserver_enqueue(lineserver *l, pollset_handle h,
                       const char *data, int len);
//NOTE: The line should not contain the delimiter
//Returns -1 on failure

//Dequeue a line or event
int lineserver_dequeue(lineserver *l, pollset_handle h);
//Returns -1 if failure, 0 if nothing to dequeue, 1 if success
//When the LINESERVER_CLOSED or LINESERVER_ERROR event is dequeued, the client
//is gone

//Signal that we're done sending data
int lineserver_shutdown(lineserver *l, pollset_handle h);

//Immediately close and delete a connection
int lineserver_close(lineserver *l, pollset_handle h);

//Find the next event on a client
lineserver_event lineserver_getevent(const lineserver *l, pollset_handle h);

//Store a pointer to the first complete line for a connection
int lineserver_getline(const lineserver *l, pollset_handle h,
                    const char **data, int *len);
//Returns 1 if a line is found, 0 if not, -1 if error

//Find out the errno of an error which occurred on a connection
int lineserver_geterror(const lineserver *l, pollset_handle h);

//Return a handle to the next client which has an event
pollset_handle lineserver_getnextclient(const lineserver *l);
//Returns -1 if no clients have a line

//Combine nextclient() and getevent() to store the next client and event
int lineserver_getnextevent(const lineserver *l, pollset_handle *h,
                            lineserver_event *e);
//Returns 1 if h and e are valid, 0 otherwise

//Access the actual pollconn for a handle, usually for logging purposes
const pollconn* lineserver_conn(const lineserver *l, pollset_handle h);

#endif
