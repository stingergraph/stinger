#ifndef  GRAPH_CONSTRUCT_H
#define  GRAPH_CONSTRUCT_H

#include "stinger.h"

int
construct_kcore(stinger_t * S, uint64_t type, uint64_t time, uint64_t weight, 
  uint64_t k, uint64_t start, uint64_t num);

int
construct_kclique(stinger_t * S, uint64_t type, uint64_t time, uint64_t weight, 
  uint64_t k, uint64_t start);


#endif  /*GRAPH_CONSTRUCT_H*/
