#include"pipeserver.h"
#include<stdlib.h>

// The number of bytes expected necessary to fit the textual representation
// of a pipeserver_event, the integer pollset_handle, and spaces
static const int msg_bytes=20;

//Update the in event data
static int pipeserver_update_in(pipeserver *p);

//Update the out event data
static int pipeserver_update_out(pipeserver *p);

static pipeserver_event translate_lineserver_event(lineserver_event e);

//Create a pipeserver
void
pipeserver_construct(pipeserver *p, struct pollfd *ufds, int len,
                     int sendbuf_len, int recvbuf_len,
                     int fd_in, int fd_out,
                     int inbuf_len, int outbuf_len)
{
    if(recvbuf_len<0)
        recvbuf_len=0;
    if(sendbuf_len<0)
        sendbuf_len=0;
    lineserver_construct(&p->ls, ufds, len, sendbuf_len, recvbuf_len);
    linepiper_construct(&p->lp, fd_in, fd_out, inbuf_len, outbuf_len);
    p->accepted=-1;
    p->in_event=PIPESERVER_NULLEVENT;
    p->out_event=PIPESERVER_NULLEVENT;
    p->msg_len=(sendbuf_len>recvbuf_len?sendbuf_len:recvbuf_len)+msg_bytes;
    p->msg=(char*)malloc(p->msg_len);
    p->in_data=NULL;
    p->in_datalen=0;
    p->out_data=NULL;
    p->out_datalen=0;
}

//Destroy a pipeserver
void
pipeserver_destruct(pipeserver *p)
{
    lineserver_destruct(&p->ls);
    linepiper_destruct(&p->lp);
    free(p->msg);
    p->msg=NULL;
    p->msg_len=0;
}

//Cause a pipeserver to start listening
int
pipeserver_listen(pipeserver *p, u_int16_t port)
{
    return lineserver_listen(&p->ls, port);
}

//Cause a pipeserver to bind to an interface rather than INADDR_ANY
int
pipeserver_bind(pipeserver *p, u_int32_t ip_addr)
{
    return lineserver_bind(&p->ls, ip_addr);
}

//Operate all the clients and the listen socket on a pipeserver
void
pipeserver_operate(pipeserver *p)
{
    lineserver_operate(&p->ls);
    p->accepted=lineserver_accept(&p->ls);
    pipeserver_update_in(p);
}

//Read what's available to the pipeserver
int
pipeserver_read(pipeserver *p)
{
    int ret;
    ret=linepiper_read(&p->lp);
    if(ret<0)
        return ret;
    pipeserver_update_out(p);
    return ret;
}

//Write what's possible from the pipeserver
int
pipeserver_write(pipeserver *p)
{
    return linepiper_write(&p->lp);
}

//Process an incoming event to the pipeserver
int
pipeserver_process_in(pipeserver *p)
{
    int ret;
    ret=pipeserver_enqueue_in(p);
    if(ret>0)
        pipeserver_dequeue_in(p);
    return ret;
}

//Process an outgoing event from the pipeserver
int
pipeserver_process_out(pipeserver *p)
{
    int ret;
    ret=pipeserver_enqueue_out(p);
    if(ret>0)
        pipeserver_dequeue_out(p);
    return ret;
}

//Find out the next incoming event of the pipeserver
int
pipeserver_event_in(const pipeserver *p,
                    pollset_handle *h, pipeserver_event *e)
{
    if(p->in_event==PIPESERVER_NULLEVENT)
        return 0;
    if(h)
        *h=p->in_conn;
    if(e)
        *e=p->in_event;
    return 1;
}

//Find out the next outgoing event of the pipeserver
int
pipeserver_event_out(const pipeserver *p,
                     pollset_handle *h, pipeserver_event *e)
{
    if(p->out_event==PIPESERVER_NULLEVENT)
        return 0;
    if(h)
        *h=p->out_conn;
    if(e)
        *e=p->out_event;
    return 1;
}

//Find out the next incoming line from a pipeserver client
int
pipeserver_line_in(const pipeserver *p, pollset_handle *h,
                   const char **data, int *len)
{
    if(p->in_event!=PIPESERVER_RECEIVED)
        return 0;
    return lineserver_getline(&p->ls, p->in_conn, data, len);
}

//Find out the next outgoing line from a pipeserver client
int
pipeserver_line_out(const pipeserver *p, pollset_handle *h,
                    const char **data, int *len)
{
    if(p->out_event!=PIPESERVER_SEND)
        return 0;
    if(data)
        *data=p->out_data;
    if(len)
        *len=p->out_datalen;
    return 1;
}

//Find out the error of a client
int
pipeserver_error(const pipeserver *p, pollset_handle h)
{
    return lineserver_geterror(&p->ls, h);
}

