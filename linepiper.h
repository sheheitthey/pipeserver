/* linepiper.h
   A simple interface for buffering and transferring delimited lines
   through file handles (which are meant to be pipes).
*/

#ifndef __LINEPIPER_H
#define __LINEPIPER_H

#include"tcpbuf.h"

typedef struct linepiper
{
    int fd_in;
    int fd_out;
    tcpbuf buf_in;
    tcpbuf buf_out;
    int in_pos;
    char delim;
} linepiper;

//Create a pipeserver
void linepiper_construct(linepiper *lp, int fd_in, int fd_out,
                          int inbuf_len, int outbuf_len);

//Destroy a pipeserver
void linepiper_destruct(linepiper *lp);

//Read what's available to the linepiper
int linepiper_read(linepiper *lp);

//Write what's possible from the linepiper
int linepiper_write(linepiper *lp);

//Enqueue a line to be sent from the linepiper
int linepiper_enqueue(linepiper *lp, const char *data, int len);
//Returns the number of bytes enqueued or -1 for error (not enough space)
//NOTE: The line shouldn't contain a the delimiter

//Dequeue a line received on the linepiper
int linepiper_dequeue(linepiper *lp);
//Returns 1 on success, 0 on failure

//Read the line that's about to be dequeued from the linepiper
int linepiper_line(linepiper *lp, const char **data, int *len);
//Returns 1 on success, 0 if no lines

#endif
