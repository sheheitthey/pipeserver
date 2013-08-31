#include"lineclient.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<ctype.h>
#include<string.h>
#include<sys/time.h>
#include<time.h>
#include<errno.h>

#define def_num_lines   32
#define def_min_size     0
#define def_max_size   256
#define def_min_wait     0
#define def_max_wait     8

int num_lines=def_num_lines;
int min_size=def_min_size;
int max_size=def_max_size;
int min_wait=def_min_wait;
int max_wait=def_max_wait;

const char *host;
u_int16_t port;

#define tcpbuf_len    4096
const int sendbuf_len=tcpbuf_len;
const int recvbuf_len=tcpbuf_len;

const char usage_fmt[]="\
Usage: echo_test <-n number of lines (default %d)\n\
                 <-s min,max size of lines (default %d,%d)\n\
                 <-w min,max wait between lines (default %d,%d)\n\
                 [host] [port]\n";

int get_min_max(char *buf, int *min, int *max);
int read_args(int argc, char *argv[]);
void print_usage_str();
inline int rand_range(int min, int max);

void generate_line(char *buf, int len);

time_t start_time;

typedef struct test_string
{
    int len;
    char *str;
    int time;
} test_string;

typedef struct test_strings
{
    int num_strings;
    test_string *strings;
    char *buf;
    int next_send;
    int next_receive;
} test_strings;

void test_strings_construct(test_strings *ts, int num_strings,
                            int min_len, int max_len,
                            int min_wait, int max_wait);
void test_strings_destruct(test_strings *ts);
int test_strings_send(test_strings *ts, const char **buf, int *len);
void test_strings_sent(test_strings *ts);
int test_strings_received(test_strings *ts, const char *buf, int len);

void print_test_strings(test_strings *ts);
void print_string(const char *buf, int len);

int
main(int argc, char *argv[])
{
    lineclient client;
    lineclient_event e;
    const pollconn *c;
    struct pollfd pfd;
    test_strings ts;
    const char *buf;
    int len, ret;
    struct timeval init_time;
    gettimeofday(&init_time, NULL);
    srand(init_time.tv_sec^init_time.tv_usec);
    if(!read_args(argc, argv))
        return 1;
    lineclient_construct(&client, &pfd, sendbuf_len, recvbuf_len);
    if(lineclient_resolve(&client, host, port)!=0)
    {
        fprintf(stderr, "Error resolving host.\n");
        lineclient_destruct(&client);
        return 2;
    }
    while(1)
    {
        if(poll(&pfd, 1, 250)<0)
        {
            perror("echo_test poll");
            lineclient_destruct(&client);
            return 3;
        }
        lineclient_operate(&client);
        c=lineclient_conn(&client);
        if((pollconn_err_connect(c)!=EINPROGRESS &&
            pollconn_err_connect(c)!=0) ||
           pollconn_err_send(c)!=0)
        {
            fprintf(stderr, "Error connecting socket.\n");
            fprintf(stderr, "err_connect=%s\n",
                    strerror(pollconn_err_connect(c)));
            fprintf(stderr, "err_send=%s\n",
                    strerror(pollconn_err_send(c)));
            lineclient_destruct(&client);
            return 4;
        }
        if(pollconn_ready(c))
            break;
    }
    start_time=time(NULL);
    test_strings_construct(&ts, num_lines,
                           min_size, max_size, min_wait, max_wait);
    //print_test_strings(&ts);
    while(num_lines>0)
    {
        if(poll(&pfd, 1, 250)<0)
        {
            perror("echo_test poll");
            lineclient_destruct(&client);
            test_strings_destruct(&ts);
            return 5;
        }
        lineclient_operate(&client);
        if((ret=test_strings_send(&ts, &buf, &len))==1)
        {
            if(lineclient_enqueue(&client, buf, len)<0)
            {
                fprintf(stderr, "Error enqueuing to send buffers.\n");
                lineclient_destruct(&client);
                test_strings_destruct(&ts);
                return 6;
            }
            test_strings_sent(&ts);
            //printf("sent: ");
            //print_string(buf, len);
            //printf("\n");
        }
        else if(ret<0)
            lineclient_shutdown(&client);
        while((e=lineclient_getevent(&client))!=LINECLIENT_NULLEVENT)
        {
            if(e==LINECLIENT_LINE)
            {
                lineclient_getline(&client, &buf, &len);
                //printf("received: ");
                //print_string(buf, len);
                //printf("\n");
                if(!test_strings_received(&ts, buf, len))
                {
                    lineclient_destruct(&client);
                    test_strings_destruct(&ts);
                    return 7;
                }
                num_lines--;
                //printf("num_lines=%d\n", num_lines);
            }
            else if(e==LINECLIENT_CLOSED)
            {
                if(num_lines>0)
                {
                    test_strings_destruct(&ts);
                    lineclient_destruct(&client);
                    return 8;
                }
            }
            lineclient_dequeue(&client);
        }
    }
    test_strings_destruct(&ts);
    lineclient_destruct(&client);
    return 0;
}

