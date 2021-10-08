#ifndef IPC_H
#define IPC_H

#define PACKET_SIZE 63 * 1024
#define SOCKET_PATH "/tmp/.unix.sock"
#define SOCKET_ADDR "127.0.0.1"
#define SOCKET_PORT 8888

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while(0)

#endif