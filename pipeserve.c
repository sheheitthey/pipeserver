#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/stat.h>
#include"pipeserver.h"

#define def_max_clients     1000
#define def_sendbuf_len     4096
#define def_recvbuf_len     4096
#define def_inbuf_len    1048576
#define def_outbuf_len   1048576
#define def_listen_port     7584

int max_clients=def_max_clients;
int sendbuf_len=def_sendbuf_len;
int recvbuf_len=def_recvbuf_len;
int inbuf_len=def_inbuf_len;
int outbuf_len=def_outbuf_len;
int listen_port=def_listen_port;
int foreground=0;

char *execv_path;
char **execv_argv;

int daemon_init(void);

pid_t pipe_std_exec(int *fd_stdout, int *fd_stdin,
                    const char *path, char *const argv[]);
//allocates for execv_argv, needs to be freed

const char usage_fmt[]="\
Usage: pipeserve <-p port (default %d)>\n\
                 <-n max clients (default %d)>\n\
                 <-s send buffer size (default %d)>\n\
                 <-r receive buffer size (default %d)>\n\
                 <-i pipe read buffer size (default %d)>\n\
                 <-o pipe write buffer size (defaut %d)>\n\
                 <-f (run in the foreground)\n\
                 [path] args...\n";

void print_usage_str();

int read_args(int argc, char *argv[]);

int
main(int argc, char *argv[])
{
    pipeserver server;
    struct pollfd *ufds;
    int fd_stdout, fd_stdin, ret=-1;
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    if(!read_args(argc, argv))
        return 0;
    ufds=(struct pollfd*)malloc(sizeof(struct pollfd)*(max_clients+2));
    if(strcmp(execv_path, "-")==0)
    {
        fd_stdout=fileno(stdin);
        fd_stdin=fileno(stdout);
    }
    else if(pipe_std_exec(&fd_stdout, &fd_stdin, execv_path, execv_argv)<0)
    {
        perror("pipeserve execv");
        return 0;
    }
    //fd_stdout=fileno(stdin);
    //fd_stdin=fileno(stdout);
    free(execv_argv);
    ufds[0].fd=fd_stdout;
    ufds[0].events=POLLIN;
    pipeserver_construct(&server, ufds+1, max_clients+1,
                         sendbuf_len, recvbuf_len,
                         fd_stdout, fd_stdin,
                         inbuf_len, outbuf_len);
    if(pipeserver_listen(&server, listen_port)==-1)
    {
        perror("pipeserver bind() and listen()");
        pipeserver_destruct(&server);
        return 0;
    }
    if(!foreground && daemon_init()<0)
    {
        perror("pipeserve backgrounding");
        return 0;
    }
    while(1)
    {
        if(poll(ufds, max_clients+1, 250)==-1)
        {
            perror("pipeserver poll()");
            break;
        }
        pipeserver_operate(&server);
        if(ufds[0].revents&POLLIN)
            ret=pipeserver_read(&server);
        while(pipeserver_process_in(&server)>0)
            pipeserver_write(&server);
        while(pipeserver_process_out(&server)>0);
        if(ret==0)
            break;
    }
    pipeserver_destruct(&server);
    free(ufds);
    return 0;
}

int
daemon_init(void)
{
    pid_t pid;
    if((pid=fork())<0)
        return -1;
    if(pid!=0)
        exit(0);
    setsid();
    chdir("/");
    umask(0);
    return 0;
}

pid_t pipe_std_exec(int *fd_stdout, int *fd_stdin,
                    const char *path, char *const argv[])
{
    pid_t ret;
    int in[2];
    int out[2];
    if(pipe(in)==-1)
        return -1;
    if(pipe(out)==-1)
    {
        close(in[0]);
        close(in[1]);
        return -1;
    }
    if((ret=fork())==-1)
    {
        close(in[0]);
        close(in[1]);
        close(out[0]);
        close(out[1]);
        return -1;
    }
    if(ret==0) //child
    {
        close(out[0]); // close read side of out
        close(in[1]);  // close write side of in
        dup2(out[1], fileno(stdout)); // make write side of out stdout
        dup2(in[0], fileno(stdin));   // make read side of in stdin
        if(execv(path, argv)==-1)
        {
            close(out[1]);
            close(in[0]);
            close(fileno(stdout));
            close(fileno(stdin));
            return -1;
        }
    }
    //parent
    *fd_stdout=out[0];
    *fd_stdin=in[1];
    close(out[1]);
    close(in[0]);
    return ret;
}

int
read_args(int argc, char *argv[])
{
    int i;
    char opt;
    while((opt=getopt(argc, argv, "p:n:s:r:i:o:f"))!=-1)
    {
        switch(opt)
        {
        case 'p':
            listen_port=atoi(optarg);
            if(listen_port<=0)
            {
                fprintf(stderr, "Error: invalid port.\n");
                return 0;
            }
            break;
        case 'n':
            max_clients=atoi(optarg);
            if(max_clients<=0)
            {
                fprintf(stderr, "Error: invalid number of max clients.\n");
                return 0;
            }
            break;
        case 's':
            sendbuf_len=atoi(optarg);
            if(sendbuf_len<=0)
            {
                fprintf(stderr, "Error: invalid send buffer size.\n");
                return 0;
            }
            break;
        case 'r':
            recvbuf_len=atoi(optarg);
            if(recvbuf_len<=0)
            {
                fprintf(stderr, "Error: invalid receive buffer size.\n");
                return 0;
            }
            break;
        case 'i':
            inbuf_len=atoi(optarg);
            if(inbuf_len<=0)
            {
                fprintf(stderr, "Error: invalid pipe read buffer size.\n");
                return 0;
            }
            break;
        case 'o':
            outbuf_len=atoi(optarg);
            if(outbuf_len<=0)
            {
                fprintf(stderr, "Error: invalid pipe out buffer size.\n");
                return 0;
            }
            break;
        case 'f':
            foreground=1;
            break;
        }
    }
    if(argc-optind<=0)
    {
        print_usage_str();
        return 0;
    }
    execv_path=argv[optind];
    execv_argv=(char **)malloc(sizeof(char *)*(argc-optind+1));
    i=0;
    while(optind<argc)
    {
        execv_argv[i]=argv[optind];
        optind++;
        i++;
    }
    execv_argv[i]=(char *)NULL;
    return 1;
}

void
print_usage_str()
{
    fprintf(stderr, usage_fmt, def_listen_port, def_max_clients,
            def_sendbuf_len, def_recvbuf_len, def_inbuf_len, def_outbuf_len);
}
