#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "ipc.h"

#define TRANSMISSIONS 100

#define SND_BUFFER_SIZE 512 * 1024 - 1

int
main (int argc, char **argv)
{
    int sockbuf;
    char writebuf[PACKET_SIZE];
    int res;
    int sockfd;
    int trans;
    ssize_t size;
    struct pollfd pfds[1];
    struct sockaddr_un address;

    if (access(SOCKET_PATH, R_OK) < 0)
        handle_error("Failed to access socket file");

    /* Init socket file descriptor */
    sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (sockfd < 0)
        handle_error("Failed to create a Unix socket");

    /* Set socket send buffer size */
    sockbuf = SND_BUFFER_SIZE;
    res = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sockbuf, sizeof(sockbuf));
    if (res < 0)
        handle_error("Failed to set send buffer length");

    /* Init the address */
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_LOCAL;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);
    address.sun_len = sizeof(SOCKET_PATH) - 1;

    /* Prepare batch of data to send (filled with zeroes at the moment) */
    memset(&writebuf, 0, sizeof(writebuf));

    /* Send packets to the socket when there is enough space in the buffer */
    pfds[0].fd = sockfd;
    pfds[0].events = POLLOUT;


    trans = TRANSMISSIONS;
    while(trans > 0)
    {
        res = poll(pfds, 1, -1);
        if (res < 0)
            handle_error("Failed to start poll() on the socket");
        
        if(pfds[0].revents & POLLOUT)
        {
            size = sendto(pfds[0].fd, &writebuf, sizeof(writebuf), 0, (const struct sockaddr *) &address, sizeof(address));
            if (size < 0)
                handle_error("Failed to send data to the socket");
            printf("Sent %lu bytes to the socket\n", size);
            trans--;
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

    exit(EXIT_SUCCESS);
}