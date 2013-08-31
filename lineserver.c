#include"lineserver.h"
#include<stdlib.h>
#include<string.h>

static int lineserver_find_handle(const lineserver *l, pollset_handle h);

// internal method for lineserver_getevent
static lineserver_event lineserver_getevent_i(const lineserver *l, int index);

// internal method for lineserver_getline
static int lineserver_getline_i(const lineserver *l, int index,
                                const char **data, int *len);

// returns 1 if delim found, 0 otherwise
static int advance_client_pos(lineserver *l, int index);

// returns 1 if next connection found, 0 otherwise
static int advance_next_conn(lineserver *l);

// remove a connection
static int lineserver_remove(lineserver *l, int index);

void lineserver_construct(lineserver *l, struct pollfd *ufds, int len,
                          int sendbuf_len, int recvbuf_len)
{
    l->num_clients=0;
    l->next_conn_pos=-1;
    l->delim='\n';
    if(len<1)
    {
        l->max_clients=0;
        pollset_construct(&l->ps, NULL, 0, sendbuf_len, recvbuf_len);
        pollconn_construct(&l->ls, NULL, 0, 0);
        l->clients=NULL;
        return;
    }
    l->max_clients=len-1;
    pollset_construct(&l->ps, ufds+1, len-1, sendbuf_len, recvbuf_len);
    pollconn_construct(&l->ls, ufds, 0, 0);
    pollconn_socket(&l->ls);
    l->clients=(lineserver_client*)
        malloc(sizeof(lineserver_client)*l->max_clients);
}

//Destroy a lineserver
void lineserver_destruct(lineserver *l)
{
    pollconn_destruct(&l->ls);
    pollset_destruct(&l->ps);
}

//Cause a lineserver to start listening
int lineserver_listen(lineserver *l, u_int16_t port)
{
    pollconn_set_saddr(&l->ls, ntohl(l->ls.saddr.sin_addr.s_addr), port);
    if(pollconn_bind(&l->ls)!=-1)
        return pollconn_listen(&l->ls);
    return -1;
}

//Cause a server to bind to an interface rather than INADDR_ANY
int lineserver_bind(lineserver *l, u_int32_t ip_addr)
{
    pollconn_set_saddr(&l->ls, ip_addr, ntohs(l->ls.saddr.sin_port));
    return pollconn_bind(&l->ls);
}

//Operate all the clients on the server
void lineserver_operate(lineserver *l)
{
    int i, error;
    lineserver_client *lsc;
    pollconn *c;
    l->next_conn_pos=-1;
    pollset_operate(&l->ps);
    for(i=0; i<l->num_clients; i++)
    {
        lsc=l->clients+i;
        advance_client_pos(l, i);
        c=pollset_conn(&l->ps, lsc->h);
        if(pollconn_did_close(c))
        {
            lsc->closed=1;
        }
        if(pollconn_did_recv(c))
        {
            if(pollconn_recv_bytes(c)==0)
            {
                lsc->shutdown=1;
            }
            error=pollconn_err_recv(c);
            if(error!=0)
                lsc->error=error;
        }
        if(pollconn_did_send(c))
        {
            error=pollconn_err_send(c);
            if(error!=0)
                lsc->error=error;
        }
        if(pollconn_did_shutdown(c))
        {
            error=pollconn_err_shutdown(c);
            if(error!=0)
                lsc->error=error;
        }
    }
    advance_next_conn(l);
}

//Accept a connection, if there is one
pollset_handle lineserver_accept(lineserver *l)
{
    pollset_handle h;
    lineserver_client *lsc;
    pollconn_operate(&l->ls);
    if(pollconn_did_accept(&l->ls))
    {
        h=pollset_addaccepted(&l->ps, pollconn_accept_sock(&l->ls));
        if(h!=-1)
        {
            if(l->num_clients>=l->max_clients) //how did this happen?
            {
                pollset_delconn(&l->ps, h);
                return -1;
            }
            lsc=l->clients+(l->num_clients);
            lsc->h=h;
            lsc->next_pos=-1;
            lsc->closed=0;
            lsc->shutdown=0;
            lsc->error=0;
            (l->num_clients)++;
            return h;
        }
    }
    return -1;
}

