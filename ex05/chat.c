#include "../my_lib/my_net.h"
#include "client_util.h"
#include "server_util.h"

int main(int argc, char *argv[]) {
    int opt;
    int has_set_mode = 0;
    in_port_t port = DEFAULT_PORT;
    char *name = DEFAULT_NAME;

    enum Mode mode;

    while ((opt = getopt(argc, argv, "SCn:p:")) != -1) {
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
            case 'S':
                initializeServer(name, DEFAULT_PORT);
                server_mainloop();
                break;
            case 'C':
                mode = setMode(name, port);
                has_set_mode = 1;
                break;
            case '?':
                fprintf(stderr, "Unknow option '%c'\n", optopt);
                break;
        }
    }

    if (!has_set_mode) {
        mode = setMode(name, port);
    }


    switch (mode) {
        case SERVER:
            initializeServer(name, port);
            server_mainloop();
            break;
        case CLIENT:
            client_mainloop();
            break;
    }

    exit(EXIT_SUCCESS);
}