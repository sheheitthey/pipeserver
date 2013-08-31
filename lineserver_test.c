#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>
#include"lineserver.h"

#define max_clients 100
#define sendbuf_len 4096
#define recvbuf_len 4096

typedef struct client_list
{
    pollset_handle *handles;
    int len;
} client_list;

void client_list_construct(client_list *cl);
void client_list_destruct(client_list *cl);

int client_list_add(client_list *cl, pollset_handle h);
int client_list_del(client_list *cl, pollset_handle h);

//void close_finished_clients(client_list *cl, lineserver *ls);

void handle_packets(client_list *cl, lineserver *ls);

void sighandler_term(int signum);

int done=0;

int
main()
{
    lineserver ls;
    struct pollfd ufds[max_clients+1];
    client_list cl;
    pollset_handle h;
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, sighandler_term);
    lineserver_construct(&ls, ufds, max_clients+1, sendbuf_len, recvbuf_len);
    client_list_construct(&cl);
    if(lineserver_listen(&ls, 7584)==-1)
    {
        perror("lineserver bind() and listen()");
        return 0;
    }
    while(!done)
    {
        if(poll(ufds, max_clients+1, 100)==-1)
        {
            perror("lineserver poll()");
            break;
        }
        lineserver_operate(&ls);
        handle_packets(&cl, &ls);
        //close_finished_clients(&cl, &ls);
        if((h=lineserver_accept(&ls))!=-1)
        {
            printf("Added new connection %d.\n", h);
            client_list_add(&cl, h);
        }
    }
    client_list_destruct(&cl);
    lineserver_destruct(&ls);
    return 0;
}

void
handle_packets(client_list *cl, lineserver *ls)
{
    pollset_handle h;
    lineserver_event e;
    int len, ret;
    const char *packet;
    while((ret=lineserver_getnextevent(ls, &h, &e))==1)
    {
        switch(e)
        {
        case LINESERVER_LINE:
            lineserver_getline(ls, h, &packet, &len);
            lineserver_enqueue(ls, h, packet, len);
            break;
        case LINESERVER_SHUTDOWN:
            printf("Shutdown connection %d.\n", h);
            lineserver_shutdown(ls, h);
            break;
        case LINESERVER_CLOSED:
            printf("Closed connection %d.\n", h);
            break;
        case LINESERVER_ERROR:
            printf("Error on connection %d.\n", h);
            perror("lineserver_test");
            break;
        case LINESERVER_NULLEVENT:
            printf("How the heck did this happen?\n");
            break;
        }
        lineserver_dequeue(ls, h);
    }
    /*
    while((ret=lineserver_nextline(ls, &h, &packet, &len))==1)
    {
        lineserver_enqueue(ls, h, packet, len);
        lineserver_dequeue(ls, h);
    }
    */
}

void
client_list_construct(client_list *cl)
{
    cl->len=0;
    cl->handles=(pollset_handle*)malloc(sizeof(pollset_handle)*max_clients);
}

void
client_list_destruct(client_list *cl)
{
    cl->len=0;
    free(cl->handles);
}

int
client_list_add(client_list *cl, pollset_handle h)
{
    cl->handles[((cl->len)++)]=h;
    return cl->len;
}

int
client_list_del(client_list *cl, pollset_handle h)
{
    int i;
    for(i=0; i<cl->len; i++)
    {
        if(cl->handles[i]==h)
        {
            memmove(cl->handles+i, cl->handles+i+1,
                    sizeof(pollset_handle)*cl->len-i-1);
            (cl->len)--;
            return cl->len;
        }
    }
    return -1;
}

/*
void
close_finished_clients(client_list *cl, lineserver *ls)
{
    int i;
    pollset_handle h;
    const pollconn *c;
    for(i=0; i<cl->len; i++)
    {
        h=cl->handles[i];
        c=lineserver_conn(ls, h);
        if(c)
        {
            if(pollconn_did_recv(c) && pollconn_recv_bytes(c)==0)
                lineserver_shutdown(ls, h);
            if(pollconn_did_close(c))
            {
                lineserver_close(ls, h);
                client_list_del(cl, h);
                i--;
            }
        }
    }
}
*/

void
sighandler_term(int signum)
{
    done=1;
}
