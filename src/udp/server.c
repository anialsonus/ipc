#include <arpa/inet.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "../ipc.h"

#define RCV_BUFFER_SIZE 512 * 1024 - 1

/* Non-atomic, but it is ok for our test case */
static volatile uint64_t totalsize;

void termination_handler(int signum)
{
    printf("Total bytes: %llu\n", totalsize);
    exit(EXIT_SUCCESS);
}

int
main (int argc, char **argv)
{
    int sockbuf;
    char readbuf[PACKET_SIZE];
    int res;
    int sockfd;
    ssize_t size;
    ssize_t packsize;
    struct pollfd pfds[1];
    struct sockaddr_in address;
    struct sigaction handler;

    totalsize = 0;

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

    /* Set signal handler */
    handler.sa_handler = termination_handler;
    sigemptyset(&handler.sa_mask);
    handler.sa_flags = 0;
    if (sigaction(SIGINT, &handler, NULL) < 0)
        handle_error("Failed to set a signal handler");

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
            else
                totalsize += size;
#ifdef DEBUG
            printf("Received %lu bytes from the socket\n", size);
#endif
        }
        else
        {
            printf("Total bytes: %llu\n", totalsize);
            exit(EXIT_FAILURE);
        }
    }
}