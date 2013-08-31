#include"lineclient.h"
#include<unistd.h>

// returns 1 if delim found, 0 otherwise
static int advance_next_pos(lineclient *lc);

//Construct a lineclient
void lineclient_construct(lineclient *lc, struct pollfd *pfd,
                          int sendbuf_len, int recvbuf_len)
{
    pollconn_construct(&lc->c, pfd, sendbuf_len, recvbuf_len);
    pollconn_socket(&lc->c);
    lc->next_pos=-1;
    lc->closed=0;
    lc->shutdown=0;
    lc->error=0;
    lc->delim='\n';
}

//Destruct a lineclient
void lineclient_destruct(lineclient *lc)
{
    pollconn_destruct(&lc->c);
}

//Set the remote address by using DNS resolution
int lineclient_resolve(lineclient *lc, const char *host, u_int16_t port)
{
    return pollconn_resolve(&lc->c, host, port);
}

//Connect a lineclient
int lineclient_connect(lineclient *lc)
{
    return pollconn_connect(&lc->c);
}

//Operate a lineclient
void lineclient_operate(lineclient *lc)
{
    int error;
    pollconn *c;
    pollconn_operate(&lc->c);
    advance_next_pos(lc);
    c=&lc->c;
    if(pollconn_did_close(c))
       lc->closed=1;
    if(pollconn_did_recv(c))
    {
        if(pollconn_recv_bytes(c)==0)
            lc->shutdown=1;
        error=pollconn_err_recv(c);
        if(error!=0)
            lc->error=error;
    }
    if(pollconn_did_send(c))
    {
        error=pollconn_err_send(c);
        if(error!=0)
            lc->error=error;
    }
    if(pollconn_did_shutdown(c))
    {
        error=pollconn_err_shutdown(c);
        if(error!=0)
            lc->error=error;
    }
}

//Enqueue a line
int lineclient_enqueue(lineclient *lc, const char *data, int len)
{
    pollconn *c;
    if(len<0)
        return -1;
    c=&lc->c;
    if(pollconn_sendbuf_space(c)<len+1)
        return -1;
    pollconn_enqueue(c, len, data);
    pollconn_enqueue(c, 1, &lc->delim);
    return 0;
}
//NOTE: The line should not contain the delimiter
//Returns -1 on failure

//Dequeue a line or event
int lineclient_dequeue(lineclient *lc)
{
    pollconn *c;
    lineclient_event e;
    e=lineclient_getevent(lc);
    switch(e)
    {
    case LINECLIENT_LINE:
        c=&lc->c;
        if(lineclient_getline(lc, NULL, NULL))
        {
            pollconn_dequeue(c, (lc->next_pos)+1);
            lc->next_pos=-1;
            advance_next_pos(lc);
        }
        break;
    case LINECLIENT_SHUTDOWN:
        lc->shutdown=0;
        break;
    case LINECLIENT_CLOSED:
        lc->closed=0;
        break;
    case LINECLIENT_ERROR:
        lc->error=0;
        lineclient_close(lc);
        break;
    default: //LINECLIENT_NULLEVENT
        return 0;
    }
    return 1;
}

//Signal a connection ready to close
int lineclient_shutdown(lineclient *lc)
{
    pollconn_shutdown(&lc->c);
    return 0;
}

//Immediately close a connection
int lineclient_close(lineclient *lc)
{
    return pollconn_close(&lc->c);
}

//Find the next event on a client
lineclient_event lineclient_getevent(const lineclient *lc)
{
    if(lc->error)
        return LINECLIENT_ERROR;
    if(lineclient_getline(lc, NULL, NULL))
        return LINECLIENT_LINE;
    if(lc->shutdown)
        return LINECLIENT_SHUTDOWN;
    if(lc->closed)
        return LINECLIENT_CLOSED;
    return LINECLIENT_NULLEVENT;    
}

//Store a pointer to the first complete line for a client
int lineclient_getline(const lineclient *lc, const char **data, int *len)
{
    const pollconn *c;
    const u_char *t;
    if(lc->next_pos<0)
        return 0;
    c=&lc->c;
    t=pollconn_recvbuf_data(c);
    if(t[lc->next_pos]!=lc->delim)
        return 0;
    if(data)
        *data=t;
    if(len)
        *len=lc->next_pos;
    return 1;
}
//Returns 1 if a line is found, 0 if not, -1 if error

//Find out the errno of an error which occurred on a connection
int lineclient_geterror(const lineclient *lc)
{
    return lc->error;
}

//Access the actual pollconn for a lineclient, usually for logging purposes
const pollconn* lineclient_conn(const lineclient *lc)
{
    return &lc->c;
}

static int advance_next_pos(lineclient *lc)
{
    pollconn *c;
    const u_char *data;
    int len;
    c=&lc->c;
    len=pollconn_recvbuf_filled(c);
    data=pollconn_recvbuf_data(c);
    if(lc->next_pos<0)
    {
        if(len>0)
            lc->next_pos=0;
        else
            return 0;
    }
    for(; lc->next_pos<len; (lc->next_pos)++)
    {
        if(data[lc->next_pos]==lc->delim)
            return 1;
    }
    lc->next_pos=len-1;
    return 0;
}
