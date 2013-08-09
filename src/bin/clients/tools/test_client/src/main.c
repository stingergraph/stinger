#include <stdio.h>
#include <stdlib.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_shared.h"

#include "stinger_utils/stinger_sockets.h"

int main (int argc, char *argv[])
{
  int port = 10101;
  char hostname[] = "localhost";
  char name[1024];
  size_t sz;

  get_shared_map_info (hostname, port, (char **) &name, 1024, &sz);

  printf("hostname: %s\n", hostname);
  printf("port: %d\n", port);
  printf("map_name: %s\n", name);
  printf("size: %ld\n", sz);

  struct stinger * S = stinger_shared_map (name, sz);

  uint64_t nv = stinger_num_active_vertices (S);
  uint64_t max_nv = stinger_max_active_vertex (S);
  int64_t ne = stinger_total_edges (S);
  uint32_t check = stinger_consistency_check (S, max_nv+1);

  printf("nv: %ld\n", nv);
  printf("max_nv: %ld\n", max_nv);
  printf("ne: %ld\n", ne);
  printf("check: %d\n", check);

  stinger_shared_unmap (S, name, sz);

  return 0;
}
