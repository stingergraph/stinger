#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

void error(const char *msg)
{
  perror(msg);
  exit(1);
}

void
start_udp_graph_name_server (char * graph_name, size_t graph_sz, int port)
{
  int sock, length, n;
  socklen_t fromlen;
  struct sockaddr_in server;
  struct sockaddr_in from;
  char buf[1024];

  assert (port > 1024);

  sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
  {
    perror("Socket create failed.\n");
    exit(-1);
  }

  length = sizeof(server);
  bzero (&server, length);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port + 1);

  if (bind(sock, (struct sockaddr *) &server, length) < 0) 
  {
    perror("Socket bind failed.\n");
    exit(-1);
  }

  fromlen = sizeof(struct sockaddr_in);

  printf("Server listening on port %d\n", port + 1);

  while (1) {
    n = recvfrom(sock, buf, 1024, 0, (struct sockaddr *) &from, &fromlen);
    if (n < 0)
    {
      perror("Error receiving datagram.\n");
      exit(-1);
    }
    //write(1, "Received a datagram: ", 21);
    //write(1, buf, n);

    if (!strncmp(buf, "NAME", 4))
    {
      sprintf(buf, "NAME:%s", graph_name);
      n = sendto (sock, buf, strlen(buf), 0, (struct sockaddr *) &from, fromlen);
    }
    else if (!strncmp(buf, "SIZE", 4))
    {
      sprintf(buf, "SIZE:%ld", graph_sz);
      n = sendto (sock, buf, strlen(buf), 0, (struct sockaddr *) &from, fromlen);
    }
    else {
      n = sendto (sock, "Default:", 9, 0, (struct sockaddr *) &from, fromlen);
    }
  }

}
