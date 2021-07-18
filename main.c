#include <netinet/in.h> // TCP/IP related functiosn and constants (e.g. PF_INET)
#include <sys/socket.h> // Socket related functions (e.g. socket(3))
#include <stdio.h>      // I/O related functionality (e.g. perror(1))
#include <arpa/inet.h>  // Helper for conversions (e.g. htons(1))
#include <unistd.h>     // Miscellaneous functions (e.g. close(1))
#include <string.h>     // String helper functions (e.g. strcmp(2))
#include <stdbool.h>    // For true and false constants

#define PORT 8080
#define WEB_ROOT "web"
#define MAX_CONNECTIONS 1000

// Forward declarations of functions.
int tb_listen_and_serve(unsigned short int port);

int main(int argv, char* argc[]) {
  return tb_listen_and_serve(PORT);
}

int tb_accept_client(int socket_fd, struct sockaddr_in server_address) {
  struct sockaddr_in client_address;
  socklen_t client_address_length = sizeof(server_address);

  int client_socket_fd = accept(socket_fd, (struct sockaddr*)&client_address, &client_address_length);
  if (client_socket_fd == -1) {
    perror("failed to create client socket");
    return -1;
  }
  printf("Client connected with address '%s'\n", inet_ntoa(client_address.sin_addr));

  return client_socket_fd;
}

typedef struct Request {
  char method[8];
  char url[256];
} Request;

int tb_parse_request(Request* restrict request, FILE* restrict client_file) {
  // Read from client connection.
  int line_length = 256;
  char line[line_length];
  char method[8];
  char url[256];

  // Parse HTTP request.
  if (fgets(line, line_length, client_file) == NULL) {
    perror("failed to parse empty request");
    return -1;
  }

  if (sscanf(line, "%7s %255s", method, url) != 2) {
    perror("failed to parse invalid request");
    return -1;
  }

  strcpy(request->method, method);
  strcpy(request->url, url);

  while (fgets(line, line_length, client_file) != NULL) {
    //printf("Client sent: %s", line);

    if (strcmp(line, "\r\n") == 0) {
      break;
    }
  }

  return 0;
}

int tb_copy_file_content( FILE* restrict source, FILE* restrict destination) {
  int line_length = 256;
  char line[line_length];
  while (fgets(line, line_length, source) != NULL) {
    if (fputs(line, source) < 0) {
      perror("failed to write to destination file");
      return -1;
    }
  }

  return 0;
}

int tb_serve_file(FILE* restrict client_file, char path[]) {
  char file_path[256];
  strncat(file_path, WEB_ROOT, sizeof(WEB_ROOT));
  strncat(file_path, path, 100);

  FILE* source_file = fopen(file_path, "r");
  if (source_file == NULL) {
    perror("failed to open source file");
    return -1;
  }

  // TODO: Generate response header and prepend.

  if (tb_copy_file_content(source_file, client_file) != 0) {
    return -1;
  }

  return 0;
}

int tb_handle_client(int client_socket_fd) {
  // Open client socket file descriptor.
  FILE* client_file = fdopen(client_socket_fd, "r");
  if (client_file == NULL) {
    perror("failed to open client socket file descriptor");
    return -1;
  }

  Request request = {};
  if (tb_parse_request(&request, client_file) != 0) {
    if (fclose(client_file) != 0) {
      perror("failed to close the client file descriptor");
    }
    return -1;
  }
  printf("Request method: %s Request URL: %s\n", request.method, request.url);

  char response[] = "HTTP/1.1 200 OK\r\n\
Date: Mon, 27 Jul 2009 12:28:53 GMT\r\n\
Server: TinyBlog\r\n\
Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT\r\n\
Content-Length: 56\r\n\
Content-Type: text/html\r\n\
Connection: Closed\r\n\r\n\
<html>\r\n\
<body>\r\n\
<h1>Hello, World!</h1>\r\n\
</body>\r\n\
</html>";

  if (send(client_socket_fd, response, sizeof(response) / sizeof(char), 0) == -1) {
    perror("failed to send response");
    return -1;
  }

  // Close the client file (also closes the socket).
  if (fclose(client_file) != 0) {
    perror("failed to close the client file descriptor");
    return -1;
  }

  return 0;
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

  while (true) {
    // Wait for incomming client connections.
    int client_socket_fd = tb_accept_client(socket_fd, server_address);

    if (tb_handle_client(client_socket_fd) != 0) {
      perror("failed to handle client");
      if (close(client_socket_fd) != 0) {
        perror("failed to close client socket");
      }
      continue;
    }
  }

  // Close the server socket.
  if (close(socket_fd) != 0) {
    perror("failed to close the server socket");
    return -1;
  }

  return 0;
}
