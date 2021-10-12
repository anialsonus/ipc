#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <mqueue.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "../ipc.h"

#define NOTIFY_SIG SIGUSR1

#define TRANSMISSIONS 1000000

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
main (int arc, char **argv)
{
    int flags;
    int res;
    char writebuf[PACKET_SIZE];
    mqd_t mqd;
    struct mq_attr attr;
    struct sigaction handler;
    ssize_t size;
    volatile sig_atomic_t do_repeat;

    /* Connect to the message queue */
    attr.mq_maxmsg = 4;
    attr.mq_msgsize = PACKET_SIZE;
    flags = O_WRONLY | O_NONBLOCK;
    mqd = mq_open((const char *) QUEUE_NAME, flags, 0777, &attr);
    if (mqd < 0)
        handle_error("Failed to open Posix queue descriptor");

    /* Set signal handler */
    handler.sa_handler = termination_handler;
    sigemptyset(&handler.sa_mask);
    handler.sa_flags = 0;
    if (sigaction(SIGINT, &handler, NULL) < 0)
        handle_error("Failed to set a signal handler");

    /* Prepare batch of data to send (filled with zeroes at the moment) */
    memset(&writebuf, 0, sizeof(writebuf));

    do_repeat = 1;
    while(do_repeat)
    {
        while (mq_send(mqd, writebuf, sizeof(writebuf), 0) == 0)
        {
            /* Block SIGINT while update critical section */
            defer_signal++;

            /* Update counter */
            totalsize += sizeof(writebuf);

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
        if (errno != EAGAIN)
            perror("Failed to send data to the queue");
    }

    printf("Total bytes: %" PRIu64 "\n", totalsize);
    exit(EXIT_SUCCESS);
}