//Enqueue a line
int lineserver_enqueue(lineserver *l, pollset_handle h,
                       const char *data, int len)
{
    pollconn *c;
    if(len<0)
        return -1;
    c=pollset_conn(&l->ps, h);
    if(!c)
        return -1;
    if(pollconn_sendbuf_space(c)<len+1)
        return -1;
    pollconn_enqueue(c, len, data);
    pollconn_enqueue(c, 1, &l->delim);
    return 0;
}

//Dequeue a line or event
int lineserver_dequeue(lineserver *l, pollset_handle h)
{
    int index;
    lineserver_client *lsc;
    pollconn *c;
    lineserver_event e;
    index=lineserver_find_handle(l, h);
    if(index<0)
        return -1;
    lsc=l->clients+index;
    e=lineserver_getevent_i(l, index);
    switch(e)
    {
    case LINESERVER_LINE:
        c=pollset_conn(&l->ps, h);
        if(lineserver_getline_i(l, index, NULL, NULL))
        {
            pollconn_dequeue(c, (lsc->next_pos)+1);
            lsc->next_pos=-1;
            advance_client_pos(l, index);
        }
        break;
    case LINESERVER_SHUTDOWN:
        lsc->shutdown=0;
        break;
    case LINESERVER_CLOSED:
        lsc->closed=0;
        lineserver_remove(l, index);
        break;
    case LINESERVER_ERROR:
        lsc->error=0;
        lineserver_close(l, h);
        break;
    default: //LINESERVER_NULLEVENT
        return 0;
    }
    advance_next_conn(l);
    return 1;
    /*
    index=lineserver_find_handle(l, h);
    if(index<0)
        return -1;
    lsc=l->clients+index;
    if(lsc->next_pos<0)
        return 0;
    c=pollset_conn(&l->ps, lsc->h);
    t=pollconn_recvbuf_data(c);
    if(t[lsc->next_pos]!=l->delim)
        return 0;
    pollconn_dequeue(c, (lsc->next_pos)+1);
    lsc->next_pos=-1;
    ret=advance_client_pos(l, index);
    while(ret==0)
    {
        (l->next_conn_pos)++;
        if(l->next_conn_pos==l->num_clients)
        {
            l->next_conn_pos=-1;
            break;
        }
        ret=advance_client_pos(l, l->next_conn_pos);
    }
    return 1;
    */
}

//Signal that we're done sending data
int lineserver_shutdown(lineserver *l, pollset_handle h)
{
    pollconn *c;
    c=pollset_conn(&l->ps, h);
    if(c)
    {  
        pollconn_shutdown(c);
        return 0;
    }
    return -1;
}

//Immediately close and delete a connection
int lineserver_close(lineserver *l, pollset_handle h)
{
    pollconn *c;
    int index;
    c=pollset_conn(&l->ps, h);
    if(!c)
        return -1;
    if(pollconn_ready(c))
    {
        if(pollconn_close(c)==-1)
            return -1;
    }
    index=lineserver_find_handle(l, h);
    if(index<0)
        return -1;
    lineserver_remove(l, index);
/*  ret=advance_client_pos(l, index);
    while(ret==0)
    {
        (l->next_conn_pos)++;
        if(l->next_conn_pos==l->num_clients)
        {
            l->next_conn_pos=-1;
            break;
        }
        ret=advance_client_pos(l, l->next_conn_pos);
    }
*/
    return 0;
}

//Find out the next event on a client
lineserver_event
lineserver_getevent(const lineserver *l, pollset_handle h)
{
    int index;
    index=lineserver_find_handle(l, h);
    if(index<0)
        return LINESERVER_NULLEVENT;
    return lineserver_getevent_i(l, index);
}

//Store a pointer to the first complete line for a connection
int lineserver_getline(const lineserver *l, pollset_handle h,
                       const char **data, int *len)
{
    int index;
    index=lineserver_find_handle(l, h);
    if(index<0)
        return -1;
/*  lsc=l->clients+index;
    if(lsc->next_pos<0)
        return 0;
    c=pollset_conn(&l->ps, lsc->h);
    t=pollconn_recvbuf_data(c);
    if(t[lsc->next_pos]!=l->delim)
        return 0;
    if(data)
        *data=t;
    if(len)
        *len=lsc->next_pos;
    return 1;*/
    return lineserver_getline_i(l, index, data, len);
}

