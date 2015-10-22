#include "stinger_shared.h"
#include "stinger_error.h"
#include "x86_full_empty.h"
#include "core_util.h"
#include "xmalloc.h"

#define _XOPEN_SOURCE 600
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void
sigbus_handler(int sig, siginfo_t *si, void * vuctx)
{
  char * err_string = 
    "FATAL: stinger_shared.c X: Bus Error - writing to STINGER failed.  It is likely that your STINGER is too large.\n"
    "       You may need to increase the size of your /dev/shm to be larger than 1/2 of total memory.\n"
    "       See the 'Handling Common Errors' section of the README.md for more information on how to do this.\n";
  write(STDERR_FILENO, err_string, strlen(err_string));
  _exit(-1);
}

/** @brief Wrapper function to open and map shared memory.  
 * 
 * Internally calls shm_open, ftruncate, and mmap.  Can be used to open/map
 * existing shared memory or new shared memory. mmap() flags are always just
 * MAP_SHARED. Ftruncate is silently ignored (for secondary mappings). Names
 * must be less than 256 characters.
 *
 * @param name The string (beginning with /) name of the shared memory object.
 * @param oflags The open flags for shm_open.
 * @param mode The mode flags for shm_open.
 * @param prot The protection/permission flags for mmap. 
 * @param size The size of the memory object to be mapped.
 * @return A pointer to the shared memory or NULL on failure.
 */
void *
shmmap (const char * name, int oflags, mode_t mode, int prot, size_t size, int map) 
{
#if !defined(__MTA__)
  int fd = shm_open(name, oflags, mode);
#else
  int fd = open(name, oflags);
#endif

  if(fd == -1) {
    fprintf(stderr, "\nSHMMAP shm_open ERROR %s\n", strerror(errno)); fflush(stdout);
    return NULL;
  } 

#if !defined(__MTA__)
  /* set up SIGBUS handler */
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = sigbus_handler;
  sa.sa_flags = SA_SIGINFO;
  sigfillset(&sa.sa_mask);
  sigaction(SIGBUS, &sa, NULL);

#ifdef __APPLE__
  if(-1 == ftruncate(fd, size)) {
    int err = errno;
    if(errno != EINVAL)  //Will always return EINVAL if another process is currently attached to the segment.  This should only be called by master
      LOG_W_A("Mapping failed (it is likely your STINGER is too large -\n"
	"try reducing the number of vertices and/or edges per block in stinger_defs.h). Error was: %s", strerror(err));
  }
#else
  if(O_CREAT == (O_CREAT & oflags)) {
    if(-1 == ftruncate(fd, size)) {
      int err = errno;
      LOG_E_A("Mapping failed (it is likely that your STINGER is too large -\n"
	  "try reducing the number of vertices and/or edges per block in stinger_defs.h).  Error was: %s", strerror(err));
      return NULL;
    }
  }
#endif
  
  void * rtn = mmap(NULL, size, prot, MAP_SHARED, fd, 0);
#else
  void * rtn = mmap(NULL, size, prot, MAP_ANON|map, fd, 0);
#endif

  close(fd);

  if(rtn == MAP_FAILED) {
    LOG_E_A("mmap error [%s]. Try resizing STINGER to be smaller (see README.md).  If this doesn't work, please report this.", strerror(errno)); fflush(stdout);
    return NULL;
  }

  return rtn;
} 

/** @brief Wrapper function to unmap shared memory.
 * 
 * @param name The string (beginning with /) name of the shared memory object.
 * @param ptr The pointer to the shared memory.
 * @param size The size of the memory object to be removed.
 * @return 0 on success, -1 on failure.
 */
int
shmunmap (const char * name, void * ptr, size_t size) 
{
  if(munmap(ptr, size)) {
    LOG_E_A("Unmapping %p of size %ld failed", ptr, (long)size);
    return -1;
  }

  return 0;
}

/** @brief Wrapper function to unlink shared memory files in /dev/shm.
 * 
 * @param name The string (beginning with /) name of the shared memory object.
 * @return 0 on success, -1 on failure.
 */
int
shmunlink (const char * name) {
  LOG_D_A("Called shmunlink with %s", name);
#if !defined(__MTA__)
  if(shm_unlink(name)) {
    LOG_E_A("Unlinking %s", name);
    return -1;
  }
#else
  if(unlink(name))
    return -1;
#endif

  return 0;
}


/** @brief Create a new empty STINGER in shared memory.
 *
 * The stinger_shared is a struct containing all of the strings needed to map
 * in the resulting STINGER from another program.  The shared STINGER is stored
 * in shared memory mapped using the string stored in out.  The out string is 
 * all that is needed to map in the same stinger in another program.
 *
 * @param S The return for the stinger_shared structure.
 * @param out The return for the random stinger_shared name.
 * @return A pointer to the new stinger.
 */
struct stinger *
stinger_shared_new (char ** out)
{
  struct stinger_config_t * config = (struct stinger_config_t *)xcalloc(1,sizeof(struct stinger_config_t));

  struct stinger * s = stinger_shared_new_full(out, config);
  xfree(config);

  return s;
}

