#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

const int PORT_NUMBER = 500;
const int NUM_OF_CONNECTIONS = 5;
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

void assign_a_name(int socket_var, struct sockaddr_un * addr) {
	socklen_t len = strlen(addr->sun_path) + sizeof(addr->sun_family);
	unlink(addr->sun_path);
	if(bind(socket_var, (struct sockaddr *) addr, len) < 0) {
		fprintf(stderr, "%s\n", addr->sun_path);
		fprintf(stderr, "Error: assigning a name to the socket failed\n");
		exit(1);
	}
}

void accept_connection(int* new_socket, int socket_var, struct sockaddr * addr, socklen_t* client_len) {
	*client_len = sizeof(struct sockaddr);
	*new_socket = accept(socket_var, addr, client_len);
	if(*new_socket < 0) {
		fprintf(stderr, "Error: could not accept data\n");
		exit(1);
	}
}

void do_job(int new_socket, char buffer[], const size_t len) {
	ssize_t res;
	bzero(buffer, BUF_SIZE);
	res = recv(new_socket, buffer, len, 0);
	if(res < 0) {
		fprintf(stderr, "Error: failed to recieve data\n");
		exit(1);
	}
	printf("Recieved message is:\n %s\n", buffer);
	res = send(new_socket, buffer, res, 0);
	if(res < 0) {
		fprintf(stderr, "Error: failed to send data\n");
		exit(1);	
	}
	printf("The message was sent back\n");
}

int main(int argc, char* argv[]) {
	int socket_var, new_socket, port_number;
	char buffer[BUF_SIZE];
	socklen_t client_len;
	struct sockaddr_un server_addr, client_addr;
	
	if(argc < 3) {
		fprintf(stderr, "Error: not enough arguments\n");
		exit(1);
	}
	sscanf(argv[2], "%d", &port_number);
	
	create_socket(&socket_var);
	init_addr(&server_addr, argv[1]);
	assign_a_name(socket_var, &server_addr);
	if(listen(socket_var, NUM_OF_CONNECTIONS) < 0) {
		fprintf(stderr, "Error: listening failed\n");
		exit(1);
	}
	accept_connection(&new_socket, socket_var, (struct sockaddr *) &client_addr, &client_len);
	do_job(new_socket, buffer, BUF_SIZE);
	close(new_socket);
	close(socket_var);
	
	return 0;
}
