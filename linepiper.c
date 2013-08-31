#include<unistd.h>
#include"linepiper.h"

static int advance_in_pos(linepiper *lp);

//Create a pipeserver
void
linepiper_construct(linepiper *lp, int fd_in, int fd_out,
                    int inbuf_len, int outbuf_len)
{
    lp->fd_in=fd_in;
    lp->fd_out=fd_out;
    if(inbuf_len<0)
        inbuf_len=0;
    if(outbuf_len<0)
        outbuf_len=0;
    tcpbuf_construct(&lp->buf_in, inbuf_len);
    tcpbuf_construct(&lp->buf_out, outbuf_len);
    lp->in_pos=-1;
    lp->delim='\n';
}

//Destroy a pipeserver
void
linepiper_destruct(linepiper *lp)
{
    lp->fd_in=-1;
    lp->fd_out=-1;
    tcpbuf_destruct(&lp->buf_in);
    tcpbuf_destruct(&lp->buf_out);
}

//Read what's available to the pipeserver
int
linepiper_read(linepiper *lp)
{
    int ret, buflen;
    u_char *buf;
    buflen=tcpbuf_alloc(&lp->buf_in, tcpbuf_space(&lp->buf_in), &buf);
    ret=read(lp->fd_in, buf, buflen);
    if(ret>=0)
    {
        tcpbuf_dealloc(&lp->buf_in, buflen-ret);
        advance_in_pos(lp);
    }
    return ret;
}

//Write what's possible from the pipeserver
int
linepiper_write(linepiper *lp)
{
    int ret;
    ret=write(lp->fd_out, tcpbuf_data(&lp->buf_out),
              tcpbuf_filled(&lp->buf_out));
    if(ret>0)
        tcpbuf_dequeue(&lp->buf_out, ret);
    return ret;
}

//Enqueue a line to be sent from the pipeserver
int
linepiper_enqueue(linepiper *lp, const char *data, int len)
{
    if(len<0)
        return -1;
    if(tcpbuf_space(&lp->buf_out)<len+1)
        return -1;
    tcpbuf_enqueue(&lp->buf_out, len, data);
    tcpbuf_enqueue(&lp->buf_out, 1, &lp->delim);
    return len+1;
}

//Dequeue a line received on the pipeserver
int
linepiper_dequeue(linepiper *lp)
{
    const char *t;
    if(lp->in_pos<0)
        return 0;
    t=tcpbuf_data(&lp->buf_in);
    if(t[lp->in_pos]!=lp->delim)
        return 0;
    tcpbuf_dequeue(&lp->buf_in, (lp->in_pos)+1);
    lp->in_pos=-1;
    advance_in_pos(lp);
    return 1;
}

//Read the line that's about to be dequeued from the pipeserver
int
linepiper_line(linepiper *lp, const char **data, int *len)
{
    const char *t;
    if(lp->in_pos<0)
        return 0;
    t=tcpbuf_data(&lp->buf_in);
    if(t[lp->in_pos]!=lp->delim)
        return 0;
    if(data)
        *data=t;
    if(len)
        *len=lp->in_pos;
    return 1;
}

static int
advance_in_pos(linepiper *lp)
{
    int len;
    const char *data;
    len=tcpbuf_filled(&lp->buf_in);
    if(lp->in_pos<0)
    {
        if(len>0)
            lp->in_pos=0;
        else
            return 0;
    }
    data=tcpbuf_data(&lp->buf_in);
    for(; lp->in_pos<len; (lp->in_pos)++)
    {
        if(data[lp->in_pos]==lp->delim)
            return 1;
    }
    return 0;
}
