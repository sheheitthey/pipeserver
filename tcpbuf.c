/*
  tcpbuf.cc
*/

#include"tcpbuf.h"
#include<string.h>
#include<stdlib.h>

void
tcpbuf_construct(tcpbuf *t, int len)
{
    if(len<=0)
    {
        t->data=NULL;
        t->len=0;
    }
    else
    {
        t->data=(u_char*)malloc(len);
        t->len=len;
    }
    t->pos=0;
}

void
tcpbuf_destruct(tcpbuf *t)
{
    free(t->data);
    t->data=NULL;
    t->len=0;
    t->pos=0;
}

int
tcpbuf_alloc(tcpbuf *t, int len, u_char **ptr)
{
    int avail, ret;
    if(len<0)
        return 0;
    *ptr=t->data+t->pos;
    avail=t->len-t->pos;
    ret=(len<avail?len:avail);
    t->pos+=ret;
    return ret;
}

int
tcpbuf_dealloc(tcpbuf *t, int len)
{
    if(len<0)
        return 0;
    if(len>t->pos)
        len=t->pos;
    t->pos-=len;
    return len;
}

int
tcpbuf_enqueue(tcpbuf *t, int len, const u_char *data)
{
    u_char *buf;
    int copylen;
    copylen=tcpbuf_alloc(t, len, &buf);
    memcpy(buf, data, copylen);
    return copylen;
}

int
tcpbuf_dequeue(tcpbuf *t, int len)
{
    int deqlen, movelen;
    deqlen=(len<t->pos?len:t->pos);
    movelen=t->pos-deqlen;
    memmove(t->data, t->data+deqlen, movelen);
    t->pos-=deqlen;
    return deqlen;
}

const char*
tcpbuf_data(const tcpbuf *t)
{
    return (const char*)t->data;
}

int
tcpbuf_filled(const tcpbuf *t)
{
    return t->pos;
}

int
tcpbuf_space(const tcpbuf *t)
{
    return t->len-t->pos;
}

int
tcpbuf_empty(const tcpbuf *t)
{
    return t->pos==0;
}

int
tcpbuf_full(const tcpbuf *t)
{
    return t->pos==t->len;
}
