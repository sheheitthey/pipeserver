#include"pollpool.h"
#include<stdlib.h>

// dequeue and return next free index, or -1 if none
static int pollpool_freelist_dequeue(pollpool *pp);

// enqueue a free index
static int pollpool_freelist_enqueue(pollpool *pp, int index);

// is an index currently allocated?
static int pollpool_index_allocated(const pollpool *pp, int index);

void
pollpool_construct(pollpool *pp, struct pollfd *ufds, int len,
                        int sendbuf_len, int recvbuf_len)
{
    int i;
    if(len<0)
        len=0;
    if(sendbuf_len<0)
        sendbuf_len=0;
    if(recvbuf_len<0)
        recvbuf_len=0;
    pp->ufds=ufds;
    pp->len=len;
    pp->pollconns=(pollconn*)malloc(sizeof(pollconn)*pp->len);
    pp->freelist=(int*)malloc(sizeof(int)*pp->len);
    pp->freelist_head=0;
    pp->freelist_len=pp->len;
    pp->sendbuf_len=sendbuf_len;
    pp->recvbuf_len=recvbuf_len;
    for(i=0; i<pp->len; i++)
    {
        pollconn_construct(pp->pollconns+i, pp->ufds+i,
                           sendbuf_len, recvbuf_len);
        pp->ufds[i].fd=-1;
        pp->ufds[i].events=0;
        pp->freelist[i]=i;
    }
}

void
pollpool_destruct(pollpool *pp)
{
    int i;
    for(i=0; i<pp->len; i++)
        pollconn_destruct(pp->pollconns+i);
    pp->ufds=NULL;
    free(pp->pollconns);
    pp->pollconns=NULL;
    free(pp->freelist);
    pp->freelist=NULL;
    pp->freelist_head=0;
    pp->freelist_len=0;
    pp->len=0;
}

void
pollpool_resize(pollpool *pp, struct pollfd *ufds, int newlen)
{
    // implement this shit
    // possibly implement packing when shrinking
}

pollconn*
pollpool_new(pollpool *pp)
{
    int index;
    index=pollpool_freelist_dequeue(pp);
    if(index<0)
        return NULL;
    return pp->pollconns+index;
}

int
pollpool_free(pollpool *pp, pollconn *c)
{
    int index;
    index=c-(pp->pollconns);
    if(!pollpool_index_allocated(pp, index))
        return -1;
    pollconn_reset(c);
    pp->ufds[index].fd=-1;
    pp->ufds[index].events=0;
    return pollpool_freelist_enqueue(pp, index);
}

int
pollpool_poll(pollpool *pp, int timeout)
{
    return poll(pp->ufds, pp->len, timeout);
}

int
pollpool_index(const pollpool *pp, const pollconn *c)
{
    int index;
    index=c-(pp->pollconns);
    if(!pollpool_index_allocated(pp, index)) //ouch this might slow things down
        return -1;
    return index;
}

static int
pollpool_freelist_dequeue(pollpool *pp)
{
    int ret;
    if(pp->freelist_len<=0)
        return -1;
    ret=pp->freelist[pp->freelist_head];
    (pp->freelist_head)++;
    if(pp->freelist_head>=pp->len)
        (pp->freelist_head)%=(pp->len);
    (pp->freelist_len)--;
    return ret;
}

static int
pollpool_freelist_enqueue(pollpool *pp, int index)
{
    int i;
    if(pp->freelist_len>=pp->len) //something went wrong...
        return -1;
    i=(pp->freelist_head+pp->freelist_len)%(pp->len);
    pp->freelist[i]=index;
    (pp->freelist_len)++;
    return 0;
}

static int
pollpool_index_allocated(const pollpool *pp, int index)
{
    int i;
    if(index<0 || index>=pp->len)
        return 0;
    for(i=0; i<pp->freelist_len; i++)
    {
        if(pp->freelist[(pp->freelist_head+i)%pp->len]==index)
            return 0;
    }
    return 1;
}
