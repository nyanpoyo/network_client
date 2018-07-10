#include "../my_lib/my_net.h"
#include "server_util.h"

int main(int argc, char *argv[]) {
    int opt;
    in_port_t port = DEFAULT_PORT;

    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case '?':
                fprintf(stderr, "Unknow option '%c'\n", optopt);
                break;
        }
    }

    initialize(port);

    mainloop();
}