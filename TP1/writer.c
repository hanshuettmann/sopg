#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>

#define FIFO_NAME "myfifo_tp1"
#define BUFFER_SIZE 300

const char *SIGNUSR1_MSG = "SIGN:1";
const char *SIGNUSR2_MSG = "SIGN:2";

static char outputBuffer[BUFFER_SIZE];
static char outputBufferAux[BUFFER_SIZE];

static uint32_t bytesWrote;

static int32_t returnCode, fd;

void sigusr1Handler(int sig)
{
    /* Write buffer to named fifo from SIGUSR. Strlen - 1 to avoid sending \n char */
    if ((bytesWrote = write(fd, SIGNUSR1_MSG, strlen(SIGNUSR1_MSG))) == -1)
    {
        perror("SIGUSR1 error");
    }
    else
    {
        printf("SIGUSR1 received: %s --> wrote %d bytes in fifo\n", SIGNUSR1_MSG, bytesWrote);
    }
}

void sigusr2Handler(int sig)
{
    /* Write buffer to named fifo. Strlen - 1 to avoid sending \n char */
    if ((bytesWrote = write(fd, SIGNUSR2_MSG, strlen(SIGNUSR2_MSG))) == -1)
    {
        perror("SIGUSR2 error");
    }
    else
    {
        printf("SIGUSR2 received: %s --> wrote %d bytes in fifo\n", SIGNUSR2_MSG, bytesWrote);
    }
}

int main(void)
{
    /* Create SIGUSR1 handler */
    struct sigaction sa1;

    sa1.sa_handler = sigusr1Handler;
    sa1.sa_flags = 0; // SA_RESTART;
    sigemptyset(&sa1.sa_mask);

    sigaction(SIGUSR1, &sa1, NULL);

    /* Create SIGUSR2 handler */
    struct sigaction sa2;

    sa2.sa_handler = sigusr2Handler;
    sa2.sa_flags = 0; // SA_RESTART;
    sigemptyset(&sa2.sa_mask);

    sigaction(SIGUSR2, &sa2, NULL);

    /* Create named fifo. -1 means already exists so no action if already exists */
    if ((returnCode = mknod(FIFO_NAME, S_IFIFO | 0666, 0)) < -1)
    {
        printf("Error creating named fifo: %d\n", returnCode);
        exit(1);
    }

    /* Open named fifo. Blocks until other process opens it */
    printf("wait reader...\n");
    if ((fd = open(FIFO_NAME, O_WRONLY)) < 0)
    {
        printf("Error opening named fifo file: %d\n", fd);
        exit(1);
    }

    /* open syscalls returned without error -> other process attached to named fifo */
    printf("got a reader --> continue process\n");

    /* Loop forever */
    while (1)
    {
        /* Get some text from console */
        fgets(outputBuffer, BUFFER_SIZE, stdin);

        if (strlen(outputBuffer) > 0)
        {
            strncpy(outputBufferAux,"DATA:", sizeof(outputBufferAux) - 1); //Third parameter is always the dest
            strncat(outputBufferAux, outputBuffer, sizeof(outputBufferAux) - 1);

            /* Write buffer to named fifo. Strlen - 1 to avoid sending \n char */
            if ((bytesWrote = write(fd, outputBufferAux, strlen(outputBufferAux) - 1)) == -1)
            {
                perror("writer error");
            }
            else
            {
                printf("writer: wrote %d bytes\n", bytesWrote);
                /* Clean buffer */
                memset(outputBufferAux, 0, BUFFER_SIZE);
                memset(outputBuffer, 0, BUFFER_SIZE);
            }
        }
    }
    return 0;
}
