#include "server_util.h"

static struct MemberInfo head;
static mem_info mem_p;
static char buf[BUFF_SIZE] = {'\0'};
static my_packet *packet;
static int sock_udp, sock_tcp;
static int sock_listen;
static struct sockaddr_in broadcast_adrs;
static struct sockaddr_in from_adrs;
static in_port_t port;
static int broadcast_sw = 1;

static void deletefromList(mem_info delete_node);

static void addInList(mem_info new_node);

static u_int32_t analyze_header(char *header);

static void login();

static void postMessage();

static mem_info newNode(char user_name[NAME_LENGTH], int sock);

static char *getNameInList(int sock);

static void showList();

static void create_packet(u_int32_t type, char *message);

static int hasConnectedUdp();


void initialize(in_port_t _port) {
    port = _port;
    my_set_sockaddr_in_broadcast(&broadcast_adrs, port);

    sock_udp = init_udpserver(port);
    sock_listen = init_tcpserver(DEFAULT_PORT, 5);
    head.next = NULL;
    if (setsockopt(sock_udp, SOL_SOCKET, SO_BROADCAST, (void *) &broadcast_sw, sizeof(broadcast_sw)) ==
        -1) {
        exit_errmesg("setsockopt()");
    }
}

void mainloop() {
    while (1) {
        if (hasConnectedUdp()) {
            sock_tcp = my_accept(sock_listen, NULL, NULL);
            break;
        }
    }
    while (1) {
        my_receive(sock_tcp, buf, BUFF_SIZE - 1);
        packet = (my_packet *) buf;

        switch (analyze_header(packet->header)) {
            case JOIN: {
                login();
                for (int i = 0; i < BUFF_SIZE; ++i) {
                    buf[i] = '\0';
                }
                break;
            }
            case POST: {
                postMessage();
                break;
            }
            case QUIT: {
                break;
            }
            default:
                break;
        }
    }
}

static void login() {
    printf("[INFO] login\n");
    fflush(stdout);
    mem_info new_node = newNode(packet->data, sock_tcp);
    addInList(new_node);
}

static void postMessage() {
    printf("[INFO] post\n");
    fflush(stdout);
    char message[BUFF_SIZE];
    snprintf(message, BUFF_SIZE, "[%s] %s", getNameInList(sock_tcp), packet->data);
    create_packet(MESSAGE, message);
    chopNl(buf);
    printf("sock %d\n",sock_udp);
    my_sendto(sock_udp, buf, BUFF_SIZE, 0, (struct sockaddr *) &broadcast_adrs, sizeof(broadcast_adrs));
}

static int hasConnectedUdp() {
    fd_set mask, readfds;

    FD_ZERO(&mask);
    FD_SET(sock_udp, &mask);

    int has_connected = 0;

    while (1) {
        readfds = mask;
        select(sock_udp + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(sock_udp, &readfds)) {
            socklen_t from_len = sizeof(from_adrs);
            my_recvfrom(sock_udp, buf, BUFF_SIZE - 1, 0, (struct sockaddr *) &from_adrs, &from_len);
            packet = (my_packet *) buf;
            if (strcmp(packet->header, "HELO") == 0) {
                create_packet(HERE, "");
                my_sendto(sock_udp, buf, BUFF_SIZE, 0, (struct sockaddr *) &from_adrs, from_len);
                has_connected = 1;
                break;
            } else {
                has_connected = 0;
                break;
            }
        }
    }
    return has_connected;
}

static mem_info newNode(char user_name[NAME_LENGTH], int sock) {
    mem_info new_node = malloc(sizeof(struct MemberInfo));
    new_node->sock = sock;
    strcpy(new_node->username, user_name);
    new_node->next = NULL;
    return new_node;
}

static char *getNameInList(int sock) {
    mem_info p = mem_p;
    while ((p->sock != sock) && (p->next != NULL)) {
        p = p->next;
    }
    return p->username;
}

static void addInList(mem_info new_node) {
    mem_p = &head;
    new_node->next = mem_p;
    mem_p = new_node;
}

static void deletefromList(mem_info delete_node) {
    mem_info *pp;
    pp = &((&head)->next);
    while (((*pp)->username != delete_node->username) && ((*pp)->sock != delete_node->sock)) {
        pp = &((*pp)->next);
    }
    if ((*pp)->next != NULL) {
        mem_info temp = (*pp)->next;
        free(*pp);
        *pp = temp;
    } else {
        free(*pp);
        *pp = NULL;
    }
}

static void showList() {
    mem_info p = mem_p;
    while (p->next != NULL) {
        printf("name:%s\tsock:%d\n", p->username, p->sock);
        fflush(stdout);
        p = p->next;
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