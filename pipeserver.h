/* pipeserver.h
   Translate multiplexed incoming events from TCP streams passing through the
   server into lines representing the events.
*/

#ifndef __PIPESERVER_H
#define __PIPESERVER_H

#include"pipeserver_msg.h"
#include"linepiper.h"

typedef struct pipeserver
{
    lineserver ls;
    linepiper lp;
    pollset_handle accepted;
    pollset_handle in_conn;
    pollset_handle out_conn;
    pipeserver_event in_event;
    pipeserver_event out_event;
    const char *in_data;
    int in_datalen;
    const char *out_data;
    int out_datalen;
    char *msg;
    int msg_len;
} pipeserver;

//Create a pipeserver
void pipeserver_construct(pipeserver *p, struct pollfd *ufds, int len,
                          int sendbuf_len, int recvbuf_len,
                          int fd_in, int fd_out,
                          int inbuf_len, int outbuf_len);

//Destroy a pipeserver
void pipeserver_destruct(pipeserver *p);

//Cause a pipeserver to start listening
int pipeserver_listen(pipeserver *p, u_int16_t port);

//Cause a pipeserver to bind to an interface rather than INADDR_ANY
int pipeserver_bind(pipeserver *p, u_int32_t ip_addr);

//Operate all the clients and the listen socket on a pipeserver
void pipeserver_operate(pipeserver *p);

//Read what's available to the pipeserver
int pipeserver_read(pipeserver *p);

//Write what's possible from the pipeserver
int pipeserver_write(pipeserver *p);

//Process an incoming event to the pipeserver
int pipeserver_process_in(pipeserver *p);
//This means to enqueue_in() the next event, and dequeue_in() if successful
//Returns the number of bytes enqueued, -1 if buffer full, 0 if no events

//Process an outgoing event from the pipeserver
int pipeserver_process_out(pipeserver *p);
//This means to enqueue_out() the next event, and dequeue_out() if successful
//Returns 1 on success, -1 on error, 0 if no events

// Methods mainly for figuring out what's going on if something goes wrong

//Find out the next incoming event of the pipeserver
int pipeserver_event_in(const pipeserver *p,
                        pollset_handle *h, pipeserver_event *e);

//Find out the next outgoing event of the pipeserver
int pipeserver_event_out(const pipeserver *p,
                         pollset_handle *h, pipeserver_event *e);

//Find out the next incoming line from a pipeserver client
int pipeserver_line_in(const pipeserver *p, pollset_handle *h,
                       const char **data, int *len);
//This can technically peek to see what line a client has received even if
//the current incoming event is not a RECEIVED event. Don't depend on this.

//Find out the next outgoing line from a pipeserver client
int pipeserver_line_out(const pipeserver *p, pollset_handle *h,
                        const char **data, int *len);
//NOTE: Unlike line_in(), line_out() is NOT able to peek to see the line if
//the current outgoing event is not a SEND event!

//Find out the error of a client
int pipeserver_error(const pipeserver *p, pollset_handle h);

// These methods are available just to be able to do things differently in
// exceptional situations

//Enqueue an incoming event to the outgoing pipe
int pipeserver_enqueue_in(pipeserver *p);
//Return number of bytes enqueued, -1 if error, 0 if no event

//Dequeue an outgoing event from the incoming pipe
int pipeserver_dequeue_out(pipeserver *p);
//Return 1 on success, 0 if no event

//Dequeue an incoming event
int pipeserver_dequeue_in(pipeserver *p);
//Return 1 if success, 0 if no event

//Enqueue/execute an outgoing event
int pipeserver_enqueue_out(pipeserver *p);
//Return 1 if success, -1 if error, 0 if no event

//Enqueue an event to the outgoing pipe
int pipeserver_enqueue_event(pipeserver *p,
                             pipeserver_event e, pollset_handle h,
                             const char *data, int len); // if PIPESERVER_SEND

//Enqueue an outgoing line
int pipeserver_enqueue_line(pipeserver *p, pollset_handle h,
                            const char *data, int len);

//Signal that we're done sending data
int pipeserver_shutdown(pipeserver *p, pollset_handle h);

//Immediately close and delete a connection
int pipeserver_close(pipeserver *p, pollset_handle h);

#endif
