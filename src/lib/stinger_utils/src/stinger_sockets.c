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
connect_to_batch_server (struct hostent * server, int port)
{
  /* start the connection */
  int sock_handle, n;
  struct sockaddr_in serv_addr;

  if (-1 == (sock_handle = socket(AF_INET, SOCK_STREAM, 0))) {
    perror("Socket create failed");
    return -1;
  }

  bzero ((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy ((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(port);

  if(-1 == connect(sock_handle, (const struct sockaddr_in *) &serv_addr, sizeof(serv_addr))) {
    perror("Connection failed");
    return -1;
  }

  return sock_handle;
}

int
get_shared_map_info (char * hostname, int port, char ** name, size_t name_len, size_t * graph_sz)
{
  LOG_D("called...");

  int sock, n;
  unsigned int length;
  struct sockaddr_in server, from;
  struct hostent *hp;
  char buffer[256];

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    LOG_F("Failed to create socket");
    exit(-1);
  }

  server.sin_family = AF_INET;
  hp = gethostbyname(hostname);
  if (hp == 0) {
    LOG_F_A("Failed to find hostname %s", hostname);
    exit(-1);
  }

  bcopy ((char *) hp->h_addr, (char *) &server.sin_addr, hp->h_length);
  server.sin_port = htons(port);
  length = sizeof(struct sockaddr_in);

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
  n = sendto(sock, buffer, strlen(buffer), 0, (const struct sockaddr *) &server, length);
  if (n < 0) {
    LOG_F("Unable to send");
    exit(-1);
  }

  /* Receive back the string containing the name of the shared map */
  n = recvfrom (sock, buffer, 256, 0, (struct sockaddr *) &from, &length);
  if (n < 0) {
    LOG_F("Unable to receive");
    exit(-1);
  }
  char * found = strchr(buffer, ':');
  strncpy (name, found+1, name_len);

  /* Send "SIZE" to server */
  sprintf(buffer, "SIZE");
  n = sendto(sock, buffer, strlen(buffer), 0, (const struct sockaddr *) &server, length);
  if (n < 0) {
    LOG_F("Unable to send");
    exit(-1);
  }

  /* Receive the size of the map */
  n = recvfrom (sock, buffer, 256, 0, (struct sockaddr *) &from, &length);
  if (n < 0) {
    LOG_F("Unable to receive");
    exit(-1);
  }
  found = strchr(buffer, ':');
  *graph_sz = atol(found+1);

  close(sock);
  return 0;
}

