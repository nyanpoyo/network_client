#ifndef CLIENT_CLIENT_UTIL_H
#define CLIENT_CLIENT_UTIL_H

#include "../my_lib/my_net.h"
#include "setting.h"
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>

enum Mode {
    CLIENT,
    SERVER
};

enum Mode setMode(char *_name, in_port_t _port);
void client_mainloop();

#endif //CLIENT_CLIENT_UTIL_H
