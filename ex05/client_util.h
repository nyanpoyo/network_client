#ifndef CLIENT_CLIENT_UTIL_H
#define CLIENT_CLIENT_UTIL_H
#include "../my_lib/my_net.h"
#include "setting.h"

enum Mode {
    CLIENT,
    SERVER
};

typedef struct Mypacket {
    char header[4];   /* パケットのヘッダ部(4バイト) */
    char sep;         /* セパレータ(空白、またはゼロ) */
    char data[];      /* データ部分(メッセージ本体) */
} my_packet;

typedef struct MemberInfo {
    char username[NAME_LENGTH];     /* ユーザ名 */
    int sock;                     /* ソケット番号 */
    struct MemberInfo *next;        /* 次のユーザ */
} *mem_info;

enum Mode initializeMode(const int *sock, const struct sockaddr_in *broadcast_adrs, struct sockaddr_in *from_adrs, my_packet *packet);

#endif //CLIENT_CLIENT_UTIL_H
