#include "stinger_core/stinger_atomics.h"
#include "stinger_utils/stinger_utils.h"
#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"

#define N 500000


  static void
push (int64_t a, int64_t * stack, int64_t * top)
{
  (*top)++;
  stack[*top] = a;
}

  static int64_t
pop (int64_t * stack, int64_t * top)
{
  int a = stack[*top];
  (*top)--;
  return a;
}

  static int64_t
is_empty (int64_t * stack, int64_t * top)
{
  if ((*top)==-1) return 1;
  else return 0;
}


  int64_t
connected_components (const size_t nv,
    const size_t ne,
    const int64_t * restrict off,
    const int64_t * restrict ind,
    int64_t * restrict d,
    int64_t * restrict crossV,
    int64_t * restrict crossU,
    int64_t * restrict marks,
    int64_t * restrict T)
{
  int64_t i, change, k = 0, count = 0, cross_count = 0;
  int64_t *stackstore = NULL;

  int64_t freeV = 0;
  int64_t freeU = 0;
  int64_t freemarks = 0;
  int64_t freeT = 0;

  if (crossV == NULL) {
    crossV = xmalloc (2 * ne * sizeof(int64_t));
    freeV = 1;
  }
  if (crossU == NULL)	{
    crossU = xmalloc (2 * ne * sizeof(int64_t));
    freeU = 1;
  }
  if (marks == NULL) {
    marks = xmalloc (nv * sizeof(int64_t));
    freemarks = 1;
  }
  if (T == NULL) {
    T = xmalloc (nv * sizeof(int64_t));
    freeT = 1;
  }

#if !defined(__MTA__)
  stackstore = xmalloc (nv * sizeof(int64_t));
#endif

  MTA("mta assert nodep")
    for (i = 0; i < nv; i++) {
      d[i] = i;
      marks[i] = 0;
    }

  MTA("mta assert nodep")
    for (i = 0; i < nv; i++) {
#if !defined(__MTA__)
      int64_t * stack = stackstore;
#else
      int64_t stack[N];
#endif
      int64_t my_root;
      int64_t my_color = i+1;
      int64_t top = -1;
      int64_t v, u;

      if (stinger_int64_fetch_add(marks + i, 1) == 0) {
	my_color = i+1;
	top = -1;
	my_root = i;
	push(my_root, stack, &top);
	while (!is_empty(stack, &top)) {
	  int64_t myStart, myEnd;
	  int64_t k;
	  u = pop(stack, &top);
	  myStart = off[u];
	  myEnd = off[u+1];

	  for (k = myStart; k < myEnd; k++) {
	    v = ind[k];
	    assert(v < nv);
	    if (stinger_int64_fetch_add(marks + v, 1) == 0) {
	      d[v] = my_root;
	      push(v, stack, &top);
	    }
	    else {
	      if (!(d[v]==d[my_root])) {
		int64_t t = stinger_int64_fetch_add(&cross_count, 1);
		crossU[t] = u;
		crossV[t] = v;
	      }
	    }
	  }
	}
      }
    }

  change = 1;
  k = 0;
  while (change) {
    change = 0;
    k++;

    MTA("mta assert nodep")
      for (i = 0; i < cross_count; i++) {
	int64_t u = crossU[i];
	int64_t v = crossV[i];
	int64_t tmp;
	if (d[u] < d[v] && d[d[v]] == d[v]) {
	  d[d[v]] = d[u];
	  change = 1;
	}
	tmp = u;
	u = v;
	v = tmp;
	if (d[u] < d[v] && d[d[v]] == d[v]) {
	  d[d[v]] = d[u];
	  change = 1;
	}
      }

    if (change == 0) break;

    MTA("mta assert nodep");
    for (i = 0; i < nv; i++) {
      while (d[i] != d[d[i]])
	d[i] = d[d[i]];
    }
  }

  for (i = 0; i < nv; i++) {
    T[i] = 0;
  }

  MTA("mta assert nodep")
    for (i = 0; i < nv; i++) {
      int64_t temp = 0;
      if (stinger_int64_fetch_add(T+d[i], 1) == 0) {
	temp = 1;
      }
      count += temp;
    }

#if !defined(__MTA__)
  free(stackstore);
#endif
  if (freeT) free(T);
  if (freemarks) free(marks);
  if (freeU) free(crossU);
  if (freeV) free(crossV);

  return count;

}

