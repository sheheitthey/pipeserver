#include"pollset.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define max_len 20

#define sendbuf_len 4096
#define recvbuf_len 4096

#define max_getline 4096

void print_pollset_state(const pollset *s);

int handle_cmd(char *cmd);
int handle_cmd_add(char *args);
int handle_cmd_del(char *args);
int handle_cmd_print(char *args);
int handle_cmd_quit(char *args);

struct pollfd ufds[max_len];
pollset s;

int
main()
{
    int len;
    char linebuf[max_getline];
    //setvbuf(stdout, NULL, _IONBF, 0);
    pollset_construct(&s, ufds, max_len, sendbuf_len, recvbuf_len);
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
    pollset_destruct(&s);
    return 0;
}

void
print_pollset_state(const pollset *s)
{
    int i;
    printf("handles: ");
    for(i=0; i<s->handles_len; i++)
        printf("%d ", pollset_connhandle(s, s->handles[i]));
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
    pollset_handle h;
    h=pollset_addconn(&s);
    if(h<0)
        printf("Error adding connection.\n");
    else
        printf("Added connection %d.\n", h);
    return 1;
}

int handle_cmd_del(char *args)
{
    pollset_handle h;
    h=atoi(args);
    if(pollset_delconn(&s, h)!=0)
        printf("Error deleting connection.\n");
    else
        printf("Deleted connection %d.\n", h);
    return 1;
}

int handle_cmd_print(char *args)
{
    print_pollset_state(&s);
    return 1;
}

int handle_cmd_quit(char *cmd)
{
    return 0;
}