void
test_strings_construct(test_strings *ts, int num_strings,
                            int min_len, int max_len,
                            int min_wait, int max_wait)
{
    int i;
    int last_wait=0;
    ts->num_strings=num_strings;
    ts->strings=(test_string*)malloc(sizeof(test_string)*num_strings);
    ts->buf=(char*)malloc(max_len*num_strings);
    ts->next_send=0;
    ts->next_receive=0;
    for(i=0; i<ts->num_strings; i++)
    {
        ts->strings[i].len=rand_range(min_len, max_len);
        last_wait+=rand_range(min_wait, max_wait);
        ts->strings[i].time=start_time+last_wait;
        ts->strings[i].str=ts->buf+(max_len*i);
        generate_line(ts->strings[i].str, ts->strings[i].len);
    }
}

void
test_strings_destruct(test_strings *ts)
{
    free(ts->strings);
    free(ts->buf);
}

int
test_strings_send(test_strings *ts, const char **buf, int *len)
{
    test_string *t;
    if(ts->next_send>=ts->num_strings)
        return -1; //done sending
    t=ts->strings+ts->next_send;
    //printf("time=%d\n", time(NULL));
    if(time(NULL)>=t->time)
    {
        *buf=t->str;
        *len=t->len;
        return 1;
    }
    return 0;
}
void
test_strings_sent(test_strings *ts)
{
    if(ts->next_send<ts->num_strings)
        (ts->next_send)++;
}

int
test_strings_received(test_strings *ts, const char *buf, int len)
{
    test_string *t;
    if(ts->next_receive>=ts->num_strings)
        return -1; //done already
    t=ts->strings+ts->next_receive;
    if(len!=t->len)
        return 0;
    while(len--)
    {
        if(buf[len]!=t->str[len])
            return 0;
    }
    (ts->next_receive)++;
    return 1;
}

void
generate_line(char *buf, int len)
{
    while(len--)
    {
        do
            buf[len]=(char)rand();
        while(buf[len]=='\n' || buf[len]=='\0' || buf[len]=='\r'
              //|| (!isalnum(buf[len])));
              );
    }
}

int
read_args(int argc, char *argv[])
{
    char opt;
    while((opt=getopt(argc, argv, "n:s:w:"))!=-1)
    {
        switch(opt)
        {
        case 'n':
            num_lines=atoi(optarg);
            if(num_lines<0)
            {
                fprintf(stderr, "Error: invalid number of lines.\n");
                return 0;
            }
            break;
        case 's':
            if(!get_min_max(optarg, &min_size, &max_size))
            {
                fprintf(stderr, "Error: invalid size range.\n");
                return 0;
            }
            break;
        case 'w':
            if(!get_min_max(optarg, &min_wait, &max_wait))
            {
                fprintf(stderr, "Error: invalid wait range.\n");
                return 0;
            }
            break;
        }
    }
    if(argc-optind<=1)
    {
        print_usage_str();
        return 0;
    }
    host=argv[optind];
    port=atoi(argv[optind+1]);
    return 1;
}

int get_min_max(char *buf, int *min, int *max)
{
    int i, len, comma_pos=-1;
    char c;
    len=strlen(buf);
    for(i=0; i<len; i++)
    {
        c=buf[i];
        if(!isdigit(c))
        {
            if(comma_pos==-1 && c==',')
                comma_pos=i;
            else
                return 0;
        }
    }
    buf[comma_pos]='\0';
    *min=atoi(buf);
    *max=atoi(buf+comma_pos+1);
    if(min<0 || max<0)
        return 0;
    return 1;
}

inline int
rand_range(int min, int max)
{
    return min+rand()%(max-min);
}

void
print_usage_str()
{
    fprintf(stderr, usage_fmt, def_num_lines, def_min_size, def_max_size,
            def_min_wait, def_max_wait);
}

void
print_test_strings(test_strings *ts)
{
    int i;
    test_string *t;
    for(i=0; i<ts->num_strings; i++)
    {
        t=ts->strings+i;
        printf("time=%d\n", t->time);
        printf("string=");
        print_string(t->str, t->len);
        printf("\n\n");
    }
}

void
print_string(const char *buf, int len)
{
    int i;
    for(i=0; i<len; i++)
        printf("%c", buf[i]);
}
