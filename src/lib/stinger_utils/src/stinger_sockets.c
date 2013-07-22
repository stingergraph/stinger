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

