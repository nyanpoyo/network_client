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

//typedef struct Mypacket {
//    char header[4];   /* パケットのヘッダ部(4バイト) */
//    char sep;         /* セパレータ(空白、またはゼロ) */
//    char data[];      /* データ部分(メッセージ本体) */
//} my_packet;

enum Mode setMode(char *_name, in_port_t _port);
void client_mainloop();
void initializeClient();

#endif //CLIENT_CLIENT_UTIL_H
