typedef struct {
  int sock;
} mon_handler_params_t;

void *
mon_handler(void * args) {
  mon_handler_params_t * params = (mon_handler_params_t *)args;
}

void
mon_connect(int port, char * host) {
  struct sockaddr_in sock_addr;
  memset(&sock_addr, 0, sizeof(struct sockaddr_in));
  struct hostent * hp = gethostbyname(host);

  if(!hp) {
    LOG_E_A("Error resolving host %s", host);
    return NULL;
  }

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if(-1 == sock) {
    LOG_E("Error opening socket");
    return NULL;
  }

  memcpy(&sock_addr.sin_addr.s_addr, hp->h_addr, hp->h_length);
  sock_addr.sin_family = AF_INET;

  LOG_D_A("Socket open, connecting to host %s %s", host, inet_ntoa(**(struct in_addr **)hp->h_addr_list));

  if(port) {
    sock_addr.sin_port = htons(port);
  } else {
    sock_addr.sin_port = htons(10103);
  }

  if(-1 == connect(sock, (sockaddr *)&sock_addr, sizeof(struct sockaddr_in))) {
    LOG_E("Error connecting socket");
    return NULL;
  }


}