int64_t
connected_components_stinger_edge_parallel_and_tree(
    const size_t nv,
    const size_t ne,
    const struct stinger *S,
    int64_t * restrict d,
    int64_t * restrict T,
    int64_t * restrict tree)
{
  int64_t i, change, count = 0, cross_count = 0, freeT = 0;

  if (T == NULL) {
    T = xmalloc (nv * sizeof(int64_t));
    freeT = 1;
  }

  assert(STINGER_NUMETYPES == 1);

  OMP("omp parallel") {
    OMP("omp for")
      for(uint64_t k = 0; k < nv; ++k) {
	d[k] = k;
	tree[k] = k;
      }

    while(1) {
      OMP("omp single")
	change = 0;
      STINGER_FORALL_EDGES_BEGIN(S, 0) {
	if(d[STINGER_EDGE_SOURCE] < d[STINGER_EDGE_DEST]) {
	  // NOTE: Do not change this to d[d[v]].  It breaks the tree. The
	  // data flow relationship represented in the tree between a vertex
	  // and its neighbors is not correct unless we move data accross one
	  // physical edge at a time.
	  d[STINGER_EDGE_DEST] = d[STINGER_EDGE_SOURCE];
	  tree[STINGER_EDGE_DEST] = STINGER_EDGE_SOURCE;
	  ++change;
	}
      } STINGER_FORALL_EDGES_END();

      if(!change) break;

    }

    OMP("omp for")
      for (i = 0; i < nv; i++) {
	T[i] = 0;
      }

    OMP("omp for reduction(+:count)")
      MTA("mta assert nodep")
      for (i = 0; i < nv; i++) {
	int64_t temp = 0;
	if (stinger_int64_fetch_add(T+d[i], 1) == 0) {
	  temp = 1;
	}
	/* stinger_int64_fetch_add(&count, temp); */
	count += temp;
      }
  }

  if (freeT) free(T);

  return count;
}

  int64_t
connected_components_stinger_edge (const size_t nv,
    const size_t ne,
    const struct stinger *S,
    int64_t * restrict d,
    int64_t * restrict T)
{
  int64_t i, change, count = 0, cross_count = 0, freeT = 0;

  if (T == NULL) {
    T = xmalloc (nv * sizeof(int64_t));
    freeT = 1;
  }

  assert(STINGER_NUMETYPES == 1);

  OMP("omp parallel") {
    OMP("omp for")
      for(uint64_t k = 0; k < nv; ++k) {
	d[k] = k;
      }

    while(1) {
      OMP("omp single")
	change = 0;
      STINGER_PARALLEL_FORALL_EDGES_BEGIN(S, 0) {
	if(d[STINGER_EDGE_SOURCE] < d[STINGER_EDGE_DEST]) {
	  // NOTE: Do not change this to d[d[v]].  It breaks the tree. The
	  // data flow relationship represented in the tree between a vertex
	  // and its neighbors is not correct unless we move data accross one
	  // physical edge at a time.
	  d[d[STINGER_EDGE_DEST]] = d[STINGER_EDGE_SOURCE];
	  ++change;
	}
      } STINGER_PARALLEL_FORALL_EDGES_END();

      if(!change) break;

      OMP("omp for")
	for(uint64_t l = 0; l < nv; ++l)
	  while(d[l] != d[d[l]])
	    d[l] = d[d[l]];
    }

    OMP("omp for")
      for (i = 0; i < nv; i++) {
	T[i] = 0;
      }

    OMP("omp for")
      for (i = 0; i < nv; i++) {
	int64_t temp = 0;
	if (stinger_int64_fetch_add(T+d[i], 1) == 0) {
	  temp = 1;
	}
	stinger_int64_fetch_add(&count, temp);
      }
  }

  if (freeT) free(T);

  return count;
}

  int
spanning_tree_is_good (int64_t * restrict d,
    int64_t * restrict tree,
    int64_t nv)
{
  int rtn = 0;

  OMP("omp parallel for reduction(+:rtn)")
    for(uint64_t i = 0; i < nv; i++) {
      uint64_t j = i;
      while(j != tree[j])
	j = tree[tree[j]];
      if(j != d[i])
	rtn++;
    }
  return rtn == 0;
}

  int64_t
