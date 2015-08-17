#include <string>
#include <sys/types.h>

#include "stinger_alg.h"
#include "send_rcv.h"

extern "C" int stream_connect(char * server, int port);

extern "C" void stream_send_batch(int sock_handle, int only_strings,
    stinger_edge_update * insertions, int64_t num_insertions,
    stinger_edge_update * deletions, int64_t num_deletions);