struct stinger *
stinger_shared_new_full (char ** out, struct stinger_config_t * config)
{
  if (*out == NULL)
  {
    *out = xmalloc(sizeof(char) * MAX_NAME_LEN);
#if !defined(__MTA__)
    sprintf(*out, "/%lx", (uint64_t)rand());
#else
    char *pwd = xmalloc (sizeof(char) * (MAX_NAME_LEN-16));
    getcwd(pwd,  MAX_NAME_LEN-16);
    sprintf(*out, "%s/%lx", pwd, (uint64_t)rand());
#endif
  }                              

  int64_t nv      = config->nv      ? config->nv      : STINGER_DEFAULT_VERTICES;
  int64_t nebs    = config->nebs    ? config->nebs    : STINGER_DEFAULT_NEB_FACTOR * nv;
  int64_t netypes = config->netypes ? config->netypes : STINGER_DEFAULT_NUMETYPES;
  int64_t nvtypes = config->nvtypes ? config->nvtypes : STINGER_DEFAULT_NUMVTYPES;

  size_t max_memsize_env = stinger_max_memsize();

  const size_t memory_size = (config->memory_size == 0) ? max_memsize_env : config->memory_size;
 
  size_t i;
  int resized   = 0;
  struct stinger_size_t sizes;

  while (1) {
    sizes = calculate_stinger_size(nv, nebs, netypes, nvtypes);

    if(sizes.size > (uint64_t)memory_size) {
      if (config->no_resize) {
        LOG_E("STINGER does not fit in memory.  no_resize set, so exiting.");
        exit(-1);
      }
      if(!resized) {
        LOG_W_A("Resizing stinger to fit into memory (detected as %ld)", memory_size);
      }
      resized = 1;

      nv    = (3*nv)/4;
      nebs  = STINGER_DEFAULT_NEB_FACTOR * nv;
    } else {
      break;
    }
  }

  struct stinger *G = shmmap (*out, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR,
    PROT_READ | PROT_WRITE, sizeof(struct stinger) + sizes.size, MAP_SHARED);

  if (!G) {
    perror("Failed to mmap STINGER graph.\n");
    exit(-1);
  }

  /* initialize the new data structure */
  xzero(G, sizeof(struct stinger) + sizes.size);

  G->max_nv       = nv;
  G->max_neblocks = nebs;
  G->max_netypes  = netypes;
  G->max_nvtypes  = nvtypes;

  G->num_insertions = 0;
  G->num_deletions = 0;
  G->num_insertions_last_batch = 0;
  G->num_deletions_last_batch = 0;
  G->batch_time = 0;
  G->update_time = 0;
  G->queue_size = 0;
  G->dropped_batches = 0;

  G->length = sizes.size;
  G->vertices_start = sizes.vertices_start;
  G->physmap_start = sizes.physmap_start;
  G->etype_names_start = sizes.etype_names_start;
  G->vtype_names_start = sizes.vtype_names_start;
  G->ETA_start = sizes.ETA_start;
  G->ebpool_start = sizes.ebpool_start;

  MAP_STING(G);

  int64_t zero = 0;
  stinger_vertices_init(vertices, nv);
  stinger_physmap_init(physmap, nv);
  stinger_names_init(etype_names, netypes);
  if (!config->no_map_none_etype) {
    stinger_names_create_type(etype_names, "None", &zero);
  }
  stinger_names_init(vtype_names, nvtypes);
  if (!config->no_map_none_vtype) {
    stinger_names_create_type(vtype_names, "None", &zero);
  };

  ebpool->ebpool_tail = 1;
  ebpool->is_shared = 0;

  OMP ("omp parallel for")
  MTA ("mta assert parallel")
  MTASTREAMS ()
  for (i = 0; i < netypes; ++i) {
    ETA(G,i)->length = nebs;
    ETA(G,i)->high = 0;
  }

  return G;
}

/** @brief Map in an existing STINGER in shared memory.
 *
 * Input the name obtained from stinger_shared_new above in a different program
 * to map in the same STINGER in shared memory.
 *
 * @param S The return for the stinger_shared structure.
 * @param out The input for the stinger_shared name.
 * @return A pointer to the stinger.
 */
struct stinger *
stinger_shared_map (const char * name, size_t sz)
{
  return shmmap (name, O_RDONLY, S_IRUSR, PROT_READ, sz, MAP_SHARED);
}

struct stinger *
stinger_shared_private (const char * name, size_t sz)
{
  return shmmap (name, O_RDONLY, S_IRUSR, PROT_READ, sz, MAP_PRIVATE);
}

/** @brief Unmap and unlink a shared STINGER from stinger_shared_new.
 * 
 * @param S The STINGER pointer.
 * @param shared The stinger_shared pointer.
 * @param name The name returned from stinger_shared_new.
 * @return A NULL pointer. Name and shared will also be freed.
 */
struct stinger *
stinger_shared_free (struct stinger *S, const char * name, size_t sz)
{
  if (!S)
    return S;

  int status = shmunmap(name, S, sz);
  //status = shmunlink(name);

  return NULL;
}

/** @brief Unmap a shared STINGER from another program.
 * 
 * @param S The STINGER pointer.
 * @param shared The stinger_shared pointer.
 * @param name The name used to map the shared STINGER.
 * @return A NULL pointer.
 */
struct stinger *
stinger_shared_unmap (struct stinger *S, const char *name, size_t sz)
{
  if(!S)
    return S;

  /* 
   * Letting the program closing clean these up since it seems to cause 
   * problems otherwise 
   */

  //free(S);
  return NULL;
}
