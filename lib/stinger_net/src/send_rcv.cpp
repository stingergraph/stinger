#include "send_rcv.h"

int
listen_for_client(int port)
{
  int sock_handle;
#ifdef STINGER_USE_TCP
  struct sockaddr_in sock_addr;
  memset(&sock_addr, 0, sizeof(sock_addr));
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_addr.s_addr = INADDR_ANY;
  sock_addr.sin_port   = htons(port);
#else
  struct sockaddr_un sock_addr;
  memset(&sock_addr, 0, sizeof(sock_addr));
  sock_addr.sun_family = AF_UNIX;
  snprintf(sock_addr.sun_path, sizeof(sock_addr.sun_path)-1, "/tmp/stinger.sock.%i", port);
  unlink(sock_addr.sun_path);
#endif

#ifdef STINGER_USE_TCP
  if(-1 == (sock_handle = socket(AF_INET, SOCK_STREAM, 0))) {
    LOG_F_A("Socket create failed: %s", strerror(errno));
    exit(-1);
  }

  if(-1 == bind(sock_handle, (const sockaddr *)&sock_addr, sizeof(sock_addr))) {
    LOG_F_A("Socket bind failed: %s", strerror(errno));
    exit(-1);
  }

  if(-1 == listen(sock_handle, 16)) {
    LOG_F_A("Socket listen failed: %s", strerror(errno));
    exit(-1);
  }

#else
  if(-1 == (sock_handle = socket(AF_UNIX, SOCK_STREAM, 0))) {
    LOG_F_A("Socket create failed: %s", strerror(errno));
    exit(-1);
  }

  if(-1 == bind(sock_handle, (const sockaddr *)&sock_addr, sizeof(sock_addr))) {
    LOG_F_A("Socket bind failed: %s", strerror(errno));
    exit(-1);
  }

  if(-1 == listen(sock_handle, 16)) {
    LOG_F_A("Socket listen failed: %s", strerror(errno));
    exit(-1);
  }
#endif
  return sock_handle;
}

int
connect_to_server (const char * hostname, int port)
{
#ifdef STINGER_USE_TCP

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
    LOG_E_A("Error resolving host [%s]", hostname);
    LOG_E_A("getaddrinfo: %s", gai_strerror(s));
    return -1;
  }

  int sock = socket(server_addr->ai_family, server_addr->ai_socktype, 0);
  if(-1 == sock) {
    LOG_E("Error opening socket");
    return -1;
  }

  char server_addr_string[INET6_ADDRSTRLEN];
  inet_ntop(server_addr->ai_family, server_addr, server_addr_string, INET6_ADDRSTRLEN);
  LOG_D_A("Socket open, connecting to host %s %s", hostname, server_addr_string);

  if(-1 == connect(sock, server_addr->ai_addr, server_addr->ai_addrlen)) {
    LOG_E("Error connecting socket");
    return -1;
  }
  freeaddrinfo(server_addr);
#else
  struct sockaddr_un sock_addr;
  memset(&sock_addr, 0, sizeof(sock_addr));
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if(-1 == sock) {
    LOG_E("Error opening socket");
    return -1;
  }
  snprintf(sock_addr.sun_path, sizeof(sock_addr.sun_path)-1, "/tmp/stinger.sock.%i", port);
  sock_addr.sun_family = AF_UNIX;

  LOG_D_A("Socket open, connecting to host %s", hostname);

  if(-1 == connect(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr))) {
    LOG_E("Error connecting socket");
    return -1;
  }
#endif

  return sock;
}