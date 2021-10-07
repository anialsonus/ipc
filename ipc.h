#ifndef IPC_H
#define IPC_H

#define BUFFER_SIZE 512 * 1024
#define PACKET_SIZE 512 * 1024
#define SOCKET_PATH "/tmp/.unix.sock"

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while(0)

#endif