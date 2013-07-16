#ifndef  STINGER_SHARED_H
#define  STINGER_SHARED_H

#include "stinger.h"

#define MAX_NAME_LEN 256

void *
shmmap (char * name, int oflags, mode_t mode, int prot, size_t size, int map);

int
shmunmap (char * name, void * ptr, size_t size);

struct stinger *
stinger_shared_new (char ** name);

struct stinger *
stinger_shared_map (char * name, size_t sz);

struct stinger *
stinger_shared_private (char * name, size_t sz);

struct stinger *
stinger_shared_free (struct stinger *S, char *name, size_t sz);

struct stinger *
stinger_shared_unmap (struct stinger *S, char *name, size_t sz);

#endif  /*STINGER_SHARED_H*/
