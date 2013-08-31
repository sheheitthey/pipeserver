/*
  pollset.h
*/

#ifndef __POLLSET_H
#define __POLLSET_H

#include"pollpool.h"

typedef int pollset_handle;

typedef struct pollset
{
    pollpool pp;
    pollconn **handles;
    int handles_len;
} pollset;

//Create a pollset
void pollset_construct(pollset *ps, struct pollfd *ufds, int len,
                       int sendbuf_len, int recvbuf_len);

//Destroy a pollset
void pollset_destruct(pollset *ps);

//Change the maximum size of a pollset
void pollset_resize(pollset *ps, struct pollfd *ufds, int newlen);

//Add a pollconn
pollset_handle pollset_addconn(pollset *ps);

//Add an accepted pollconn
pollset_handle pollset_addaccepted(pollset *ps, int fd);

//Delete a pollconn
int pollset_delconn(pollset *ps, pollset_handle psh);

//Access a pollconn
pollconn *pollset_conn(const pollset *ps, pollset_handle psh);

//Find out the handle of a pollconn belonging to this pollset
pollset_handle pollset_connhandle(const pollset *ps, pollconn *c);

//Poll everything in the pollset
int pollset_poll(pollset *ps, int timeout);

//Operate everything in the pollset
void pollset_operate(pollset *ps);

#endif
