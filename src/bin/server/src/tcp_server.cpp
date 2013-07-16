#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


void error(const char *msg);

void dostuff (int sock)
{
  int n;
  char buffer[256];

  bzero (buffer, 256);
  n = read(sock, buffer, 255);
  if (n < 0)
    error ("ERROR reading from socket");
  printf ("Here is the message: %s\n", buffer);
  n = write (sock,"I got your message", 18);
  if (n < 0)
    error ("ERROR writing to socket");
}

void
thisWorks(int port)
{
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  pid_t pid;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = port;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &serv_addr,
	sizeof(serv_addr)) < 0) 
    error("ERROR on binding");
  listen(sockfd,5);
  clilen = sizeof(cli_addr);
  printf("Server listening on port %d\n", portno);
  while (1) {
    if (-1 == (newsockfd = accept(sockfd, 
	(struct sockaddr *) &cli_addr, &clilen)))
      error("oh no\n");
    if (newsockfd < 0) 
      error("ERROR on accept");
    pid = fork();
    if (pid < 0)
      error("ERROR on fork");
    if (pid == 0)  {
      close(sockfd);
      dostuff(newsockfd);
      exit(0);
    }
    else close(newsockfd);
  } /* end of while */
  close(sockfd);
  return; /* we never get here */
}
