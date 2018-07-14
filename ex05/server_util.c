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
static char *name;

static void deletefromList(mem_info delete_node);

static void addInList(mem_info new_node);

static u_int32_t analyze_header(char *header);

static void login();

static void postMessage(int _sock);

static mem_info newNode(char user_name[NAME_LENGTH], int sock);

static char *getNameInList(int sock);

static void showList();

static void create_packet(u_int32_t type, char *message);

static void checkUdpConnect(int signo);

static void clearBuf();

void initializeServer(char *_name, in_port_t _port) {
    name = _name;
    port = _port;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;

    my_set_sockaddr_in_broadcast(&broadcast_adrs, port);

    sock_udp = init_udpserver(port);
    sock_listen = init_tcpserver(port, 5);
    head.next = NULL;
    head.sock = DUMMY_SOCK;
    mem_p = &head;

    if (setsockopt(sock_udp, SOL_SOCKET, SO_BROADCAST, (void *) &broadcast_sw, sizeof(broadcast_sw)) ==
        -1) {
        exit_errmesg("setsockopt()");
    }
}

void server_mainloop() {
    fd_set mask, readfds;
    FD_ZERO(&mask);
    struct sigaction action;
    /* シグナルハンドラを設定する */
    action.sa_handler = checkUdpConnect;

    if (sigfillset(&action.sa_mask) == -1) {
        exit_errmesg("sigfillset()");
    }
    action.sa_flags = 0;
    if (sigaction(SIGIO, &action, NULL) == -1) {
        exit_errmesg("sigaction()");
    }
    /* ソケットの所有者を自分自身にする */
    if (fcntl(sock_udp, F_SETOWN, getpid()) == -1) {
        exit_errmesg("fcntl():F_SETOWN");
    }

    /* ソケットをノンブロッキング, 非同期モードにする */
    if (fcntl(sock_udp, F_SETFL, O_NONBLOCK | O_ASYNC) == -1) {
        exit_errmesg("fcntl():F_SETFL");
    }

    while (1) {
        if (mem_p->sock != DUMMY_SOCK) { //joinされてリストにsockが格納されてから通信を始めることを保証
            mem_info p = mem_p;
            mem_info sock_p = mem_p;

            FD_SET(sock_tcp, &mask);
            FD_SET(0, &mask);
            while (p->next != NULL) {
                FD_SET(p->sock, &mask);
                p = p->next;
            }

            readfds = mask;
            select(sock_tcp + 1, &readfds, NULL, NULL, NULL);

            while (sock_p->next != NULL) {
                if (FD_ISSET(sock_p->sock, &readfds)) {
                    my_receive(sock_p->sock, buf, BUFF_SIZE - 1);
                    packet = (my_packet *) buf;

                    switch (analyze_header(packet->header)) {
                        case POST: {
                            postMessage(sock_p->sock);
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
                sock_p = sock_p->next;
            }

            sock_p = mem_p;

            if (FD_ISSET(0, &readfds)) {
                char input_buff[BUFF_SIZE];
                char message[BUFF_SIZE];
                fgets(input_buff, BUFF_SIZE, stdin);
                chopNl(input_buff, BUFF_SIZE);
                snprintf(message, BUFF_SIZE, "[%s] %s", name, input_buff);
                create_packet(MESSAGE, message);
                while (sock_p->next != NULL) {
                    my_send(sock_p->sock, buf, strlen(buf), SO_NOSIGPIPE);
                    sock_p = sock_p->next;
                }
                printf("[input] %s\n", buf);
                clearBuf();
            }
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

static void postMessage(int _sock) {
    char message[BUFF_SIZE];
    snprintf(message, BUFF_SIZE, "[%s] %s", getNameInList(_sock), packet->data);
    create_packet(MESSAGE, message);
    chopNl(buf, BUFF_SIZE);
    mem_info p = mem_p;
    while (p->next != NULL) {
        my_send(p->sock, buf, BUFF_SIZE, 0);
        p = p->next;
    }
    printf("[post] %s\n", buf);
    clearBuf();
}

static void checkUdpConnect(int signo) {

    socklen_t from_len = sizeof(from_adrs);
    if (my_recvfrom(sock_udp, buf, BUFF_SIZE - 1, 0, (struct sockaddr *) &from_adrs, &from_len) == -1 &&
        errno == EWOULDBLOCK) {
        printf("[INFO] EWOULDBLOCK\n");
        return;
    }
    packet = (my_packet *) buf;
    if (strcmp(packet->header, "HELO") == 0) {
        create_packet(HERE, "");
        my_sendto(sock_udp, buf, BUFF_SIZE, 0, (struct sockaddr *) &from_adrs, from_len);
    }

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