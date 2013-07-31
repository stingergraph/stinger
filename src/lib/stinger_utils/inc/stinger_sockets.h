#ifndef  STINGER_SOCKETS_H
#define  STINGER_SOCKETS_H

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

int
connect_to_batch_server (struct hostent * server, int port);

int
get_shared_map_info (char * hostname, int port, char ** name, size_t name_len, size_t * graph_sz);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif  /*STINGER_SOCKETS_H*/
