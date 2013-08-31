#ifndef __PIPESERVER_MSG_H
#define __PIPESERVER_MSG_H

#include"lineserver.h"

typedef enum pipeserver_event
{
    // incoming events
    PIPESERVER_NULLEVENT,
    PIPESERVER_ACCEPTED,
    PIPESERVER_CLOSED,
    PIPESERVER_CLOSING,
    PIPESERVER_ERROR,
    PIPESERVER_RECEIVED,
    // outgoing events
    PIPESERVER_CLOSE,
    PIPESERVER_SEND,
    PIPESERVER_SHUTDOWN
} pipeserver_event;

int pipeserver_msg_build(char *buf, int buflen,
                         pipeserver_event e, pollset_handle h,
                         const char *data, int len);
//Returns -1 if error or not enough space in buffer, number of bytes
//written otherwise

int pipeserver_msg_read(const char *msg, int len,
                        pipeserver_event *e, pollset_handle *h,
                        const char **data, int *datalen);
//Returns 1 if message makes sense, 0 otherwise

#endif