//Enqueue an incoming event to the outgoing pipe
int
pipeserver_enqueue_in(pipeserver *p)
{
    int ret;
    if(p->in_event==PIPESERVER_NULLEVENT)
        return 0;
    ret=pipeserver_msg_build(p->msg, p->msg_len, p->in_event, p->in_conn,
                             p->in_data, p->in_datalen);
    if(ret<=0)
        return -1;
    return linepiper_enqueue(&p->lp, p->msg, ret);
}

//Dequeue an outgoing event from the incoming pipe
int
pipeserver_dequeue_out(pipeserver *p)
{
    int ret;
    ret=linepiper_dequeue(&p->lp);
    pipeserver_update_out(p);
    return ret;
}

//Dequeue an incoming event
int
pipeserver_dequeue_in(pipeserver *p)
{
    int ret;
    if(p->in_event==PIPESERVER_NULLEVENT)
        return 0;
    if(p->in_event==PIPESERVER_ACCEPTED)
    {
        p->in_event=PIPESERVER_NULLEVENT;
        p->in_conn=-1;
        return 1;
    }
    ret=lineserver_dequeue(&p->ls, p->in_conn);
    pipeserver_update_in(p);
    return ret;
}

//Enqueue/execute an outgoing event
int
pipeserver_enqueue_out(pipeserver *p)
{
    switch(p->out_event)
    {
    case PIPESERVER_CLOSE:
        //if(lineserver_close(&p->ls, p->out_conn)!=0)
        //    return -1;
        lineserver_close(&p->ls, p->out_conn);
        break;
    case PIPESERVER_SHUTDOWN:
        //if(lineserver_shutdown(&p->ls, p->out_conn)!=0)
        //    return -1;
        lineserver_shutdown(&p->ls, p->out_conn);
        break;
    case PIPESERVER_SEND:
        //if(lineserver_enqueue(&p->ls, p->out_conn,
        //                      p->out_data, p->out_datalen)<0)
        //    return -1;
        lineserver_enqueue(&p->ls, p->out_conn,
            p->out_data, p->out_datalen);
        break;
    default:
        return 0;
    }
    return 1;
}

//Enqueue an event to the outgoing pipe
int
pipeserver_enqueue_event(pipeserver *p,
                         pipeserver_event e, pollset_handle h,
                         const char *data, int len)
{
    int ret;
    ret=pipeserver_msg_build(p->msg, p->msg_len, e, h, data, len);
    if(ret<0)
        return -1;
    return linepiper_enqueue(&p->lp, p->msg, ret);
}

//Enqueue an outgoing line
int
pipeserver_enqueue_line(pipeserver *p, pollset_handle h,
                            const char *data, int len)
{
    return lineserver_enqueue(&p->ls, h, data, len);
}

//Signal that we're done sending data
int
pipeserver_shutdown(pipeserver *p, pollset_handle h)
{
    return lineserver_shutdown(&p->ls, h);
}

//Immediately close and delete a connection
int
pipeserver_close(pipeserver *p, pollset_handle h)
{
    return lineserver_close(&p->ls, h);
}

static int
pipeserver_update_in(pipeserver *p)
{
    pollset_handle h;
    lineserver_event e;
    if(lineserver_getnextevent(&p->ls, &h, &e))
    {
        p->in_conn=h;
        p->in_event=translate_lineserver_event(e);
        if(p->in_event==PIPESERVER_RECEIVED)
            lineserver_getline(&p->ls, h, &p->in_data, &p->in_datalen);
    }
    else if(p->accepted!=-1)
    {
        p->in_conn=p->accepted;
        p->in_event=PIPESERVER_ACCEPTED;
        p->accepted=-1;
    }
    else
    {
        p->in_conn=-1;
        p->in_event=PIPESERVER_NULLEVENT;
        return 0;
    }
    return 1;
}

static int
pipeserver_update_out(pipeserver *p)
{
    const char *buf;
    int len;
    while(1)
    {
        if(linepiper_line(&p->lp, &buf, &len)) // there is a complete line
        {
            if(pipeserver_msg_read(buf, len,
                                   &p->out_event, &p->out_conn,
                                   &p->out_data, &p->out_datalen))
                return 1;
            //silently ignore poorly formed messages
            linepiper_dequeue(&p->lp);
        }
        else
            break;
    }
    p->out_event=PIPESERVER_NULLEVENT;
    p->out_conn=-1;
    return 0;
}

static pipeserver_event
translate_lineserver_event(lineserver_event e)
{
    switch(e)
    {
    case LINESERVER_CLOSED:
        return PIPESERVER_CLOSED;
    case LINESERVER_ERROR:
        return PIPESERVER_ERROR;
    case LINESERVER_SHUTDOWN:
        return PIPESERVER_CLOSING;
    case LINESERVER_LINE:
        return PIPESERVER_RECEIVED;
    default:
        return PIPESERVER_NULLEVENT;
    }
}
