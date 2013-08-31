CC=gcc
CFLAGS=-Wall -g
#CFLAGS=-Wall
LDFLAGS=
HEADERS=pollconn.h pollpool.h pollset.h pollsock.h linepiper.h lineserver.h lineclient.h pipeserver.h pipeserver_msg.h
pollconn_OBJ=pollconn.o pollsock.o tcpbuf.o
pollpool_OBJ=$(pollconn_OBJ) pollpool.o
pollset_OBJ=$(pollpool_OBJ) pollset.o
pollconn_test_OBJ=$(pollconn_OBJ) pollconn_test.o
pollpool_test_OBJ=$(pollpool_OBJ) pollpool_test.o
pollset_test_OBJ=$(pollset_OBJ) pollset_test.o
lineserver_OBJ=$(pollset_OBJ) lineserver.o
lineserver_test_OBJ=$(lineserver_OBJ) lineserver_test.o
lineclient_OBJ=$(pollconn_OBJ) lineclient.o
linepiper_OBJ=linepiper.o
pipeserver_OBJ=$(lineserver_OBJ) $(linepiper_OBJ) pipeserver.o pipeserver_msg.o
pipeserve_OBJ=$(pipeserver_OBJ) pipeserve.o
echo_test_OBJ=$(lineclient_OBJ) echo_test.o
all:pollconn_test pollpool_test pollset_test lineserver_test pipeserve echo_test

clean:
	rm pollconn_test pollpool_test pollset_test lineserver_test \
	    pipeserve echo_test *.o

pollconn_test:$(pollconn_test_OBJ) $(HEADERS)
	$(CC) -o pollconn_test $(LDFLAGS) $(pollconn_test_OBJ)

pollpool_test:$(pollpool_test_OBJ) $(HEADERS)
	$(CC) -o pollpool_test $(LD_FLAGS) $(pollpool_test_OBJ)

pollset_test:$(pollset_test_OBJ) $(HEADERS)
	$(CC) -o pollset_test $(LD_FLAGS) $(pollset_test_OBJ)

lineserver_test:$(lineserver_test_OBJ) $(HEADERS)
	$(CC) -o lineserver_test $(LD_FLAGS) $(lineserver_test_OBJ)

pipeserve:$(pipeserve_OBJ) $(HEADERS)
	$(CC) -o pipeserve $(LD_FLAGS) $(pipeserve_OBJ)

echo_test:$(echo_test_OBJ) $(HEADERS)
	$(CC) -o echo_test $(LD_FLAGS) $(echo_test_OBJ)

pollconn.o:pollconn.c $(HEADERS)
	$(CC) -c -o pollconn.o $(CFLAGS) pollconn.c

pollpool.o:pollpool.c $(HEADERS)
	$(CC) -c -o pollpool.o $(CFLAGS) pollpool.c

pollset.o:pollset.c $(HEADERS)
	$(CC) -c -o pollset.o $(CFLAGS) pollset.c

pollsock.o:pollsock.c $(HEADERS)
	$(CC) -c -o pollsock.o $(CFLAGS) pollsock.c

tcpbuf.o:tcpbuf.c $(HEADERS)
	$(CC) -c -o tcpbuf.o $(CFLAGS) tcpbuf.c

pollconn_test.o:pollconn_test.c $(HEADERS)
	$(CC) -c -o pollconn_test.o $(CFLAGS) pollconn_test.c

pollpool_test.o:pollpool_test.c $(HEADERS)
	$(CC) -c -o pollpool_test.o $(CFLAGS) pollpool_test.c

pollset_test.o:pollset_test.c $(HEADERS)
	$(CC) -c -o pollset_test.o $(CFLAGS) pollset_test.c

lineserver.o:lineserver.c $(HEADERS) lineserver.h
	$(CC) -c -o lineserver.o $(CFLAGS) lineserver.c

lineserver_test.o:lineserver_test.c $(HEADERS) lineserver.h
	$(CC) -c -o lineserver_test.o $(CFLAGS) lineserver_test.c

lineclient_test.o:lineclient_test.c $(HEADERS) lineclient.h
	$(CC) -c -o lineclient_test.o $(CFLAGS) lineclient_test.c

linepiper.o:linepiper.c $(HEADERS) linepiper.h
	$(CC) -c -o linepiper.o $(CFLAGS) linepiper.c

pipeserver.o:pipeserver.c $(HEADERS) pipeserver.h
	$(CC) -c -o pipeserver.o $(CFLAGS) pipeserver.c

pipeserver_msg.o:pipeserver_msg.c $(HEADERS) 
	$(CC) -c -o pipeserver_msg.o $(CFLAGS) pipeserver_msg.c

pipeserve.o:pipeserve.c $(HEADERS)
	$(CC) -c -o pipeserve.o $(CFLAGS) pipeserve.c

echo_test.o:echo_test.c $(HEADERS)
	$(CC) -c -o echo_test.o $(CFLAGS) echo_test.c
