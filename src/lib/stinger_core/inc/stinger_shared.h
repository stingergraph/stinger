#ifndef  STINGER_SHARED_H
#define  STINGER_SHARED_H

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#include "stinger.h"

#define MAX_NAME_LEN 256

void *
shmmap (const char * name, int oflags, mode_t mode, int prot, size_t size, int map);

int
shmunmap (const char * name, void * ptr, size_t size);

int shmunmap_unlink (const char * name, void * ptr, size_t size);

struct stinger *
stinger_shared_new (char ** name);

struct stinger *
stinger_shared_new_full (char ** out, int64_t nv, int64_t nebs, int64_t netypes, int64_t nvtypes);

struct stinger *
stinger_shared_map (const char * name, size_t sz);

struct stinger *
stinger_shared_private (const char * name, size_t sz);

struct stinger *
stinger_shared_free (struct stinger *S, const char *name, size_t sz);

struct stinger *
stinger_shared_unmap (struct stinger *S, const char *name, size_t sz);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif  /*STINGER_SHARED_H*/
