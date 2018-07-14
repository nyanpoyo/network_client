#ifndef CLIENT_SETTING_H
#define CLIENT_SETTING_H

#define BUFF_SIZE 512   /* 送信用バッファサイズ */
#define DEFAULT_PORT 50001
#define DEFAULT_NAME "unknown"
#define TIMEOUT_SEC 3
#define NAME_LENGTH 15
#define DUMMY_SOCK -1
#define MESG_LENGTH 488

typedef enum Header {
    HELLO,
    HERE,
    JOIN,
    POST,
    MESSAGE,
    QUIT
} header_command;

#endif //CLIENT_SETTING_H
