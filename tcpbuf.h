/*
  tcpbuf.h
  Structure for managing a buffer for TCP send and receive operations

  TODO: have alloc() return the pointer, and NULL if not enough space
  
  TODO: for "efficiency" turn this into a circular array with delayed shifting
  on tcpbuf_alloc only when necessary.

  POSSIBLY TODO: resizing features?
*/

#ifndef __TCPBUF_H
#define __TCPBUF_H

#include<sys/types.h>

typedef struct tcpbuf
{
    int len;       //length in bytes of the buffer
    int pos;       //index of the next available byte
    u_char *data;  //the bytes
} tcpbuf;

//Creates a tcpbuf of the specified length.
void tcpbuf_construct(tcpbuf *t, int len);

//Destroys a tcpbuf.
void tcpbuf_destruct(tcpbuf *t);

//Allocate space to enqueue something directly
int tcpbuf_alloc(tcpbuf *t, int len, u_char **ptr);
//Stores address of writable block in ptr
//Returns size of writable block

//Deallocate space if part of what was allocated wasn't used
int tcpbuf_dealloc(tcpbuf *t, int len);
//Returns the amount fo bytes deallocated

//Enqueue bytes onto the buffer.
int tcpbuf_enqueue(tcpbuf *t, int len, const u_char *data);
//Returns the number of bytes enqueued

//Dequeue bytes from the buffer.
int tcpbuf_dequeue(tcpbuf *t, int len);
//Returns number of bytes dequeued.

//A pointer to the actual data in the buffer
const char* tcpbuf_data(const tcpbuf *t);
//Warning: may not be thread safe if any other tcpbuf method is called

//Number of bytes filled in the buffer
int tcpbuf_filled(const tcpbuf *t);

//Number of bytes available in the buffer
int tcpbuf_space(const tcpbuf *t);
 
//Test whether the buffer is empty
int tcpbuf_empty(const tcpbuf *t);

//Test whether the buffer is full
int tcpbuf_full(const tcpbuf *t);

#endif
