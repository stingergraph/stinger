#if !defined(SORTS_HEADER_)
#define SORTS_HEADER_

/* iki == two intvtx_t's, first is key */

void insertion_iki (intvtx_t * restrict, const size_t);
void shellsort_iki (intvtx_t * restrict, const size_t);
void introsort_iki (intvtx_t * restrict, const size_t);

void insertion_ikii (intvtx_t * restrict, const size_t);
void shellsort_ikii (intvtx_t * restrict, const size_t);
void introsort_ikii (intvtx_t * restrict, const size_t);

#endif /* SORTS_HEADER_ */
