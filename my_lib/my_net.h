/*
  mynet.h
*/
#ifndef MYNET_H_
#define MYNET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

int init_tcpserver(in_port_t myport, int backlog);

int init_tcpclient(char *servername, in_port_t serverport);

int init_udpserver(in_port_t myport);

int init_udpclient();

void my_set_sockaddr_in(struct sockaddr_in *server_adrs, char *servername, in_port_t port_number);

void my_set_sockaddr_in_broadcast(struct sockaddr_in *server_adrs, in_port_t port_number);

ssize_t my_receive(int sock, void *buf, size_t buf_size);

ssize_t my_send(int sock, const void *buf, size_t str_size);

void exit_errmesg(char *errmesg);

int split(char *str, const char *delim, char *outlist[]);

char *chop_nl(char *str);

void chopNl(char *str, int size);

int *malloc_thread_args();

pid_t my_fork();

void my_echo(int sock, char *buf, size_t buf_size);

int my_accept(int s, struct sockaddr *addr, socklen_t *addrlen);

int my_sendto(int sock, const void *s_buf, size_t strsize, int flags, const struct sockaddr *to, socklen_t tolen);

int my_recvfrom(int sock, void *r_buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);

#endif  /* MYNET_H_ */