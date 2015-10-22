#include "graph_construct.h"

/**
* @brief Constructs a k-core using vertices [start, start+num). 
*
* Neighbors will be v +/- k/2. Edges will be undirected.
* <a href="http://en.wikipedia.org/wiki/Degeneracy_(graph_theory)#k-Cores">More at Wikipedia</a>
*
* @param S The stinger to insert into
* @param type Type of the edges created.
* @param time Timestamp for the instertion.
* @param weight Weight to be inserted (does increment).
* @param k K for the k-core. (Number of neighbors in the core each must have).
* @param start Vertex to start from.
* @param num Total number of vertices involved (must be at least k).
*/
int
construct_kcore(stinger_t * S, uint64_t type, uint64_t time, uint64_t weight, 
  uint64_t k, uint64_t start, uint64_t num) 
{
  for(uint64_t v = 0; v < num; v++) {
    uint64_t stop = (k+1)/2 + v;
    if((k % 2) && (v % 2))
      stop -= 1;
    for(uint64_t u = 1 + v; u <= stop; u++) {
      uint64_t i = v + start;
      uint64_t j = ((u >= num) ? (u - num) : u) + start;
      stinger_insert_edge_pair(S, type, i, j, weight, time);
    }
  }
  return !((num % k) && ((k*num) % 2));
}

/**
* @brief Constructs a k-clique using vertices [start, start+k). 
*
* All vertices will be connected. Edges will be undirected.
*
* @param S The stinger to insert into
* @param type Type of the edges created.
* @param time Timestamp for the instertion.
* @param weight Weight to be inserted (does increment).
* @param k K for the k-core. (Number of neighbors in the core each must have).
* @param start Vertex to start from.
* @param num Total number of vertices involved (must be at least k).
*/
int
construct_kclique(stinger_t * S, uint64_t type, uint64_t time, uint64_t weight, 
  uint64_t k, uint64_t start) 
{
  return construct_kcore(S, type, time, weight, k-1, start, k);
}
