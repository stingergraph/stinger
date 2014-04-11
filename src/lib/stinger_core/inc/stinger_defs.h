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

#if !defined(STINGER_DEFAULT_VERTICES)
/** Default maximum number of vertices in allocated STINGER */
#define STINGER_DEFAULT_VERTICES (1L<<24)
#endif
#if !defined(STINGER_DEFAULT_NUMETYPES)
/** Default maximum number of edge types in allocated STINGER */
#define STINGER_DEFAULT_NUMETYPES 5
#endif
#if !defined(STINGER_DEFAULT_NUMVTYPES)
/** Default maximum number of vertex types with names - integer types can be
 *  INT64_MIN to INT64_MAX */
#define STINGER_DEFAULT_NUMVTYPES 128
#endif

#if !defined(STINGER_DEFAULT_NEB_FACTOR)
#define STINGER_DEFAULT_NEB_FACTOR 4
#endif

/** \def STINGER_DEFAULT_VERTICES
*   \brief Default maximum number of vertices in allocated STINGER
*/
/** \def STINGER_DEFAULT_NUMETYPES
*   \brief Default maximum number of edge types in allocated STINGER
*/
/** \def STINGER_DEFAULT_NUMVTYPES
*   \brief  Default maximum number of vertex types with names - integer types can be
*           INT64_MIN to INT64_MAX
*/


#if !defined(STINGER_EDGEBLOCKSIZE)
/** Edges per edge block */
#define STINGER_EDGEBLOCKSIZE 14
#endif
/** \def STINGER_EDGEBLOCKSIZE
*   \brief Number of edges stored in an edge block, users should not change 
*         this value as it is used statically
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
    assert (from < stinger_max_nv(G));  \
    assert (to < stinger_max_nv(G));    \
  } while (0)

#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* STINGER_DEFS_H_ */
