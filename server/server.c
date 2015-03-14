#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <errno.h>
#include <protocol.h>

const int NUM_OF_CONNECTIONS = 5;

void create_socket(int* socket_var) {
	*socket_var = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_var < 0) {
		perror("Error: couldn't create a socket\n");
		exit(1);
	}
}

void init_addr(struct sockaddr_in * addr, int port_number) {
	bzero((char*) addr, sizeof(struct sockaddr));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port_number);
	addr->sin_addr.s_addr = INADDR_ANY;
}

void assign_a_name(int socket_var, struct sockaddr_in* addr) {
	if(bind(socket_var, (struct sockaddr*) addr, sizeof(struct sockaddr)) < 0) {
		perror("Error: assigning a name to the socket failed\n");
 		exit(1);
	}
}

void accept_connection(int* new_socket, int socket_var, struct sockaddr * addr, socklen_t* client_len) {
	*client_len = sizeof(struct sockaddr);
	*new_socket = accept(socket_var, addr, client_len);
	if(*new_socket < 0) {
		perror("Error: could not accept data\n");
		exit(1);
	}
}

void do_job(int new_socket, char buffer[], const size_t len) {
	ssize_t res;
	bzero(buffer, BUF_SIZE);
	res = recv(new_socket, buffer, len, 0);
	if(res < 0) {
		perror("Error: failed to recieve data\n");
		exit(1);
	}
	printf("Recieved message is:\n %s\n", buffer);
	res = send(new_socket, buffer, res, 0);
	if(res < 0) {
		perror("Error: failed to send data\n");
		exit(1);	
	}
	printf("The message was sent back\n");
}

int main(int argc, char* argv[]) {
	int socket_var, new_socket;
	char buffer[BUF_SIZE];
	socklen_t client_len;
	struct sockaddr_in server_addr, client_addr;
	
	create_socket(&socket_var);
	init_addr(&server_addr, PORT_NUMBER);
	assign_a_name(socket_var, &server_addr);
	if(listen(socket_var, NUM_OF_CONNECTIONS) < 0) {
		perror("Error: listening failed\n");
		exit(1);
	}
	accept_connection(&new_socket, socket_var, (struct sockaddr *) &client_addr, &client_len);
	do_job(new_socket, buffer, BUF_SIZE);
	close(new_socket);
	close(socket_var);
	
	return 0;
}
