#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "my_net.h"

#define MAX_SPLIT_SIZE 50

void exit_errmesg(char *errmesg) {
    perror(errmesg);
    exit(EXIT_FAILURE);
}

int split(char *str, const char *delim, char *out_list[]) {
    char *tk;
    int size = 0;

    tk = strtok(str, delim);
    while (tk != NULL && size < MAX_SPLIT_SIZE) {
        out_list[size++] = tk;
        tk = strtok(NULL, delim);
    }
    return size;
}

int *malloc_thread_args() {
    int *thread_args;
    if ((thread_args = (int *) malloc(sizeof(int))) == NULL) {
        exit_errmesg("malloc()");
    }
    return thread_args;
}

char *chop_nl(char *str) {
    int len = strlen(str);
    if (str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
    return str;
}

int my_sendto(int sock, const void *s_buf, size_t strsize, int flags, const struct sockaddr *to, socklen_t tolen) {
    int r;
    if ((r = sendto(sock, s_buf, strsize, 0, to, tolen)) == -1) {
        exit_errmesg("sendto()");
    }

    return (r);
}

int my_recvfrom(int sock, void *r_buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen) {
    int r;
    if ((r = recvfrom(sock, r_buf, len, 0, from, fromlen)) == -1) {
        exit_errmesg("recvfrom()");
    }

    return (r);
}
