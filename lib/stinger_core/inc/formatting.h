#if !defined(FORMATTING_H_)
#define FORMATTING_H_

#include <inttypes.h>

#if !defined(PRId64)
#if 64 == __WORDSIZE
#define PRId64 "ld"
#define PRIu64 "lu"
#else
#define PRId64 "lld"
#define PRIu64 "llu"
#endif
#endif

#endif /* FORMATTING_H_ */
