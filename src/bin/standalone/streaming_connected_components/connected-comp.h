#if !defined(CONNECTED_COMP_H_)
#define CONNECTED_COMP_H_

#include "stinger_core/stinger.h"

int64_t
connected_components (const size_t nv,
		      const size_t ne,
		      const int64_t * restrict off,
		      const int64_t * restrict ind,
		      int64_t * restrict d,
		      int64_t * restrict crossV,
		      int64_t * restrict crossU,
		      int64_t * restrict marks,
		      int64_t * restrict T);

int64_t
connected_components_stinger_edge_parallel_and_tree (const size_t nv, 
						     const size_t ne, 
						     const struct stinger *S,
						     int64_t * restrict d,
						     int64_t * restrict T,
						     int64_t * restrict tree);

int64_t
connected_components_stinger_edge (const size_t nv, 
				   const size_t ne, 
				   const struct stinger *S,
				   int64_t * restrict d,
				   int64_t * restrict T);

void
component_dist (const size_t,
		const int64_t * restrict,
		int64_t * restrict,
		const int64_t);

int64_t
connected_components_stinger (const struct stinger *S,
			      const size_t nv,
			      const size_t ne,
			      int64_t * restrict d,
			      int64_t * restrict crossV,
			      int64_t * restrict crossU,
			      int64_t * restrict marks,
			      int64_t * restrict T,
			      int64_t * restrict neighbors);

int
spanning_tree_is_good (int64_t * restrict d,
		       int64_t * restrict tree,
		       int64_t nv);

int64_t
connected_components_edge (const size_t nv,
			   const size_t ne,
			   const int64_t * restrict sV,
			   const int64_t * restrict eV,
			   int64_t * restrict D);

void
bfs_stinger (const struct stinger *G,
	     const int64_t nv,
	     const int64_t ne,
	     const int64_t startV,
	     int64_t * marks,
	     const int64_t numSteps,
	     int64_t *Q,
	     int64_t *QHead,
	     int64_t *neighbors);
#endif
