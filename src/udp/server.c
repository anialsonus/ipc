#include <arpa/inet.h>
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
    struct sockaddr_in address;

    /* Init socket file descriptor */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        handle_error("Failed to create a UDP socket");

    /* Set socket receive buffer size */
    sockbuf = RCV_BUFFER_SIZE;
    res = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &sockbuf, sizeof(sockbuf));
    if (res < 0)
        handle_error("Failed to set receive buffer length");

    /* Assign address to the socket descriptor */
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(SOCKET_ADDR);
    address.sin_port = SOCKET_PORT;
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
            exit(EXIT_FAILURE);
        }
    }
}