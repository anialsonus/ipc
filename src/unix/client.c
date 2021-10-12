#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>

#include "../ipc.h"

#define TRANSMISSIONS 1000000

#define SND_BUFFER_SIZE 4 * PACKET_SIZE

static volatile uint64_t totalsize = 0;

/* Signum of the pending signal */
static volatile sig_atomic_t signal_pending = 0;
/* Do we have a pending signal? */
static volatile sig_atomic_t defer_signal = 0;

void termination_handler(int signum)
{
    if (defer_signal)
    {
        signal_pending = signum;
    }
    else
    {
        printf("Total bytes: %" PRIu64 "\n", totalsize);
        exit(EXIT_SUCCESS);
    }
}

int
main (int argc, char **argv)
{
    int sockbuf;
    int flags;
    char writebuf[PACKET_SIZE];
    int res;
    int sockfd;
    ssize_t size;
    struct pollfd pfds[1];
    struct sockaddr_un address;
    struct sigaction handler;
    volatile sig_atomic_t do_repeat;


    if (access(SOCKET_PATH, R_OK) < 0)
        handle_error("Failed to access socket file");

    /* Init socket file descriptor */
    sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (sockfd < 0)
        handle_error("Failed to create a Unix socket");

    /* Make socket non-blocking */
    flags = fcntl(sockfd, F_GETFL, 0);
    if(flags < 0)
        handle_error("Failed to get socket descriptor flags");
    if(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
        handle_error("Failed to add non-blocking flag to the socket");

    /* Set socket send buffer size */
    sockbuf = SND_BUFFER_SIZE;
    res = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sockbuf, sizeof(sockbuf));
    if (res < 0)
        handle_error("Failed to set send buffer length");

    /* Init the address */
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_LOCAL;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    /* Prepare batch of data to send (filled with zeroes at the moment) */
    memset(&writebuf, 0, sizeof(writebuf));

    /* Set signal handler */
    handler.sa_handler = termination_handler;
    sigemptyset(&handler.sa_mask);
    handler.sa_flags = 0;
    if (sigaction(SIGINT, &handler, NULL) < 0)
        handle_error("Failed to set a signal handler");

    /* Send packets to the socket when there is enough space in the buffer */
    pfds[0].fd = sockfd;
    pfds[0].events = POLLOUT;

    do_repeat = 1;
    while(do_repeat)
    {
        res = poll(pfds, 1, -1);
        if (res < 0)
            handle_error("Failed to start poll() on the socket");
        
        if(pfds[0].revents & POLLOUT)
        {
            while ((size = sendto(pfds[0].fd, &writebuf, sizeof(writebuf), 0, (const struct sockaddr *) &address, sizeof(address))) >= 0)
            {
                if(size != sizeof(writebuf))
                    printf("Send: %" PRIu64 ", expected: %" PRIu64, size, sizeof(writebuf));

                /* Block SIGINT while update critical section */
                defer_signal++;

                /* Update counter */
                totalsize += size;

                /* Unblock critical section and reraise SIGINT */
                defer_signal--;
                if (!defer_signal && signal_pending != 0)
                    raise(signal_pending);

                if ((uint64_t) TRANSMISSIONS * PACKET_SIZE <= totalsize)
                {
                    do_repeat = 0;
                    break;
                }
            }
            if ((errno != EAGAIN) && (errno != ENOBUFS))
                perror("Failed to send data to the socket");
#ifdef DEBUG
            printf("Sent %lu bytes to the socket\n", size);
#endif
        }
        else if (pfds[0].revents & POLLHUP)
        {
            printf("Total bytes: %" PRIu64 "\n", totalsize);
            exit(EXIT_FAILURE);
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

    printf("Total bytes: %" PRIu64 "\n", totalsize);
    exit(EXIT_SUCCESS);
}