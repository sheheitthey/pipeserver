#include"pollpool.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define max_len 20

#define sendbuf_len 4096
#define recvbuf_len 4096

#define max_getline 4096

void print_pollpool_state(const pollpool *p);

int handle_cmd(char *cmd);
int handle_cmd_add(char *args);
int handle_cmd_del(char *args);
int handle_cmd_print(char *args);
int handle_cmd_quit(char *args);

struct pollfd ufds[max_len];
pollpool p;

int
main()
{
    int len;
    char linebuf[max_getline];
    //setvbuf(stdout, NULL, _IONBF, 0);
    pollpool_construct(&p, ufds, max_len, sendbuf_len, recvbuf_len);
    printf("> ");
    while(fgets(linebuf, max_getline, stdin)!=NULL)
    {
        len=strlen(linebuf);
        if(linebuf[len-1]=='\n')
        {
            linebuf[len-1]='\0';
            len--;
        }
        if(handle_cmd(linebuf)==0)
            break;
        printf("> ");
    }
    pollpool_destruct(&p);
    return 0;
}

void
print_pollpool_state(const pollpool *p)
{
    int i;
    printf("pollconns: ");
    for(i=0; i<p->len; i++)
        printf("%d ", pollconn_sock(p->pollconns+i));
    printf("\n");
    printf("freelist: ");
    for(i=0; i<p->freelist_len; i++)
    {
        printf("%d ", p->freelist[(p->freelist_head+i)%p->len]);
    }
    printf("\n");
}

int handle_cmd(char *cmd)
{
    int i, len;
    len=strlen(cmd);
    const char *args;
    for(i=0; i<len; i++)
    {
        if(cmd[i]==' ')
        {
            cmd[i]='\0';
            break;
        }
    }
    args=cmd+i+1;
    if(strcmp(cmd, "add")==0)
        return handle_cmd_add(cmd+i+1);
    else if(strcmp(cmd, "del")==0)
        return handle_cmd_del(cmd+i+1);
    else if(strcmp(cmd, "print")==0)
        return handle_cmd_print(cmd+i+1);
    else if(strcmp(cmd, "quit")==0 || strcmp(cmd, "exit")==0)
        return handle_cmd_quit(cmd+i+1);
    else
    {
        printf("Unrecognized command.\n");
        return 1;
    }
}

int handle_cmd_add(char *args)
{
    pollconn *c;
    c=pollpool_new(&p);
    if(c)
        pollconn_socket(c);
    else
        printf("Error adding connection.\n");
    return 1;
}

int handle_cmd_del(char *args)
{
    int index;
    index=atoi(args);
    if(index>=0 && index <p.len)
    {
        if(pollpool_free(&p, p.pollconns+index)!=0)
            printf("Error deleting connection.\n");
    }
    else
        printf("Error with argument for del.\n");
    return 1;
}

int handle_cmd_print(char *args)
{
    print_pollpool_state(&p);
    return 1;
}

int handle_cmd_quit(char *cmd)
{
    return 0;
}
