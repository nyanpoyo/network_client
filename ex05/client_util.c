#include "client_util.h"
#include "server_util.h"

static char buf[BUFF_SIZE];
static struct sockaddr_in broadcast_adrs;
static struct sockaddr_in from_adrs;
static int broadcast_sw = 1;
static int sock_udp;
static int sock_tcp;
static char *name;
static in_port_t port;
static my_packet *packet;
static struct timeval timeout;

static void create_packet(u_int32_t type, char *message);

static void join();

static void receiveMessage();

static void postMessage();

static void clearBuf();

static u_int32_t analyze_header(char *header);

void initializeClient() {

}

enum Mode setMode(char *_name, in_port_t _port) {
    name = _name;
    port = _port;

    my_set_sockaddr_in_broadcast(&broadcast_adrs, port);
    sock_udp = init_udpclient();
    if (setsockopt(sock_udp, SOL_SOCKET, SO_BROADCAST, (void *) &broadcast_sw, sizeof(broadcast_sw)) == -1) {
        exit_errmesg("setsockopt()");
    }

    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;

    fd_set mask, readfds;
    int ask_count = 0;
    enum Mode mode;

    /* ビットマスクの準備 */

    FD_ZERO(&mask);
    FD_SET(sock_udp, &mask);
    FD_SET(0, &mask);

    create_packet(HELLO, "");
    my_sendto(sock_udp, buf, BUFF_SIZE, 0, (struct sockaddr *) &broadcast_adrs,
              sizeof(broadcast_adrs));

    while (1) {
        /* 受信データの有無をチェック */
        readfds = mask;

        if (select(sock_udp + 1, &readfds, NULL, NULL, &timeout) == 0) {
            ask_count++;
            fprintf(stderr, "time out\n");
            if (ask_count < 3) {
                create_packet(HELLO, "");
                my_sendto(sock_udp, buf, BUFF_SIZE, 0, (struct sockaddr *) &broadcast_adrs,
                          sizeof(broadcast_adrs));
            } else {
                mode = SERVER;
                break;
            }
        } else {
            if (FD_ISSET(sock_udp, &readfds)) {
                socklen_t from_len = sizeof(from_adrs);
                int str_size = my_recvfrom(sock_udp, buf, BUFF_SIZE - 1, 0, (struct sockaddr *) &from_adrs, &from_len);
                buf[str_size] = '\0';
                my_packet *packet;
                packet = (my_packet *) buf;
                if (strcmp(packet->header, "HERE") == 0) {
                    mode = CLIENT;
                    break;
                }
            }
        }
    }
    return mode;
}

void client_mainloop() {
    printf("[INFO] client loop\n");
    sock_tcp = init_tcpclient(inet_ntoa(from_adrs.sin_addr), port);

    fd_set mask, readfds;
    FD_ZERO(&mask);
    FD_SET(sock_tcp, &mask);
    FD_SET(0, &mask);

    join();

    while (1) {
        readfds = mask;
        select(sock_tcp + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(sock_tcp, &readfds)) {
            receiveMessage();
        }

        if (FD_ISSET(0, &readfds)) {
            postMessage();
        }
    }
}

static void join() {
    create_packet(JOIN, name);
    my_send(sock_tcp, buf, strlen(buf));
    clearBuf();
}

static void receiveMessage() {
    my_receive(sock_tcp, buf, BUFF_SIZE - 1);
    packet = (my_packet *) buf;

    switch (analyze_header(packet->header)) {
        case MESSAGE: {
            printf("[receive] %s\n", packet->data);
            break;
        }
    }
    clearBuf();
}

static void postMessage() {
    char input_buff[BUFF_SIZE];
    fgets(input_buff, BUFF_SIZE, stdin);
    my_packet *input = (my_packet *) input_buff;
    if (strcmp(input->header, "QUIT") == 0) {
        create_packet(QUIT, "");
        my_send(sock_tcp, buf, strlen(buf));
        printf("[post] %s\n", buf);
        close(sock_tcp);
        exit(0);
    } else {
        chopNl(input_buff, BUFF_SIZE);
        create_packet(POST, input_buff);
        my_send(sock_tcp, buf, strlen(buf));
        printf("[post] %s\n", buf);
    }
    clearBuf();
}

static void clearBuf() {
    char *p = buf;
    while (*p != '\0') {
        *p = '\0';
        p++;
    }
}

static void create_packet(u_int32_t type, char *message) {
    switch (type) {
        case HELLO:
            snprintf(buf, MESG_LENGTH, "HELO");
            break;
        case HERE:
            snprintf(buf, MESG_LENGTH, "HERE");
            break;
        case JOIN:
            snprintf(buf, MESG_LENGTH, "JOIN %s", message);
            break;
        case POST:
            snprintf(buf, MESG_LENGTH, "POST %s", message);
            break;
        case MESSAGE:
            snprintf(buf, MESG_LENGTH, "MESG %s", message);
            break;
        case QUIT:
            snprintf(buf, MESG_LENGTH, "QUIT");
            break;
        default:
            /* Undefined packet type */
            break;
    }
}

static u_int32_t analyze_header(char *header) {
    if (strncmp(header, "HELO", 4) == 0) { return (HELLO); }
    if (strncmp(header, "HERE", 4) == 0) { return (HERE); }
    if (strncmp(header, "JOIN", 4) == 0) { return (JOIN); }
    if (strncmp(header, "POST", 4) == 0) { return (POST); }
    if (strncmp(header, "MESG", 4) == 0) { return (MESSAGE); }
    if (strncmp(header, "QUIT", 4) == 0) { return (QUIT); }
    return 0;
}