#include "stinger_shared.h"
#include "x86_full_empty.h"
#include "xmalloc.h"

#include <unistd.h>
#include <sys/mman.h>



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
shmmap (char * name, int oflags, mode_t mode, int prot, size_t size, int map) 
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
  int dontcare = ftruncate(fd, size);
  /* silently ignore ftruncate errors */
  
  void * rtn = mmap(NULL, size, prot, MAP_SHARED, fd, 0);
#else
  void * rtn = mmap(NULL, size, prot, MAP_ANON|map, fd, 0);
#endif

  if(rtn == MAP_FAILED) {
    fprintf(stderr, "\nSHMMAP mmap ERROR %s\n", strerror(errno)); fflush(stdout);
    return NULL;
  }

  return rtn;
} 

/** @brief Wrapper function to unmap / unlink shared memory.
 * 
 * @param name The string (beginning with /) name of the shared memory object.
 * @param ptr The pointer to the shared memory.
 * @param size The size of the memory object to be removed.
 * @return 0 on success, -1 on failure.
 */
int
shmunmap (char * name, void * ptr, size_t size) 
{
  if(munmap(ptr, size))
    return -1;

#if !defined(__MTA__)
  if(shm_unlink(name))
    return -1;
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
 
  size_t i;
  size_t sz = 0;

  size_t vertices_start = 0;
  sz += stinger_vertices_size(STINGER_MAX_LVERTICES);

  size_t ebpool_start = sz;
  sz += sizeof(struct stinger_ebpool);

  size_t etype_names_start = sz;
  sz += stinger_names_size(STINGER_NUMETYPES);

  size_t vtype_names_start = sz;
  sz += stinger_names_size(STINGER_NUMVTYPES);

  size_t ETA_start = sz;
  sz += STINGER_NUMETYPES * sizeof(struct stinger_etype_array);

  size_t length = sz;

  struct stinger *G = shmmap (*out, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR,
    PROT_READ | PROT_WRITE, sizeof(struct stinger) + sz, MAP_SHARED);

  if (!G) {
    perror("Failed to mmap STINGER graph.\n");
    exit(-1);
  }

  /* initialize the new data structure */
  xzero(G, sizeof(struct stinger) + sz);
  G->length = length;
  G->vertices_start = vertices_start;
  G->etype_names_start = etype_names_start;
  G->vtype_names_start = vtype_names_start;
  G->ETA_start = ETA_start;
  G->ebpool_start = ebpool_start;

  MAP_STING(G);

  stinger_vertices_init(vertices, STINGER_MAX_LVERTICES);
  stinger_names_init(etype_names, STINGER_NUMETYPES);
  stinger_names_init(vtype_names, STINGER_NUMVTYPES);

  ebpool->ebpool_tail = 1;
  ebpool->is_shared = 0;

#if STINGER_NUMETYPES == 1
  ETA[0].length = EBPOOL_SIZE;
  ETA[0].high = 0;
#else
  OMP ("omp parallel for")
  MTA ("mta assert parallel")
  MTASTREAMS ()
  for (i = 0; i < STINGER_NUMETYPES; ++i) {
    ETA[i].length = EBPOOL_SIZE;
    ETA[i].high = 0;
  }
#endif
  printf("Shared: %s\n", *out);

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
stinger_shared_map (char * name, size_t sz)
{
  return shmmap (name, O_RDONLY, S_IRUSR, PROT_READ, sz, MAP_SHARED);
}

struct stinger *
stinger_shared_private (char * name, size_t sz)
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
stinger_shared_free (struct stinger *S, char * name, size_t sz)
{
  if (!S)
    return S;

  int status = shmunmap(name, S, sz);
  if (status < 0)
    fprintf(stderr, "whooops\n");
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
stinger_shared_unmap (struct stinger *S, char *name, size_t sz)
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
