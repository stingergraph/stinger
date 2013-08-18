#include "stinger_core/stinger.h"
#include "stinger_utils/stinger_la.h"

int
main(int argc, char *argv[]) 
{
  stinger_t * S = stinger_new();

  /* build a 10x10 matrix */
  for(int64_t v = 0; v < 9; v++) {
    stinger_insert_edge(S, 0, v, v+1, 1, 1);
  }

  int64_t vec[] = {0,1,2,3,4,5,6,7,8,9};
  int64_t out[10];

  stinger_matvec(S, vec, 10, out, stinger_mult, stinger_add, 0);

  for(int64_t v = 0; v < 10; v++) {
    printf("in[%ld] = %ld, out[%ld] = %ld\n", (long)v, (long)vec[v], (long)v, (long)out[v]);
  }

  return 0;
}
