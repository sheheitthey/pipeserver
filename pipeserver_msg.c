#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include"pipeserver_msg.h"

static const char pipeserver_nullevent[]="NULL";
static const char pipeserver_accepted[]="ACCEPTED";
static const char pipeserver_closed[]="CLOSED";
static const char pipeserver_closing[]="CLOSING";
static const char pipeserver_error[]="ERROR";
static const char pipeserver_received[]="RECEIVED";
static const char pipeserver_close[]="CLOSE";
static const char pipeserver_send[]="SEND";
static const char pipeserver_shutdown[]="SHUTDOWN";

static const char* pipeserver_event_str(pipeserver_event e);

static int read_pipeserver_event(const char *event, int len,
                                 pipeserver_event *e);
//Returns 1 if success, 0 otherwise

static int read_pollset_handle(const char *handle, int len, pollset_handle *h);
//Returns 1 on success, 0 on failure

static int bufs_match(const char *buf1, const char *buf2, int len);
//Returns 1 if they match, 0 otherwise

static int get_msg_lens(const char *msg, int len,
                        int *event_len, int *handle_len);
//Returns 1 on success, 0 on failure

int
pipeserver_msg_build(char *buf, int buflen,
                     pipeserver_event e, pollset_handle h,
                     const char *data, int len)
{
    const char *event_str;
    int ret, i;
    event_str=pipeserver_event_str(e);
    ret=snprintf(buf, buflen, "%s %d", event_str, h);
    if(ret>=buflen || ret<0)
        return -1;
    if(e!=PIPESERVER_RECEIVED && e!=PIPESERVER_SEND)
        return ret;
    buf[ret]=' ';
    ret++;
    buflen-=ret;
    buf+=ret;
    for(i=0; i<len && i<buflen; i++)
        buf[i]=data[i];
    return ret+len;
}

int
pipeserver_msg_read(const char *msg, int len,
                    pipeserver_event *e, pollset_handle *h,
                    const char **data, int *datalen)
{
    int event_len, handle_len, total_len;
    if(!get_msg_lens(msg, len, &event_len, &handle_len))
        return 0;
    if(!read_pipeserver_event(msg, event_len, e))
        return 0;
    if(!read_pollset_handle(msg+event_len+1, handle_len, h))
        return 0;
    if(*e!=PIPESERVER_RECEIVED && *e!=PIPESERVER_SEND)
        return 1;
    total_len=event_len+handle_len+2;
    if(len<total_len)
        return 0;
    if(msg[total_len-1]!=' ')
        return 0;
    *data=msg+total_len;
    *datalen=len-total_len;
    return 1;
}


static const char*
pipeserver_event_str(pipeserver_event e)
{
    switch(e)
    {
    case PIPESERVER_ACCEPTED:
        return pipeserver_accepted;
    case PIPESERVER_CLOSED:
        return pipeserver_closed;
    case PIPESERVER_CLOSING:
        return pipeserver_closing;
    case PIPESERVER_ERROR:
        return pipeserver_error;
    case PIPESERVER_RECEIVED:
        return pipeserver_received;
    case PIPESERVER_CLOSE:
        return pipeserver_close;
    case PIPESERVER_SEND:
        return pipeserver_send;
    case PIPESERVER_SHUTDOWN:
        return pipeserver_shutdown;
    default:
        return pipeserver_nullevent;
    }
}

static int
read_pipeserver_event(const char *event, int len, pipeserver_event *e)
{
    if(len==strlen(pipeserver_accepted) &&
       bufs_match(pipeserver_accepted, event, len))
    {
        *e=PIPESERVER_ACCEPTED;
        return 1;
    }
    if(len==strlen(pipeserver_closed) &&
       bufs_match(pipeserver_closed, event, len))
    {
        *e=PIPESERVER_CLOSED;
        return 1;
    }
    if(len==strlen(pipeserver_closing) &&
       bufs_match(pipeserver_closing, event, len))
    {
        *e=PIPESERVER_CLOSING;
        return 1;
    }
    if(len==strlen(pipeserver_error) &&
       bufs_match(pipeserver_error, event, len))
    {
        *e=PIPESERVER_ERROR;
        return 1;
    }
    if(len==strlen(pipeserver_received) &&
       bufs_match(pipeserver_received, event, len))
    {
        *e=PIPESERVER_RECEIVED;
        return 1;
    }
    if(len==strlen(pipeserver_close) &&
       bufs_match(pipeserver_close, event, len))
    {
        *e=PIPESERVER_CLOSE;
        return 1;
    }
    if(len==strlen(pipeserver_send) &&
       bufs_match(pipeserver_send, event, len))
    {
        *e=PIPESERVER_SEND;
        return 1;
    }
    if(len==strlen(pipeserver_shutdown) &&
       bufs_match(pipeserver_shutdown, event, len))
    {
        *e=PIPESERVER_SHUTDOWN;
        return 1;
    }
    return 0;
}

static int
read_pollset_handle(const char *handle, int len, pollset_handle *h)
{
    *h=0;
    int place=1;
    for(len--; len>=0; len--)
    {
        if(!isdigit(handle[len]))
            return 0;
        (*h)+=place*(handle[len]-'0');
        place*=10;
    }
    return 1;
}

static int bufs_match(const char *buf1, const char *buf2, int len)
{
    for(len--; len>=0; len--)
    {
        if(buf1[len]!=buf2[len])
            return 0;
    }
    return 1;
}

static int
get_msg_lens(const char *msg, int len, int *event_len, int *handle_len)
{
    int i, j;
    for(i=0; i<len; i++)
    {
        if(msg[i]==' ')
        {
            *event_len=i;
            break;
        }
    }
    for(j=i+1; j<len; j++)
    {
        if(msg[j]==' ')
        {
            break;
            *handle_len=j-i-1;
            return 1;
        }
    }
    if(i>=len) //never saw first ' '
        return 0;
    *handle_len=j-i-1;
    return 1;
}