connected_components_edge (const size_t nv,
    const size_t ne,
    const int64_t * restrict sV,
    const int64_t * restrict eV,
    int64_t * restrict D)
{
  int64_t count = 0;
  int64_t nchanged;
  int64_t freemarks = 0;

  if (D == NULL) {
    D = xmalloc (nv * sizeof(int64_t));
    freemarks = 1;
  }

  OMP("omp parallel") {
    while (1) {
      OMP("omp single") nchanged = 0;
      MTA("mta assert nodep") OMP("omp for reduction(+:nchanged)")
	for (int64_t k = 0; k < ne; ++k) {
	  const int64_t i = sV[k];
	  const int64_t j = eV[k];
	  if (D[i] < D[j]) {
	    D[D[j]] = D[i];
	    ++nchanged;
	  }
	}
      if (!nchanged) break;
      MTA("mta assert nodep") OMP("omp for")
	for (int64_t i = 0; i < nv; ++i)
	  while (D[i] != D[D[i]])
	    D[i] = D[D[i]];
    }

    MTA("mta assert nodep") OMP("omp for reduction(+:count)")
      for (int64_t i = 0; i < nv; ++i) {
	while (D[i] != D[D[i]])
	  D[i] = D[D[i]];
	if (D[i] == i) ++count;
      }
  }

  if (freemarks) free(D);

  return count;
}


  void
component_dist (const size_t nv,
    int64_t * restrict d,
    int64_t * restrict cardinality,
    const int64_t numColors)
{
  uint64_t sum = 0, sum_sq = 0;
  double average, avg_sq, variance, std_dev;
  int64_t i;

  uint64_t max = 0, maxV = 0;

  OMP("parallel for")
    MTA("mta assert nodep")
    for (i = 0; i < nv; i++) {
      cardinality[i] = 0;
    }

  for (i = 0; i < nv; i++) {
    cardinality[d[i]]++;
  }

  for (i = 0; i < nv; i++) {
    uint64_t degree = cardinality[i];
    sum_sq += degree*degree;
    sum += degree;

    if (degree > max) {
      max = degree;
    }
  }

  uint64_t max2 = 0;
  uint64_t checker = max;

  MTA("mta assert nodep")
    for (i = 0; i < nv; i++) {
      uint64_t degree = cardinality[i];
      uint64_t tmp = (checker == degree ? i : 0);
      if (tmp > max2)
	max2 = tmp;
    }
  maxV = max2;

  average = (double) sum / numColors;
  avg_sq = (double) sum_sq / numColors;
  variance = avg_sq - (average*average);
  std_dev = sqrt(variance);

  PRINT_STAT_INT64 ("max_num_vertices", max);
  PRINT_STAT_DOUBLE ("avg_num_vertices", average);
  PRINT_STAT_DOUBLE ("exp_value_x2", avg_sq);
  PRINT_STAT_DOUBLE ("variance", variance);
  PRINT_STAT_DOUBLE ("std_dev", std_dev);

}


  int64_t
