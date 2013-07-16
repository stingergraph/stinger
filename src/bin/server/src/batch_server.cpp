#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include "server.h"


void
dostuff(struct stinger * S, int sock, uint64_t buffer_size)
{
  const char * buffer = (char *) malloc (buffer_size);
  if (!buffer) {
    perror("Buffer alloc failed.\n");
    exit(-1);
  }

  int nfail = 0;

  V("Ready to accept messages.");
  while(1)
  {
    StingerBatch batch;
    if (recv_message(sock, batch)) {
      nfail = 0;

      V_A("Received message of size %ld", (long)batch.ByteSize());

      if (0 == batch.insertions_size () && 0 == batch.deletions_size ()) {
	V("Empty batch.");
	if (!batch.keep_alive ())
	  break;
	else
	  continue;
      }

      process_batch(S, batch, NULL);

      if(!batch.keep_alive())
	break;

    } else {
      ++nfail;
      V("ERROR Parsing failed.\n");
      if (nfail > 2) break;
    }
  }
}

void
start_tcp_batch_server (struct stinger * S, int port, uint64_t buffer_size)
{
  int sock_handle, newsockfd;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  pid_t pid;

  if (-1 == (sock_handle = socket(AF_INET, SOCK_STREAM, 0))) {
    perror("Socket create failed.\n");
    exit(-1);
  }

  bzero ((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);

  if (-1 == bind(sock_handle, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) {
    perror("Socket bind failed.\n");
    exit(-1);
  }

  if (-1 == listen(sock_handle, 5)) {
    perror("Socket listen failed.\n");
    exit(-1);
  }

  clilen = sizeof(cli_addr);

  V_A("STINGER server listening for input on port %d, web server on port 8088.",
      (int)port);

  while (1) {
    newsockfd = accept (sock_handle, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0)
    {
      perror("Accept new connection failed.\n");
      exit(-1);
    }
    pid = fork();
    if (pid < 0) {
      perror("Unable to fork.\n");
      exit(-1);
    }
    if (pid == 0)
    {
      close(sock_handle);
      dostuff(S, newsockfd, buffer_size);
      exit(0);
    }
    else
      close(newsockfd);
  }

  close(sock_handle);
  return;





}
