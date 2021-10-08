#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "../ipc.h"

#define RCV_BUFFER_SIZE 512 * 1024 - 1

int
main (int argc, char **argv)
{
    int sockbuf;
    char readbuf[PACKET_SIZE];
    int res;
    int sockfd;
    ssize_t size;
    struct pollfd pfds[1];
    struct sockaddr_un address;

    /* Init socket file descriptor */
    sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (sockfd < 0)
        handle_error("Failed to create a Unix socket");

    /* Set socket receive buffer size */
    sockbuf = RCV_BUFFER_SIZE;
    res = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &sockbuf, sizeof(sockbuf));
    if (res < 0)
        handle_error("Failed to set receive buffer length");

    /* Delete an existing socket file if any */
    if (access(SOCKET_PATH, R_OK) < 0)
        handle_error("Failed to access the socket");
    if (unlink(SOCKET_PATH) < 0)
        handle_error("Failed to unlink the socket");

    /* Assign address to the socket descriptor */
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_LOCAL;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);
    address.sun_len = sizeof(SOCKET_PATH) - 1;
    if (bind(sockfd, (const struct sockaddr *) &address, sizeof(address)) < 0)
        handle_error("Failed to assign address to the socket");

    /* Wait for incoming data from the socket */
    pfds[0].fd = sockfd;
    pfds[0].events = POLLIN;

    while(1)
    {
        res = poll(pfds, 1, -1);
        if (res < 0)
            handle_error("Failed to start poll() on the socket");
        
        if(pfds[0].revents & POLLIN)
        {
            size = recvfrom(pfds[0].fd, &readbuf, sizeof(readbuf), 0, NULL, NULL);
            if (size < 0)
                perror("Failed to receive from the socket");
#ifdef DEBUG
            printf("Received %lu bytes from the socket\n", size);
#endif
        }
        else
        {
            if (close(pfds[0].fd) < 0)
                handle_error("Failed to close socket");
            if ((access(SOCKET_PATH, R_OK) == 0) && unlink(SOCKET_PATH) < 0)
                handle_error("Failed to unlink the socket");
            exit(EXIT_FAILURE);
        }
    }
}