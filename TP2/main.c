#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pthread.h>
#include <signal.h>
#include "SerialManager.h"

#define SERIAL_BUFFER_SIZE 64
#define SERIAL_PORT 4040
#define SERIAL_BAUDRATE 115200

static pthread_t threadSerial;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void blockSignal(void);
void unblockSignal(void);

int newfd;

// Client connection status
bool socketConnected;
bool serverConnected;

void *readSerial(void *message)
{
	char rxBuffer[SERIAL_BUFFER_SIZE];

	while (1)
	{
		if (serial_receive(rxBuffer, SERIAL_BUFFER_SIZE) > 0)
		{
			// Print Message
			printf("%s", rxBuffer);

			// Send to socket
			pthread_mutex_lock(&mutex);
			if (socketConnected)
			{
				// Send message to client
				if (write(newfd, rxBuffer, strlen(rxBuffer)) == -1)
				{
					perror("Failed receiving message in socket");
				}
			}
			pthread_mutex_unlock(&mutex);

			// Add 10ms delay to avoid CPU 100%
			usleep(10000);
		}
	}

	return NULL;
}

void sigint_handler(int sig)
{
	// close server
	serverConnected = false;
}

int main(void)
{

	// Set signal handlers
	// Signal ctrl+c
	struct sigaction sa1;
	sa1.sa_handler = sigint_handler;
	sa1.sa_flags = 0;
	sigemptyset(&sa1.sa_mask);
	if (sigaction(SIGINT, &sa1, NULL) == -1)
	{
		perror("sigaction sigint");
		exit(1);
	}

	// Signal sigterm
	struct sigaction sa2;
	sa2.sa_handler = sigint_handler;
	sa2.sa_flags = 0;
	sigemptyset(&sa2.sa_mask);
	if (sigaction(SIGTERM, &sa2, NULL) == -1)
	{
		perror("sigaction sigterm");
		exit(1);
	}

	printf("Init serial port\r\n");

	// Open Serial Port
	if (serial_open(SERIAL_PORT, SERIAL_BAUDRATE))
	{
		perror("Error initializing serial port...\r\n");
		return 0;
	}

	// Block signals
	blockSignal();

	// Create threadSerial
	pthread_create(&threadSerial, NULL, readSerial, NULL);

	// Unblock signals
	unblockSignal();

	// Create TCP Server
	socklen_t addr_len;
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;
	char buffer[128];
	int n;

	// Socket fd's
	int s = socket(AF_INET, SOCK_STREAM, 0);

	serverConnected = true;
	bool isClientAvailable;

	// Load IP data server IP:PORT
	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(10000);
	// serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (inet_pton(AF_INET, "127.0.0.1", &(serveraddr.sin_addr)) <= 0)
	{
		fprintf(stderr, "ERROR invalid server IP\r\n");
		return 1;
	}

	// Open port with bind()
	if (bind(s, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
	{
		close(s);
		perror("listener: bind");
		return 1;
	}

	// Socket in Listening mode
	if (listen(s, 10) == -1) // backlog=10
	{
		perror("Error in listen function");
		exit(1);
	}

	while (serverConnected)
	{
		// Call accept() to receive new connections
		addr_len = sizeof(struct sockaddr_in);
		if ((newfd = accept(s, (struct sockaddr *)&clientaddr, &addr_len)) == -1)
		{
			perror("Error in accept function");
			serverConnected = false;
			break;
		}

		char ipClient[32];
		inet_ntop(AF_INET, &(clientaddr.sin_addr), ipClient, sizeof(ipClient));
		printf("server:  connection from:  %s\n", ipClient);

		pthread_mutex_lock(&mutex);
		socketConnected = true;
		isClientAvailable = socketConnected;
		pthread_mutex_unlock(&mutex);

		while (isClientAvailable && serverConnected)
		{
			// Read message from client
			// if( (n = recv(newfd,buffer,128,0)) == -1 )
			if ((n = read(newfd, buffer, 128)) == -1)
			{
				perror("Error reading message in socket");
			}

			if (n <= 0)
			{
				// Update Client connection status
				pthread_mutex_lock(&mutex);
				socketConnected = false;
				isClientAvailable = socketConnected;
				pthread_mutex_unlock(&mutex);
			}
			else
			{
				buffer[n] = 0x00;
				printf("Received %d bytes.:%s\n", n, buffer);

				// Send message to serial port
				serial_send(buffer, strlen(buffer));
			}
		}

		// Close connection with client
		close(newfd);

	} // fin while

	// Close client and socket
	printf("Closing socket...\n");
	close(s);

	// Cancel Serial Thread
	pthread_cancel(threadSerial);

	// Wait thread to be closed
	void *ret;
	pthread_join(threadSerial, &ret);
	if (ret == PTHREAD_CANCELED)
		printf("Serial Thread Cancelled\n");
	else
		printf("Serial Thread Ended\n");

	// Close serial port
	printf("Closing serial port...\n");
	serial_close();

	printf("Programm finished\n");

	return 0;
}

void blockSignal(void)
{
	sigset_t set;
	int s;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}

void unblockSignal(void)
{
	sigset_t set;
	int s;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}
