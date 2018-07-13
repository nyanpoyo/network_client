#ifndef SERVER_SERVER_UTIL_H
#define SERVER_SERVER_UTIL_H

#include "../my_lib/my_net.h"
#include "setting.h"
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>


typedef struct MyPacket {
    char header[4];   /* パケットのヘッダ部(4バイト) */
    char sep;         /* セパレータ(空白、またはゼロ) */
    char data[];      /* データ部分(メッセージ本体) */
} my_packet;

typedef struct MemberInfo {
    char username[NAME_LENGTH];     /* ユーザ名 */
    int sock;                     /* ソケット番号 */
    struct MemberInfo *next;        /* 次のユーザ */
} *mem_info;

void server_mainloop();
void initializeServer(char *_name, in_port_t _port);
#endif //SERVER_SERVER_UTIL_H
