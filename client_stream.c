#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#define LENGTH 2048

volatile sig_atomic_t flag = 0;
char name[32] = {};
int sockfd = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void send_msg_handler()
{
	char message[LENGTH] = {};
	char buffer[LENGTH + 32] = {};

	while (1)
	{
		fgets(message, 1024, stdin);
		if (strlen(message) <= 1)
			continue;

		if (message[0] == 'e' && message[1] == 'x')
			break;
		else
		{
			sprintf(buffer, "%s : %s\n", name, message);
			send(sockfd, buffer, strlen(buffer), 0);
		}

		bzero(message, 2048);
		bzero(buffer, 2048 + 32);		
	}
}

void recv_msg_handler()
{

	char message[2048] = {};
	while (1)
	{
		int recvbytes = recv(sockfd, message, LENGTH, 0);
		if (recvbytes > 0)
		{
			printf("%s", message);
		}
		else if (recvbytes == 0)
		{
			break;
		}
		memset(message, 0, sizeof(message));
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("Enter the port number, please.\n");
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);

	printf("Please enter your name : \n");
	fgets(name,32,stdin);
	printf("Now you can send messages! \n DROP YOUR MESSAGES BELOW!\n");
	//printf("Name entered!\n");
	struct sockaddr_in serv_addr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);

	int err = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (err == -1)
	{
		printf("ERROR in connecting with Server\n");
		return EXIT_FAILURE;
	}

	printf("[+]SUCCESS : CONNECTED\n");

	send(sockfd, name, 32, 0);
	//printf("[+]Name sent successfully\n");

	pthread_t send_msg_thread, recv_msg_thread;

	if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, NULL) != 0)
	{
		printf("ERROR in pthread\n");
		return EXIT_FAILURE;
	}

	//printf("Thread Created!");

	if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, NULL) != 0)
	{
		printf("ERROR in pthread\n");
		return EXIT_FAILURE;
	}
	pthread_join(send_msg_thread, NULL);
	pthread_join(recv_msg_thread, NULL);

	close(sockfd);
	return EXIT_SUCCESS;
}