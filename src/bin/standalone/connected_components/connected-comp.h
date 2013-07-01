#if !defined(CONNECTED_COMP_H_)
#define CONNECTED_COMP_H_

int64_t connected_components (const size_t, const size_t,
			      const int64_t * restrict,
			      const int64_t * restrict, int64_t * restrict,
			      int64_t * restrict, int64_t * restrict,
			      int64_t * restrict, int64_t * restrict);

int64_t connected_components_stinger_edge_parallel_and_tree (const size_t
							     nv,
							     const size_t
							     ne,
							     const struct
							     stinger *S,
							     int64_t *
							     restrict d,
							     int64_t *
							     restrict T,
							     int64_t *
							     restrict tree);

int64_t connected_components_stinger_edge (const size_t nv,
					   const size_t ne,
					   const struct stinger *S,
					   int64_t * restrict d,
					   int64_t * restrict T);

void component_dist (const size_t, int64_t * restrict,
		     int64_t * restrict, const int64_t);

int64_t connected_components_stinger (const struct stinger *S,
				      const size_t nv, const size_t ne,
				      int64_t * restrict d,
				      int64_t * restrict crossV,
				      int64_t * restrict crossU,
				      int64_t * restrict marks,
				      int64_t * restrict T,
				      int64_t * restrict neighbors);

int spanning_tree_is_good (int64_t * restrict d, int64_t * restrict tree,
			   int64_t nv);

int64_t connected_components_edge (const size_t nv, const size_t ne,
				   const int64_t * restrict sV,
				   const int64_t * restrict eV,
				   int64_t * restrict D);

static void push (int64_t a, int64_t * stack, int64_t * top);

static int64_t pop (int64_t * stack, int64_t * top);

static int64_t is_empty (int64_t * stack, int64_t * top);

#endif
