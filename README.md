pipeserver
==========

A super-server, inspired by inetd, but for services with two way interactive
traffic over one TCP connection per client.

After getting some experience with using poll() for monitoring multiple
sockets on Linux, I felt that I could generalize and reuse the low level
buffer management work for multiple applications, including an IRC-like chat
system I had previously developed on Windows for fun.

The tcpbuf, pollsock, and pollset files implement that low level
functionality. The linepiper and pipeserver files build on top of them to
implement the super-server. The protocol is that the super-server will launch
an application server process and communicate with it through one-line
commands over STDIN/STDOUT. For example, the application server may receive
a PIPESERVER_RECEIVED event over its STDIN, and it could also send a
PIPESERVER_SEND command over its STDOUT. This makes it possible to write a
server application without dealing with any socket code - not even in the form
of importing a library.

In particular, the server application doesn't even have to be written in C.
echoserver and lisp_server are two example application servers for testing
pipeserver. echoserver is written in Perl, and it just echoes back whatever
the clients send. lisp_server is written in Common Lisp, and it serves up a
shared (and not at all secure) Common Lisp evaluator to all the clients that
connect to it.
