#if !defined(STINGER_DEFS_H_)
#define STINGER_DEFS_H_

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

/** @file */

/**
* @defgroup STINGER_DEFINE Graph Definitions
*
* @{
*/

/** Maximum number of vertices in STINGER */
#if !defined(STINGER_MAX_LVERTICES)
#if defined (__MTA__)
/* XXX: Assume only 2**25 vertices */
#define STINGER_MAX_LVERTICES (1L<<27)
#else
/* much smaller for quick testing */
#define STINGER_MAX_LVERTICES (1L<<20)
#endif
#endif
/** Edges per edge block */
#define STINGER_EDGEBLOCKSIZE 14
/** Number of edge types */
#define STINGER_NUMETYPES 5
/** Number of vertex types (with names) */
#define STINGER_NUMVTYPES 128

/** \def STINGER_MAX_LVERTICES
*   \brief Maximum number of vertices that STINGER can support
*/
/** \def STINGER_EDGEBLOCKSIZE
*   \brief Number of edges stored in an edge block
*/
/** \def STINGER_NUMETYPES
*   \brief Number of edge types allowed.  Edge type values are 0 to STINGER_NUMETYPES - 1.
*/


/** @} */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#if !defined(__MTA__)
#include <libgen.h>
#endif
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(__MTA__)
#define MTA(x) _Pragma(x)
#if defined(MTA_STREAMS)
#define MTASTREAMS() MTA(MTA_STREAMS)
#else
#define MTASTREAMS() MTA("mta use 70 streams")
#endif
#else
#define MTA(x)
#define MTASTREAMS()
#endif

#if defined(_OPENMP)
#define OMP(x) _Pragma(x)
#else
#define OMP(x)
#endif

#if defined(__GNUC__)
#define FNATTR_MALLOC __attribute__((malloc))
#define UNLIKELY(x) __builtin_expect((x),0)
#define LIKELY(x) __builtin_expect((x),1)
#else
#define FNATTR_MALLOC
#define UNLIKELY(x) x
#define LIKELY(x) x
#endif

#if !defined(__MTA__) & 0
#define XMTI MTA("mta inline")
#else
#define XMTI
#endif

#define STINGERASSERTS()		    \
  do {					    \
    assert (G);				    \
    assert (from >= 0);			    \
    assert (to >= 0);			    \
    assert (from < STINGER_MAX_LVERTICES);  \
    assert (to < STINGER_MAX_LVERTICES);    \
  } while (0)

#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* STINGER_DEFS_H_ */
