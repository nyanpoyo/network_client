#include "client_util.h"

enum Mode initializeMode(const int *sock, const struct sockaddr_in *broadcast_adrs, struct sockaddr_in *from_adrs,
                         my_packet *packet) {
    int broadcast_sw = 1;
    fd_set mask, readfds;
    struct timeval timeout;
    int ask_count = 0;
    enum Mode mode;

    if (setsockopt(*sock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcast_sw, sizeof(broadcast_sw)) == -1) {
        exit_errmesg("setsockopt()");
    }

    /* ビットマスクの準備 */
    FD_ZERO(&mask);
    FD_SET(*sock, &mask);

    while (1) {
        /* 受信データの有無をチェック */
        readfds = mask;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = 0;

        if (select(*sock + 1, &readfds, NULL, NULL, &timeout) == 0) {
            ask_count++;
            fprintf(stderr, "time out\n");
            if (ask_count < 3) {
                my_sendto(*sock, ASK_PACKET, strlen(ASK_PACKET), 0, (struct sockaddr *) broadcast_adrs,
                          sizeof(*broadcast_adrs));
            } else {
                mode = SERVER;
                break;
            }
        } else {
            if (FD_ISSET(*sock, &readfds)) {
                char r_buf[BUFF_SIZE];
                socklen_t from_len = sizeof(*from_adrs);
                int str_size = my_recvfrom(*sock, r_buf, BUFF_SIZE - 1, 0, (struct sockaddr *) from_adrs, &from_len);
                r_buf[str_size] = '\0';
                packet = (my_packet *) r_buf;
                if (strcmp(packet->header, ACK_PACKET) == 0) {
                    mode = CLIENT;
                    break;
                }
            }
        }
    }
    return mode;
}