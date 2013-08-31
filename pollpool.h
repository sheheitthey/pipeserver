/* pollpool.h
   A structure representing a pool of pollconns that can be logically allocated
   and with pfd pointers in sync with an array of pollfds.

   Allocation of the ufds member is not handled by this structure.
*/

#ifndef __POLLPOOL_H
#define __POLLPOOL_H

#include"pollconn.h"

typedef struct pollpool
{
    int len;
    pollconn *pollconns;
    struct pollfd *ufds;
    int *freelist;
    int freelist_head;
    int freelist_len;
    int sendbuf_len;
    int recvbuf_len;
} pollpool;

//Create a pollpool
void pollpool_construct(pollpool *pp, struct pollfd *ufds, int len,
                        int sendbuf_len, int recvbuf_len);

//Destroy a pollpool
void pollpool_destruct(pollpool *pp);

//Resize a pollpool
void pollpool_resize(pollpool *pp, struct pollfd *ufds, int newlen);
// NOTE: unpredictable results when resizing a non-empty pollpool
// Maybe pack before resizing? Still lose some if newlen too small though

//Allocate a pollconn from the pollpool
pollconn *pollpool_new(pollpool *pp);
// Returns NULL if no space remaining.

//Deallocate a pollconn from the pollpool
int pollpool_free(pollpool *pp, pollconn *c);

//Poll everything in the pollpool
int pollpool_poll(pollpool *pp, int timeout);

//The internal index of a pollconn in the pollpool
int pollpool_index(const pollpool *pp, const pollconn *c);

#endif
