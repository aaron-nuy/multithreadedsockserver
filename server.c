#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Global variables
int client_socket[MAX_CLIENTS];
int client_count = 0;

// Function prototypes
void *connection_handler(void *);

int main(int argc, char *argv[]) {
    int server_socket, port_number;
    struct sockaddr_in server_address;
    int opt = 1;
    int addrlen = sizeof(server_address);
    pthread_t thread_id;

    // Check for command line arguments
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    // Set socket options
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(1);
    }

    // Set server address
    port_number = atoi(argv[1]);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port_number);

    // Bind socket to address
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    // Listen for connections
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(1);
    }

    // Accept connections
    while (1) {
        int new_socket = accept(server_socket, (struct sockaddr *)&server_address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("accept");
            exit(1);
        }

        // Add new socket to list of sockets
        client_socket[client_count] = new_socket;
        client_count++;

        // Create thread for new connection
        if (pthread_create(&thread_id, NULL, connection_handler, (void*)&new_socket) < 0) {
            perror("could not create thread");
            exit(1);
        }
    }

    return 0;
}

// Thread function for handling connections
void *connection_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    int read_size;
    char client_message[BUFFER_SIZE];

    // Receive messages from client
    while ((read_size = recv(sock, client_message, BUFFER_SIZE, 0)) > 0) {
        // Send message to all other clients
        for (int i = 0; i < client_count; i++) {
            if (client_socket[i] != sock) {
                write(client_socket[i], client_message, strlen(client_message));
            }
        }
    }

    // Close socket
    if (read_size == 0) {
        printf("Client disconnected\n");
        fflush(stdout);
    } else if (read_size == -1) {
        perror("recv failed");
    }

    return 0;
}
