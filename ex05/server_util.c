#include "server_util.h"

#define EXEC_MODE

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
static int max_sd = 0;

static void deletefromList(int sock);

static void addInList(mem_info new_node);

static u_int32_t analyze_header(char *header);

static void login();

static void logout(int sock);

static void postMessage(int sock);

static mem_info newNode(char user_name[NAME_LENGTH], int sock);

static char *getNameInList(int sock);

static void showList();

static void create_packet(u_int32_t type, char *message);

static void checkUdpConnect();

static void clearBuf();

void initializeServer(char *_name, in_port_t _port) {
    name = _name;
    port = _port;
    timeout.tv_sec = 0;
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

    while (1) {

        checkUdpConnect();

        mem_info sock_p = mem_p;
        if (sock_p->sock != DUMMY_SOCK) { //joinされてリストにsockが格納されてから通信を始めることを保証
            FD_SET(0, &mask);
            while (sock_p->next != NULL) {
                FD_SET(sock_p->sock, &mask);
                sock_p = sock_p->next;
            }

            sock_p = mem_p;

            readfds = mask;
            select(max_sd + 1, &readfds, NULL, NULL, &timeout);

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
                            logout(sock_p->sock);
#ifdef DEBUG_MODE
                            showList();
#endif
                            FD_CLR(sock_p->sock, &mask);
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
#ifdef DEBUG_MODE
                printf("[input] %s\n", buf);
#elif defined(EXEC_MODE)
                printf("%s\n", message);
#endif
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
#ifdef DEBUG_MODE
    printf("[INFO] login\n");
#endif
    mem_info new_node = newNode(packet->data, sock_tcp);
    addInList(new_node);
}

static void logout(int sock) {
    deletefromList(sock);
    close(sock);
}

static void postMessage(int sock) {
    mem_info p = mem_p;
    char message[BUFF_SIZE];
    snprintf(message, BUFF_SIZE, "[%s] %s", getNameInList(sock), packet->data);
    create_packet(MESSAGE, message);
    chopNl(buf, BUFF_SIZE);

#ifdef DEBUG_MODE
    printf("[post] %s\n", buf);
#elif defined(EXEC_MODE)
    printf("%s\n", message);
#endif

    while (p->next != NULL) {
        my_send(p->sock, buf, BUFF_SIZE, SO_NOSIGPIPE);
        p = p->next;
    }
    clearBuf();
}

static void checkUdpConnect() {
    fd_set mask, readfds;
    FD_ZERO(&mask);
    FD_SET(sock_udp, &mask);

    readfds = mask;
    select(sock_udp + 1, &readfds, NULL, NULL, &timeout);

    if (FD_ISSET(sock_udp, &readfds)) {
#ifdef DEBUG_MODE
        printf("[INFO] detected UDP connection\n");
#endif
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
#ifdef DEBUG_MODE
            printf("[send] HERE packet\n");
#endif
        }

        sock_tcp = my_accept(sock_listen, NULL, NULL);
        max_sd = sock_tcp;

        my_receive(sock_tcp, buf, BUFF_SIZE - 1);
        packet = (my_packet *) buf;

        switch (analyze_header(packet->header)) {
            case JOIN: {
                login();
#ifdef DEBUG_MODE
                showList();
#endif
                break;
            }
            default:
                break;
        }
        clearBuf();
    }
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

static void deletefromList(int sock) {
    mem_info *pp;
    pp = &mem_p;
    while (((*pp)->sock != sock)) {
        pp = &((*pp)->next);
    }
    mem_info temp = (*pp)->next;
    free(*pp);
    *pp = temp;
}

static void showList() {
    mem_info p = mem_p;
    if (p->next == NULL) {
        printf("Nothing in List\n");
    }
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