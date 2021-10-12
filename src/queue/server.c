#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <mqueue.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>

#include "../ipc.h"

#define NOTIFY_SIG SIGUSR1

static volatile uint64_t totalsize = 0;

/* Signum of the pending signal */
static volatile sig_atomic_t signal_pending = 0;
/* Do we have a pending signal? */
static volatile sig_atomic_t defer_signal = 0;

static void termination_handler(int signum)
{
    if (defer_signal)
    {
        signal_pending = signum;
    }
    else
    {
        printf("Total bytes: %" PRIu64 "\n", totalsize);
        if(mq_unlink((const char *) QUEUE_NAME) < 0)
            handle_error("Failed to unlink Posix queue");
        exit(EXIT_SUCCESS);
    }
}

static void notify_handler(int signum)
{
    /* Just interrupt sigsuspend() */
}

int
main(int argc, char **argv)
{
    struct mq_attr attr;
    mqd_t mqd;
    mode_t mode;
    int flags;
    char readbuf[PACKET_SIZE];
    sigset_t blockMask, emptyMask;
    struct sigaction sa;
    struct sigaction handler;
    struct sigevent sev;
    ssize_t size;
    char qname[17];
    int res;

    sprintf(qname, QUEUE_NAME);

    /* Remove queue if exists */
    res = mq_unlink(qname);
    if ((res < 0) && (errno != ENOENT))
        handle_error("Failed to unlink Posix queue");

    /* Init the message queue */
    attr.mq_maxmsg = 4;
    attr.mq_msgsize = PACKET_SIZE;
    mode = S_IRUSR | S_IWUSR;
    flags = O_CREAT | O_RDWR | O_NONBLOCK;
    mqd = mq_open(qname, flags, 0777, &attr);
    if (mqd < 0)
        handle_error("Failed to open Posix queue descriptor");
    
    /* Block NOTIFY_SIG to establish its handler and queue events */
    sigemptyset(&blockMask);
    sigaddset(&blockMask, NOTIFY_SIG);
    if(sigprocmask(SIG_BLOCK, &blockMask, NULL) < 0)
        handle_error("Failed to set a blocking mask");
    
    /* Set NOTIFY_SIG handler */
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = notify_handler;
    if (sigaction(NOTIFY_SIG, &sa, NULL) < 0)
        handle_error("Failed to set a signal handler");

    /* Set signal events */
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = NOTIFY_SIG;
    if (mq_notify(mqd, &sev) < 0)
        handle_error("Failed to set a queue notification");

    /* Set SIGINT handler */
    handler.sa_handler = termination_handler;
    sigemptyset(&handler.sa_mask);
    handler.sa_flags = 0;
    if (sigaction(SIGINT, &handler, NULL) < 0)
        handle_error("Failed to set a signal handler");

    sigemptyset(&emptyMask);

    while (1)
    {
        /* Wait for notification signal */
        sigsuspend(&emptyMask);

        if (mq_notify(mqd, &sev) < 0)
            handle_error("Failed to set a queue notification");

        while ((size = mq_receive(mqd, readbuf, attr.mq_msgsize, NULL)) > 0)
        {
            /* Block SIGINT while update critical section */
            defer_signal++;

            /* Update counter */
            totalsize += size;

            /* Unblock critical section and reraise SIGINT */
            defer_signal--;
            if (!defer_signal && signal_pending != 0)
                raise(signal_pending);
        }
        if (errno != EAGAIN)
            perror("Failed to receive from the queue");
    }

    printf("Total bytes: %" PRIu64 "\n", totalsize);
    if(mq_unlink((const char *) QUEUE_NAME) < 0)
        handle_error("Failed to unlink Posix queue");
    exit(EXIT_SUCCESS);
}