//Find out the errno of an error which occurred on a connection
int
lineserver_geterror(const lineserver *l, pollset_handle h)
{
    int index;
    lineserver_client *lsc;
    index=lineserver_find_handle(l, h);
    if(index<0)
        return 0;
    lsc=l->clients+index;
    return lsc->error;
}

//Return a handle to the next client which has a complete line
pollset_handle lineserver_getnextclient(const lineserver *l)
{
    if(l->next_conn_pos==-1)
        return -1;
    return l->clients[l->next_conn_pos].h;
}

/*
//Combine nextclient() and nextline() to store the next client and line
int lineserver_nextline(const lineserver *l, pollset_handle *h,
                        const char **data, int *len)
{
    pollset_handle t;
    if(l->next_conn_pos==-1)
        return -1;
    t=l->clients[l->next_conn_pos].h;
    if(h)
        *h=t;
    return lineserver_line(l, t, data, len);
}
*/

//Combine nextclient() and getevent() to store the next client and event
int
lineserver_getnextevent(const lineserver *l, pollset_handle *h,
                        lineserver_event *e)
{
    if(l->next_conn_pos==-1)
        return 0;
    if(h)
        *h=l->clients[l->next_conn_pos].h;
    if(e)
        *e=lineserver_getevent_i(l, l->next_conn_pos);
    return 1;
}

//Access the actual pollconn for a handle, usually for logging purposes
const pollconn* lineserver_conn(const lineserver *l, pollset_handle h)
{
    return pollset_conn(&l->ps, h);
}

static int
lineserver_find_handle(const lineserver *l, pollset_handle h)
{
    int i;
    for(i=0; i<l->num_clients; i++)
    {
        if(l->clients[i].h==h)
            return i;
    }
    return -1;
}

static lineserver_event
lineserver_getevent_i(const lineserver *l, int index)
{
    lineserver_client *lsc;
    lsc=l->clients+index;
    if(lsc->error)
        return LINESERVER_ERROR;
    if(lineserver_getline_i(l, index, NULL, NULL))
        return LINESERVER_LINE;
    if(lsc->shutdown)
        return LINESERVER_SHUTDOWN;
    if(lsc->closed)
        return LINESERVER_CLOSED;
    return LINESERVER_NULLEVENT;
}

static int
lineserver_getline_i(const lineserver *l, int index,
                     const char **data, int *len)
{
    lineserver_client *lsc;
    pollconn *c;
    const u_char *t;
    if(index<0)
        return -1;
    lsc=l->clients+index;
    if(lsc->next_pos<0)
        return 0;
    c=pollset_conn(&l->ps, lsc->h);
    t=pollconn_recvbuf_data(c);
    if(t[lsc->next_pos]!=l->delim)
        return 0;
    if(data)
        *data=t;
    if(len)
        *len=lsc->next_pos;
    return 1;
}
    
static int
advance_client_pos(lineserver *l, int index)
{
    lineserver_client *lsc;
    pollconn *c;
    const u_char *data;
    int len;
    lsc=l->clients+index;
    c=pollset_conn(&l->ps, lsc->h);
    len=pollconn_recvbuf_filled(c);
    data=pollconn_recvbuf_data(c);
    if(lsc->next_pos<0)
    {
        if(len>0)
            lsc->next_pos=0;
        else
            return 0;
    }
    for(; lsc->next_pos<len; (lsc->next_pos)++)
    {
        if(data[lsc->next_pos]==l->delim)
            return 1;
    }
    lsc->next_pos=len-1;
    return 0;
}

static int
advance_next_conn(lineserver *l)
{
    if(l->num_clients<=0 || l->next_conn_pos>=l->num_clients)
    {
        l->next_conn_pos=-1;
        return 0;
    }
    if(l->next_conn_pos<0)
        l->next_conn_pos=0;
    while(lineserver_getevent_i(l, l->next_conn_pos)==LINESERVER_NULLEVENT)
    {
        if(l->next_conn_pos==l->num_clients-1)
        {
            l->next_conn_pos=-1;
            return 0;
        }
        (l->next_conn_pos)++;
    }
    return 1;
}

static int
lineserver_remove(lineserver *l, int index)
{
    pollset_delconn(&l->ps, l->clients[index].h);
    memmove(l->clients+index, l->clients+index+1,
            sizeof(lineserver_client)*l->num_clients-index-1);
    (l->num_clients)--;
    advance_next_conn(l);
    return 0;
}
