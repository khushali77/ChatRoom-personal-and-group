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

static int uid = 10;

typedef struct
{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
} client_t;

static _Atomic unsigned int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
client_t *clients[100] = {};

void queue_add(client_t *cl)
{
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < 100; ++i)
	{
		if (!clients[i])
		{
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void queue_remove(int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < 100; ++i)
	{
		if (clients[i])
		{
			if (clients[i]->uid == uid)
			{
				clients[i] = NULL;
				break;
			}
		}
	}
	pthread_mutex_unlock(&clients_mutex);
}

// Handling all the communication with the client
void send_message_to_other_clients(char *s, int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < 100; ++i)
	{
		if (clients[i])
		{
			//printf("%d\n", clients[i]->uid);
			if (clients[i]->uid != uid)
			{
				if (write(clients[i]->sockfd, s, strlen(s)) < 0)
				{
					perror("ERROR : writing into descriptor failed\n");
					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&clients_mutex);
}

void send_message_to_next_client(char *s, int uid)
{
	int i;
	pthread_mutex_lock(&clients_mutex);
	
	for(i=0;i<100;++i)
	{
		if(clients[i])
		{
			if(clients[i]->uid==uid)
			{
				break;
			}
		}
	}
	
	printf("%d",clients[i]->uid);
	
	if(write(clients[(i+1)%client_count]->sockfd,s,strlen(s))<0)
	{
			perror("ERROR : writing into descriptor failed\n");
	}
	
	
	pthread_mutex_unlock(&clients_mutex);
}

void *handle_client_oo(void *arg)
{
	char buff_out[2048];
	int leave = 0;
	char name[32];
	char ret[2048];
	client_count++;
	client_t *cli = (client_t *)arg;

	int reads = recv(cli->sockfd, name, 32, 0);
	sprintf(ret, "Server : I recieved your name!\n");
	write(cli->sockfd, ret, strlen(ret));

	if (reads <= 0 || strlen(name) < 2 || strlen(name) >= 31)
	{
		printf("Client didn't enter the name.\n");
		leave = 1;
	}
	else
	{
		strcpy(cli->name, name);
		//Acknowledgment code//
		//send_message_to_next_client(buff_out,cli->uid);
	}
	//bzero(buff_out,2048);
	bzero(ret, 2048);

	//Message recieved from the client
	while (1)
	{
		if (leave)
			break;
		else
		{
			int recvbytes = recv(cli->sockfd, buff_out, 2048, 0);
			printf("recvbytes : %d\n", recvbytes);
			if (recvbytes > 0)
			{
				if (strlen(buff_out) > 0)
				{
					printf("Length of buff_out is %d\n", (int)strlen(buff_out));
					sprintf(ret, "Server : I got your message safely\n");
					printf("1\n");
					send_message_to_next_client(buff_out, cli->uid);
					printf("2\n");
					write(cli->sockfd, ret, strlen(ret));
				}
			}
			else if (recvbytes == 0 || strcmp(buff_out, "exit") == 0)
			{
				sprintf(buff_out, "%s ha left\n", cli->name);
				printf("%s\n", buff_out);
				send_message_to_next_client(buff_out, cli->uid);
				leave = 1;
			}
			else
			{
				printf("ERROR : -1\n");
				leave = 1;
			}
		}
		bzero(buff_out, 2048);
		bzero(ret, 2048);
	}
	close(cli->sockfd);
	queue_remove(cli->uid);
	free(cli);
	client_count--;
	pthread_detach(pthread_self());
	return NULL;
}

void *handle_client_bc(void *arg)
{
	char buff_out[2048];
	int leave = 0;
	char name[32];
	char ret[2048];
	client_count++;
	client_t *cli = (client_t *)arg;

	int reads = recv(cli->sockfd, name, 32, 0);
	sprintf(ret, "Server : I recieved your name!\n");
	write(cli->sockfd, ret, strlen(ret));

	if (reads <= 0 || strlen(name) < 2 || strlen(name) >= 31)
	{
		printf("Client didn't enter the name.\n");
		leave = 1;
	}
	else
	{
		strcpy(cli->name, name);
		//Acknowledgment code//
		//send_message_to_other_clients(buff_out,cli->uid);
	}

	bzero(buff_out, 2048);
	bzero(ret, 2048);

	//Message recieved from the client
	while (1)
	{
		if (leave)
			break;
		else
		{
			int recvbytes = recv(cli->sockfd, buff_out, 2048, 0);

			if (recvbytes > 0)
			{
				if (strlen(buff_out) > 0)
				{
					sprintf(ret, "Server : I got your message safely\n");
					send_message_to_other_clients(buff_out, cli->uid);
					write(cli->sockfd, ret, strlen(ret));
					//printf("No. of active clients : %d",client_count);
				}
			}
			else if (recvbytes == 0 || strcmp(buff_out, "exit") == 0)
			{
				sprintf(buff_out, "%s has left\n", cli->name);
				printf("%s", buff_out);
				send_message_to_other_clients(buff_out, cli->uid);
				;
				leave = 1;
			}
			else
			{
				printf("ERROR : -1\n");
				leave = 1;
			}
		}
		bzero(buff_out, 2048);
		bzero(ret, 2048);
	}
	close(cli->sockfd);
	queue_remove(cli->uid);
	free(cli);
	client_count--;
	pthread_detach(pthread_self());
	return NULL;
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("Invalid.Enter the port number please!\n");
		return EXIT_FAILURE;
	}

	int w;
	int sockfd, newsockfd;
	pthread_t tid;
	struct sockaddr_in serv_addr, client_addr;
	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);

	//Choose your option
	printf("1] One to One Mode\n2] Broadcast Mode\nChoose your Option : \n");
	scanf("%d", &w);

	//Create Socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		printf("[-]ERROR creating socket\n");
		return EXIT_FAILURE;
	}
	printf("[+]SUCCESS : SOCKET CREATED\n");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);

	//Binding Socket
	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("[-]BIND ERROR / ERROR on binding\n");
		return EXIT_FAILURE;
	}
	printf("[+]SUCCESS : BINDED\n");

	//Waiting for the client
	if (listen(sockfd, 50) < 0)
	{
		printf("[-]ERROR on listening\n");
		return EXIT_FAILURE;
	}
	printf("[+]Listening....\n");

	if (w == 1)
	{
		//One to One mode
		printf("-----ONE TO ONE MODE-----\n");
		while (1)
		{

			socklen_t clilen = sizeof(client_addr);
			newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &clilen);

			if (newsockfd < 0)
			{
				printf("[-]ERROR accepting connection request\n");
				return EXIT_FAILURE;
			}
			printf("[+]SUCCESS : Connection Accepted\n");

			client_t *cli = (client_t *)malloc(sizeof(client_t));
			cli->address = client_addr;
			cli->sockfd = newsockfd;
			cli->uid = uid++;

			queue_add(cli);
			pthread_create(&tid, NULL, &handle_client_oo, (void *)cli);

			sleep(1);
		}
	}
	else if (w == 2)
	{
		//Broadcast mode
		printf("-----BROADCAST MODE-----\n");
		while (1)
		{
			socklen_t clilen = sizeof(client_addr);
			newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &clilen);

			if (newsockfd < 0)
			{
				printf("[-]ERROR accepting connection request\n");
				return EXIT_FAILURE;
			}
			printf("[+]SUCCESS : Connection Accepted\n");

			client_t *cli = (client_t *)malloc(sizeof(client_t));
			cli->address = client_addr;
			cli->sockfd = newsockfd;
			cli->uid = uid++;

			queue_add(cli);
			pthread_create(&tid, NULL, &handle_client_bc, (void *)cli);

			sleep(1);
		}
	}
	return EXIT_SUCCESS;
}