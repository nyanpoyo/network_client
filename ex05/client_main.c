#include "../my_lib/my_net.h"
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include "client_util.h"

int main(int argc, char *argv[]) {
    int opt;
    in_port_t port = DEFAULT_PORT;
    char *name;

    int sock;
    char s_buf[S_BUFSIZE], r_buf[R_BUFSIZE];
    struct sockaddr_in broadcast_adrs;
    struct sockaddr_in from_adrs;

    my_packet packet;

    enum Mode mode;

    while ((opt = getopt(argc, argv, "n:p:")) != -1) {
        switch (opt) {
            case 'n':
                if (strlen(optarg) > NAME_LENGTH) {
                    fprintf(stderr, "Too long name\n");
                    name = DEFAULT_NAME;
                } else {
                    name = optarg;
                }
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case '?':
                fprintf(stderr, "Unknow option '%c'\n", optopt);
                break;
        }
    }

    sock = init_udpclient();

    c_set_sockaddr_in_broadcast(&broadcast_adrs, port);

    mode = initializeMode(&sock, &broadcast_adrs, &from_adrs, &packet);

    switch (mode) {
        case SERVER:
            printf("I am server\n");
            break;
        case CLIENT:
            printf("I am client\n");
            break;
        default:
            fprintf(stderr, "Mode Choise Error!\n");
            return -1;
    }
    close(sock);             /* ソケットを閉じる */

    exit(EXIT_SUCCESS);
}