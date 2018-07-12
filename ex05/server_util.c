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
static struct timeval timeout;


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

static void clearBuf();


void initialize(in_port_t _port) {
    port = _port;
    my_set_sockaddr_in_broadcast(&broadcast_adrs, port);

    sock_udp = init_udpserver(port);
    sock_listen = init_tcpserver(port, 5);
    head.next = NULL;
    mem_p = &head;

    if (setsockopt(sock_udp, SOL_SOCKET, SO_BROADCAST, (void *) &broadcast_sw, sizeof(broadcast_sw)) ==
        -1) {
        exit_errmesg("setsockopt()");
    }

    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
}

void mainloop() {
    fd_set mask, readfds;
    FD_ZERO(&mask);

    while (1) {
        if (hasConnectedUdp()) {
            sock_tcp = my_accept(sock_listen, NULL, NULL);

            my_receive(sock_tcp, buf, BUFF_SIZE - 1);
            packet = (my_packet *) buf;

            switch (analyze_header(packet->header)) {
                case JOIN: {
                    login();
                    showList();
                    break;
                }
                default:
                    break;
            }
            clearBuf();
            break;
        }
    }

    while (1) {

        mem_info p = mem_p;
        mem_info q = mem_p;

        FD_SET(sock_tcp, &mask);
        FD_SET(0, &mask);
        while (p->next != NULL) {
            FD_SET(p->sock, &mask);
            p = p->next;
        }

        readfds = mask;
        select(sock_tcp + 1, &readfds, NULL, NULL, NULL);

//        if (FD_ISSET(sock_tcp, &readfds)) {
//            my_receive(sock_tcp, buf, BUFF_SIZE - 1);
//            packet = (my_packet *) buf;
//
//            switch (analyze_header(packet->header)) {
//                case JOIN: {
//                    login();
//                    showList();
//                    break;
//                }
//                default:
//                    break;
//            }
//        }

        while (q->next != NULL) {
            if (FD_ISSET(q->sock, &readfds)) {
                my_receive(q->sock, buf, BUFF_SIZE - 1);
                packet = (my_packet *) buf;

                switch (analyze_header(packet->header)) {
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
                clearBuf();
            }
            q = q->next;
        }

        if (FD_ISSET(0, &readfds)) {
            char input_buff[BUFF_SIZE];
            fgets(input_buff, BUFF_SIZE, stdin);
            chopNl(input_buff);
            create_packet(MESSAGE, input_buff);
            my_send(sock_tcp, buf, strlen(buf));
            printf("[post] %s\n", buf);
        }
    }
}

static void clearBuf() {
    char *p = buf;
    while (*p != '\0') {
        *p = '\0';
        p++;
    }
}

static void login() {
    printf("[INFO] login\n");
    fflush(stdout);
    mem_info new_node = newNode(packet->data, sock_tcp);
    addInList(new_node);
}

static void postMessage() {
    char message[BUFF_SIZE];
    snprintf(message, BUFF_SIZE, "%s", packet->data);
    create_packet(MESSAGE, message);
    chopNl(buf);
    mem_info p = mem_p;
    while (p->next != NULL) {
        my_send(p->sock, buf, BUFF_SIZE);
        p = p->next;
    }
//    my_send(sock_tcp, buf, BUFF_SIZE);
    printf("[post] %s\n", buf);
}

static int hasConnectedUdp() {
    fd_set mask, readfds;

    FD_ZERO(&mask);
    FD_SET(sock_udp, &mask);

    static int has_connected = 0;

    readfds = mask;
    if (select(sock_udp + 1, &readfds, NULL, NULL, &timeout) == 0) {
        return 0;
    } else {
        if (FD_ISSET(sock_udp, &readfds)) {
            socklen_t from_len = sizeof(from_adrs);
            my_recvfrom(sock_udp, buf, BUFF_SIZE - 1, 0, (struct sockaddr *) &from_adrs, &from_len);
            packet = (my_packet *) buf;
            if (strcmp(packet->header, "HELO") == 0) {
                create_packet(HERE, "");
                my_sendto(sock_udp, buf, BUFF_SIZE, 0, (struct sockaddr *) &from_adrs, from_len);
                has_connected = 1;
            } else {
                has_connected = 0;
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