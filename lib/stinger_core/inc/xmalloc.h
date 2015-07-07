#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>

#if !defined(XMALLOC_H_)
#define XMALLOC_H_

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#if defined(__GNUC__)
#define FNATTR_MALLOC __attribute__((malloc))
#else
#define FNATTR_MALLOC
#endif

void * xmalloc (size_t) FNATTR_MALLOC;
void * xcalloc (size_t, size_t) FNATTR_MALLOC;
void * xrealloc (void *, size_t) FNATTR_MALLOC;
void * xmmap (void *addr, size_t len, int prot, int flags, int fd, off_t offset);
void xelemset (int64_t *s, int64_t c, int64_t n);
void xelemcpy (int64_t *dest, int64_t *src, int64_t n);
void xzero (void *x, const size_t sz);

/**
* @brief Self-checking wrapper to free()
*
* @param p Pointer to allocation
*/
#define xfree(p) \
{ \
  if (p != NULL) { \
    free(p); \
    p = NULL; \
  } \
}

#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* XMALLOC_H_ */
