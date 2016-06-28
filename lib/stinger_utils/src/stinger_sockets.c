#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "stinger_core/stinger_error.h"

int
connect_to_batch_server (const char * hostname, int port)
{
  /* Look up the hostname */
  struct addrinfo hints;
  struct addrinfo *server_addr;
  char port_string[128];
  snprintf(port_string, 128, "%i", port);
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* Stream socket */
  hints.ai_flags = 0;
  hints.ai_protocol = 0;           /* Any protocol */
  int s = getaddrinfo(hostname, port_string, &hints, &server_addr);

  if (s != 0) {
    LOG_E_A("ERROR: server %s could not be resolved.", hostname);
    LOG_E_A("getaddrinfo: %s", gai_strerror(s));
    exit(-1);
  }

  /* start the connection */
  int sock_handle, n;

  if (-1 == (sock_handle = socket(server_addr->ai_family, server_addr->ai_socktype, 0))) {
    perror("Socket create failed");
    return -1;
  }

  if(-1 == connect(sock_handle, server_addr->ai_addr, server_addr->ai_addrlen)) {
    perror("Connection failed");
    return -1;
  }

  freeaddrinfo(server_addr);
  return sock_handle;
}

int
get_shared_map_info (char * hostname, int port, char ** name, size_t name_len, size_t * graph_sz)
{
  LOG_D("called...");

  int sock, n;
  char buffer[256];

  /* Look up the hostname */
  struct addrinfo hints;
  struct addrinfo *server_addr;
  char port_string[128];
  snprintf(port_string, 128, "%i", port);
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM;  /* Datagram socket */
  hints.ai_flags = 0;
  hints.ai_protocol = 0;           /* Any protocol */
  int s = getaddrinfo(hostname, port_string, &hints, &server_addr);

  if (s != 0) {
    LOG_E_A("ERROR: server %s could not be resolved.", hostname);
    LOG_E_A("getaddrinfo: %s", gai_strerror(s));
    exit(-1);
  }

  /* Create the socket */
  sock = socket(server_addr->ai_family, server_addr->ai_socktype, 0);
  if (sock < 0) {
    LOG_F("Failed to create socket");
    exit(-1);
  }

  /* Set 5 second timeout on UDP socket */
  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 0;
  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    LOG_F("Error");
    exit(-1);
  }

  bzero(buffer, 256);

  /* Send "NAME" to server */
  sprintf(buffer, "NAME");
  n = sendto(sock, buffer, strlen(buffer), 0, server_addr->ai_addr, server_addr->ai_addrlen);
  if (n < 0) {
    LOG_F("Unable to send");
    exit(-1);
  }

  /* Receive back the string containing the name of the shared map */
  n = recvfrom (sock, buffer, 256, 0, NULL, NULL);
  if (n < 0) {
    LOG_F("Unable to receive");
    exit(-1);
  }
  char * found = strchr(buffer, ':');
  strncpy (name, found+1, name_len);

  /* Send "SIZE" to server */
  sprintf(buffer, "SIZE");
  n = sendto(sock, buffer, strlen(buffer), 0, server_addr->ai_addr, server_addr->ai_addrlen);
  if (n < 0) {
    LOG_F("Unable to send");
    exit(-1);
  }

  /* Receive the size of the map */
  n = recvfrom (sock, buffer, 256, 0, NULL, NULL);
  if (n < 0) {
    LOG_F("Unable to receive");
    exit(-1);
  }
  found = strchr(buffer, ':');
  *graph_sz = atol(found+1);

  close(sock);
  freeaddrinfo(server_addr);
  return 0;
}