connected_components_stinger (const struct stinger *S,
    const size_t nv,
    const size_t ne,
    int64_t * restrict d,
    int64_t * restrict crossV,
    int64_t * restrict crossU,
    int64_t * restrict marks,
    int64_t * restrict T,
    int64_t * restrict neighbors)
{
  int64_t i, change, k = 0, count = 0, cross_count = 0;
  int64_t *stackstore = NULL;

  int64_t freeV = 0;
  int64_t freeU = 0;
  int64_t freemarks = 0;
  int64_t freeT = 0;
  int64_t freeneighbors = 0;

  if (crossV == NULL) {
    crossV = xmalloc (2 * ne * sizeof(int64_t));
    freeV = 1;
  }
  if (crossU == NULL)	{
    crossU = xmalloc (2 * ne * sizeof(int64_t));
    freeU = 1;
  }
  if (marks == NULL) {
    marks = xmalloc (nv * sizeof(int64_t));
    freemarks = 1;
  }
  if (T == NULL) {
    T = xmalloc (nv * sizeof(int64_t));
    freeT = 1;
  }
  if (neighbors == NULL) {
    neighbors = xmalloc (2 * ne * sizeof(int64_t));
    freeneighbors = 1;
  }

#if !defined(__MTA__)
  stackstore = xmalloc (nv * sizeof(int64_t));
#endif

  MTA("mta assert nodep")
    for (i = 0; i < nv; i++) {
      d[i] = i;
      marks[i] = 0;
    }

  int64_t head = 0;

  MTA("mta assert nodep")
    for (i = 0; i < nv; i++) {
#if !defined(__MTA__)
      int64_t * stack = stackstore;
#else
      int64_t stack[N];
#endif
      int64_t my_root;
      int64_t my_color = i+1;
      int64_t top = -1;
      int64_t v, u;
      int64_t deg_u;

      if (stinger_int64_fetch_add(marks + i, 1) == 0) {
	my_color = i+1;
	top = -1;
	my_root = i;
	push(my_root, stack, &top);
	while (!is_empty(stack, &top)) {
	  int64_t myStart, myEnd;
	  int64_t k;
	  size_t md;
	  u = pop(stack, &top);
	  deg_u = stinger_outdegree(S, u);
	  myStart = stinger_int64_fetch_add(&head, deg_u);
	  myEnd = myStart + deg_u;
	  stinger_gather_typed_successors(S, 0, u, &md, &neighbors[myStart], deg_u);

	  for (k = myStart; k < myEnd; k++) {
	    v = neighbors[k];
	    if (stinger_int64_fetch_add(marks + v, 1) == 0) {
	      d[v] = my_root;
	      push(v, stack, &top);
	    }
	    else {
	      if (!(d[v]==d[my_root])) {
		int64_t t = stinger_int64_fetch_add(&cross_count, 1);
		crossU[t] = u;
		crossV[t] = v;
	      }
	    }
	  }
	}
      }
    }

  change = 1;
  k = 0;
  while (change) {
    change = 0;
    k++;

    MTA("mta assert nodep")
      for (i = 0; i < cross_count; i++) {
	int64_t u = crossU[i];
	int64_t v = crossV[i];
	int64_t tmp;
	if (d[u] < d[v] && d[d[v]] == d[v]) {
	  d[d[v]] = d[u];
	  change = 1;
	}
	tmp = u;
	u = v;
	v = tmp;
	if (d[u] < d[v] && d[d[v]] == d[v]) {
	  d[d[v]] = d[u];
	  change = 1;
	}
      }

    if (change == 0) break;

    MTA("mta assert nodep");
    for (i = 0; i < nv; i++) {
      while (d[i] != d[d[i]])
	d[i] = d[d[i]];
    }
  }

  for (i = 0; i < nv; i++) {
    T[i] = 0;
  }

  MTA("mta assert nodep")
    for (i = 0; i < nv; i++) {
      int64_t temp = 0;
      if (stinger_int64_fetch_add(T+d[i], 1) == 0) {
	temp = 1;
      }
      count += temp;
    }

#if !defined(__MTA__)
  free(stackstore);
#endif
  if (freeneighbors) free(neighbors);
  if (freeT) free(T);
  if (freemarks) free(marks);
  if (freeU) free(crossU);
  if (freeV) free(crossV);

  return count;

}


  void
bfs_stinger (const struct stinger *G,
    const int64_t nv,
    const int64_t ne,
    const int64_t startV,
    int64_t * marks,
    const int64_t numSteps,
    int64_t *Q,
    int64_t *QHead,
    int64_t *neighbors)
{
  int64_t j, k, s;
  int64_t nQ, Qnext, Qstart, Qend;
  int64_t w_start;

  int64_t d_phase;

  MTA("mta assert nodep")
    for (j = 0; j < nv; j++) {
      marks[j] = 0;
    }

  s = startV;
  /* Push node s onto Q and set bounds for first Q sublist */
  Q[0] = s;
  Qnext = 1;
  nQ = 1;
  QHead[0] = 0;
  QHead[1] = 1;
  marks[s] = 1;

PushOnStack:    /* Push nodes onto Q */

  /* Execute the nested loop for each node v on the Q AND
     for each neighbor w of v  */
  d_phase = nQ;
  Qstart = QHead[nQ-1];
  Qend = QHead[nQ];
  w_start = 0;

  MTA("mta assert no dependence")
    MTA("mta block dynamic schedule")
    for (j = Qstart; j < Qend; j++) {
      int64_t v = Q[j];
      size_t d;
      size_t deg_v = stinger_outdegree (G, v);
      int64_t my_w = stinger_int64_fetch_add(&w_start, deg_v);
      stinger_gather_typed_successors (G, 0, v, &d, &neighbors[my_w], deg_v);
      assert (d == deg_v);

      MTA("mta assert nodep")
	for (k = 0; k < deg_v; k++) {
	  int64_t d, w, l;
	  w = neighbors[my_w + k];
	  /* If node has not been visited, set distance and push on Q */
	  if (stinger_int64_fetch_add(marks + w, 1) == 0) {
	    Q[stinger_int64_fetch_add(&Qnext, 1)] = w;
	  }
	}
    }

  if (Qnext != QHead[nQ] && nQ < numSteps) {
    nQ++;
    QHead[nQ] = Qnext;
    goto PushOnStack;
  }
}
