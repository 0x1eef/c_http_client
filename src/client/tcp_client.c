#include "tcp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define __BUFFER_SIZE 65536

/**
 * @brief Sends an HTTP request to a specified server and port using a TCP socket.
 *
 * @param request The HTTPRequest structure containing the request information.
 * @param host The server's IP address or hostname.
 * @param port The port number on which the server is listening.
 * @return An HTTPResponse structure containing the parsed response, or NULL on error.
 */
HTTPResponse *shttp(HTTPRequest *request, char *host, unsigned int port) {
  int sock;
  struct addrinfo hints, *result, *rp;
  char buffer[__BUFFER_SIZE];
  char *parsed_request;
  int read_size;
  HTTPResponse *response = NULL;

  // Initialize buffer with null characters
  memset(buffer, '\0', __BUFFER_SIZE);

  // Set up hints structure for getaddrinfo
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // Use IPv4
  hints.ai_socktype = SOCK_STREAM;

  // Resolve the server's IP address or hostname
  if (getaddrinfo(host, NULL, &hints, &result) != 0) {
    return NULL;
  }

  // Iterate over the available addresses and connect to the first successful one
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    // Create a TCP socket
    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock == -1) {
      continue;
    }

    // Set up server address structure
    struct sockaddr_in *server_addr = (struct sockaddr_in *)rp->ai_addr;
    server_addr->sin_port = htons(port);

    // Parse the HTTP request and store it in buffer
    parsed_request = parse_request(request);
    strcpy(buffer, parsed_request);
    free(parsed_request);

    // Connect to the server
    if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1) {
      break; // Success
    }

    close(sock);
  }

  // Free the address information structure
  freeaddrinfo(result);

  // Check if a successful connection was established
  if (rp == NULL) {
    return NULL;
  }

  // Send the HTTP request to the server
  if (send(sock, buffer, strlen(buffer), 0) < 0) {
    close(sock);
    return NULL;
  }

  // Receive the HTTP response from the server
  read_size = recv(sock, buffer, __BUFFER_SIZE - 1, 0);
  if (read_size < 0) {
    close(sock);
    return NULL;
  }

  // Parse the HTTP response and store it in the response structure
  response = parse_response(buffer);

  // Close the socket
  close(sock);
  return response;
}
