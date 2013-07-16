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

int
get_shared_map_info (char * hostname, int port, char ** name, size_t name_len, size_t * graph_sz)
{
  int sock, n;
  unsigned int length;
  struct sockaddr_in server, from;
  struct hostent *hp;
  char buffer[256];

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    error("Failed to create socket.\n");
    exit(-1);
  }

  server.sin_family = AF_INET;
  hp = gethostbyname(hostname);
  if (hp == 0) {
    error("Failed to find hostname %s\n", hostname);
    exit(-1);
  }

  bcopy ((char *) hp->h_addr, (char *) &server.sin_addr, hp->h_length);
  server.sin_port = htons(port);
  length = sizeof(struct sockaddr_in);

  bzero(buffer, 256);

  /* Send "NAME" to server */
  sprintf(buffer, "NAME");
  n = sendto(sock, buffer, strlen(buffer), 0, (const struct sockaddr *) &server, length);
  if (n < 0) {
    perror("Unable to send.\n");
    exit(-1);
  }

  /* Receive back the string containing the name of the shared map */
  n = recvfrom (sock, buffer, 256, 0, (struct sockaddr *) &from, &length);
  if (n < 0) {
    perror("Unable to receive.\n");
    exit(-1);
  }
  char * found = strchr(buffer, ':');
  strncpy (name, found+1, name_len);

  /* Send "SIZE" to server */
  sprintf(buffer, "SIZE");
  n = sendto(sock, buffer, strlen(buffer), 0, (const struct sockaddr *) &server, length);
  if (n < 0) {
    perror("Unable to send.\n");
    exit(-1);
  }

  /* Receive the size of the map */
  n = recvfrom (sock, buffer, 256, 0, (struct sockaddr *) &from, &length);
  if (n < 0) {
    perror("Unable to receive.\n");
    exit(-1);
  }
  found = strchr(buffer, ':');
  *graph_sz = atol(found+1);

  close(sock);
  return 0;
}

