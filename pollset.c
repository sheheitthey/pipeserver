/*
  pollset.cc
  
  pollset.handles is sorted by fd
*/

#include"pollset.h"
#include<stdlib.h>
#include<string.h>

static int pollset_binsearch(const pollset *ps, pollset_handle h, int *found);
// binary search of handles for h
// returns index with handle h if found
// returns the index where h should be added if not found, meaning everything
// from there on should be shifted right

static pollset_handle pollset_addconn_socket(pollset *ps, pollconn *c);
// add a pollconn that has an allocated socket
// returns -1 if error

//Create a pollset
void
pollset_construct(pollset *ps, struct pollfd *ufds, int len,
                  int sendbuf_len, int recvbuf_len)
{
    pollpool_construct(&ps->pp, ufds, len, sendbuf_len, recvbuf_len);
    ps->handles=(pollconn**)malloc(sizeof(pollconn*)*ps->pp.len);
    ps->handles_len=0;
}

//Destroy a pollset
void
pollset_destruct(pollset *ps)
{
    pollpool_destruct(&ps->pp);
    free(ps->handles);
    ps->handles=NULL;
    ps->handles_len=0;
}

//Change the maximum size of a pollset
void
pollset_resize(pollset *ps, struct pollfd *ufds, int newlen)
{
    //pollpool_resize(ps->pp, ufds, newlen);
}

//Add a pollconn
pollset_handle
pollset_addconn(pollset *ps)
{
    pollset_handle ret;
    pollconn *c;
    c=pollpool_new(&ps->pp);
    if(!c)
        return -1; // problem allocating pollconn
    ret=pollconn_socket(c);
    if(ret<0)
        return ret; // problem calling pollconn_socket()
    return pollset_addconn_socket(ps, c);
}

//Add an accepted pollconn
pollset_handle
pollset_addaccepted(pollset *ps, int fd)
{
    pollconn *c;
    c=pollpool_new(&ps->pp);
    if(!c)
        return -1; // problem allocating pollconn
    pollconn_accepted(c, fd);
    return pollset_addconn_socket(ps, c);
}

//Delete a pollconn
int
pollset_delconn(pollset *ps, pollset_handle psh)
{
    int index, found;
    index=pollset_binsearch(ps, psh, &found);
    if(!found)
        return -1;
    pollpool_free(&ps->pp, ps->handles[index]);
    memmove(ps->handles+index, ps->handles+index+1,
            sizeof(pollconn*)*(ps->handles_len-index-1));
    (ps->handles_len)--;
    return 0;
}

//Access a pollconn
pollconn *pollset_conn(const pollset *ps, pollset_handle psh)
{
    int index, found;
    index=pollset_binsearch(ps, psh, &found);
    if(!found)
        return NULL;
    return ps->handles[index];
}

//Find out the handle of a pollconn belonging to this pollset
pollset_handle
pollset_connhandle(const pollset *ps, pollconn *c)
{
    return pollpool_index(&ps->pp, c);
}

//Poll everything in the pollset
int
pollset_poll(pollset *ps, int timeout)
{
    return pollpool_poll(&ps->pp, timeout);
}

//Operate everything in the pollset
void
pollset_operate(pollset *ps)
{
    int i;
    for(i=0; i<ps->handles_len; i++)
        pollconn_operate(ps->handles[i]);
}
#include<stdio.h>
static int
pollset_binsearch(const pollset *ps, pollset_handle h, int *found)
{
    int l, m, r;
    pollset_handle t;
    if(found)
        *found=0;
    l=0;
    r=ps->handles_len-1;
    if(l>r)
        return 0;
    while(1)
    {
        m=(l+r)/2;
        t=pollset_connhandle(ps, ps->handles[m]);
        if(h==t)
        {
            if(found)
                *found=1;
            return m;
        }
        if(l>=r) //definitely can't find it at this point
        {
            if(h<t)
                return m;
            else
                return m+1;
        }
        if(h<t)
            r=m-1;
        else //(h>t)
            l=m+1;
    }
}

static pollset_handle
pollset_addconn_socket(pollset *ps, pollconn *c)
{
    int index, ret, found;
    ret=pollset_connhandle(ps, c);
    index=pollset_binsearch(ps, ret, &found);
    if(found)
        return -1; // entry already exists
    memmove(ps->handles+index+1, ps->handles+index,
            sizeof(pollconn*)*(ps->handles_len-index));
    ps->handles[index]=c;
    (ps->handles_len)++;
    return ret;
}
