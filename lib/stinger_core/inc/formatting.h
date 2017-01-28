#if !defined(FORMATTING_H_)
#define FORMATTING_H_

#include <inttypes.h>

#if !defined(PRId64)
#if 64 == __WORDSIZE
#define PRId64 "ld"
#else
#define PRId64 "lld"
#endif
#endif

#endif /* FORMATTING_H_ */
