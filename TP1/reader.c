#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#define FIFO_NAME "myfifo_tp1"
#define BUFFER_SIZE 300

#define DATA_PREFIX "DATA"
#define SIGNAL_PREFIX "SIGN"

int main(void)
{
    uint8_t inputBuffer[BUFFER_SIZE];
    int32_t bytesRead, returnCode, fd;

    /* Create named fifo. -1 means already exists so no action if already exists */
    if ((returnCode = mknod(FIFO_NAME, S_IFIFO | 0666, 0)) < -1)
    {
        printf("Error creating named fifo: %d\n", returnCode);
        exit(1);
    }

    /* Init log.txt file pointer */
    FILE *pLogFile = NULL;
    char *logFileName = "Log.txt";

    /* Create and open sign.txt file */
    FILE *pSignFile = NULL;
    char *signFileName = "Sign.txt";

    pSignFile = fopen(signFileName, "w+");
    if (pSignFile == NULL)
        printf("Failed to open %s.\n", signFileName);

    /* Open named fifo. Blocks until other process opens it */
    printf("waiting for writers...\n");
    if ((fd = open(FIFO_NAME, O_RDONLY)) < 0)
    {
        printf("Error opening named fifo file: %d\n", fd);
        exit(1);
    }

    /* open syscalls returned without error -> other process attached to named fifo */
    printf("got a writer\n");

    /* Loop until read syscall returns a value <= 0 */
    do
    {
        /* read data into local buffer */
        if ((bytesRead = read(fd, inputBuffer, BUFFER_SIZE)) == -1)
        {
            perror("read");
        }
        else
        {
            inputBuffer[bytesRead] = '\0';
            printf("reader: read %d bytes: \"%s\"\n", bytesRead, inputBuffer);

            if (strncmp(inputBuffer, DATA_PREFIX, strlen(DATA_PREFIX)) == 0)
            {
                pLogFile = fopen(logFileName, "a");
                if (pLogFile == NULL)
                {
                    printf("Failed to open %s.\n", logFileName);
                }
                else
                {
                    /* Write log file */
                    fputs(inputBuffer, pLogFile);

                    /* Close file */
                    fclose(pLogFile);
                }
            }

            if (strncmp(inputBuffer, SIGNAL_PREFIX, strlen(SIGNAL_PREFIX)) == 0)
            {
                pSignFile = fopen(signFileName, "a");
                if (pSignFile == NULL)
                {
                    printf("Failed to open %s.\n", signFileName);
                }
                else
                {
                    /* Write log file */
                    fputs(inputBuffer, pSignFile);

                    /* Close file */
                    fclose(pSignFile);
                }
            }
        }
    } while (bytesRead > 0);

    return 0;
}
