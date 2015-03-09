#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

const size_t BUF_SIZE = 100;

void create_socket(int* socket_var) {
	*socket_var = socket(AF_UNIX, SOCK_STREAM, 0);
	if(socket_var < 0) {
		fprintf(stderr, "Error: couldn't create a socket\n");
		exit(1);
	}
}

void init_addr(struct sockaddr_un * addr, const char* path) {
	bzero((char*) addr, sizeof(addr));
	addr->sun_family = AF_UNIX;
	strcpy(addr->sun_path, path);
}

void connect_to_server(int socket_var, struct sockaddr_un * server_addr) {
	socklen_t len = strlen(server_addr->sun_path) + sizeof(server_addr->sun_family);
	if(connect(socket_var, (struct sockaddr *) server_addr, len) < 0) {
		fprintf(stderr, "Error: connection to server failed\n");
		exit(1);
	}
}

void do_job(int socket_var, char message[], char buffer[], const size_t len) {
	ssize_t res;
	bzero(buffer, BUF_SIZE);
	res = send(socket_var, message, len, 0);
	if(res < 0) {
		fprintf(stderr, "Error: failed to send a message\n");
		exit(1);
	}
	printf("Sent message:\n %s\n", message);
	res = recv(socket_var, buffer, res, 0);
	if(res < 0) {
		fprintf(stderr, "Error: failed to recieve a message\n");
		exit(1);	
	}
	printf("Recieved an answer:\n %s\n", buffer);
}

int main(int argc, char* argv[]) {
	int socket_var;
	char buffer[BUF_SIZE];
	char message[] = "Reply 'POTATO' (6 letters)";
	socklen_t client_len;
	struct sockaddr_un server_addr;
	
	if(argc < 2) {
		fprintf(stderr, "Error: not enough arguments\n");
		exit(1);
	}
	
	create_socket(&socket_var);
	init_addr(&server_addr, argv[1]);
	connect_to_server(socket_var, &server_addr);
	do_job(socket_var, message, buffer, BUF_SIZE);
	close(socket_var);
	
	return 0;
}
