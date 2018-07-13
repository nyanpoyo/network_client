#include "../my_lib/my_net.h"
#include "client_util.h"
#include "server_util.h"

int main(int argc, char *argv[]) {
    int opt;
    in_port_t port = DEFAULT_PORT;
    char *name = DEFAULT_NAME;

    enum Mode mode;

    while ((opt = getopt(argc, argv, "n:p:")) != -1) {
        switch (opt) {
            case 'n':
                if (strlen(optarg) > NAME_LENGTH) {
                    fprintf(stderr, "Too long name\n");
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

    mode = setMode(name,port);

    switch (mode) {
        case SERVER:
            initializeServer(name, port);
            server_mainloop();
            break;
        case CLIENT:
            initializeClient();
            client_mainloop();
            break;
    }

    exit(EXIT_SUCCESS);
}