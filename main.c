#include <netinet/in.h> // TCP/IP related functiosn and constants (e.g. PF_INET)
#include <sys/socket.h> // Socket related functions (e.g. socket(3))
#include <stdio.h>      // I/O related functionality (e.g. perror(1))
#include <arpa/inet.h>  // Helper for conversions (e.g. htons(1))
#include <unistd.h>     // Miscellaneous functions (e.g. close(1))

#define PORT 8080
#define MAX_CONNECTIONS 1000

// Forward declarations of functions.
int tb_listen_and_serve(unsigned short int port);

int main(int argv, char* argc[]) {
  return tb_listen_and_serve(PORT);
}

int tb_listen_and_serve(unsigned short int port) {
  // Initialize a socket for the server to listen on.
  int socket_fd = socket(PF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    perror("failed to initialize the server socket");
    return -1;
  }

  // Configure server address struct.
  struct sockaddr_in server_address = {
    .sin_family = AF_INET,
    .sin_port = htons(port),
    .sin_addr.s_addr = INADDR_ANY,  // Bind to any IP address.
  };

  // Assign the configured address to the socket.
  // TODO: Document sockaddr casting
  if (bind(socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) != 0) {
    perror("failed to bind socket to address");
    return -1;
  }

  // Initialize connection queue for the socket
  if (listen(socket_fd, MAX_CONNECTIONS) != 0) {
    perror("failed to listen for new connections");
    return -1;
  }
  printf("Listening on port %d ...\n", PORT);

  // Wait for incomming client connections.
  struct sockaddr_in client_address;
  socklen_t client_address_length = sizeof(server_address);
  int client_socket_fd = accept(socket_fd, (struct sockaddr*)&client_address, &client_address_length);
  if (client_socket_fd == -1) {
    perror("failed to create client socket");
      return -1;
  }
  printf("Client connected with address '%s'\n", inet_ntoa(client_address.sin_addr));

  // Close the server socket.
  if (close(socket_fd) != 0) {
    perror("failed to close the server socket");
    return -1;
  }

  return 0;
}
