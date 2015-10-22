#include "vtx_set.h"

#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define VTX_SET_MMAP_PATH "/dev/shm/vtx_set/"

void *
mmalloc(char * name, int64_t size) {
  char filename[1024] = VTX_SET_MMAP_PATH;
  strcat(filename, name);
  int fh = open(filename, O_CREAT | O_RDWR, S_IRWXU);

  if(-1 == fh) {
    perror("open");
    exit(-1);
  }

  if(-1 == ftruncate(fh, size)) {
    perror("truncate");
    exit(-1);
  }

  char * p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fh, 0);
  close(fh);
  return p;
}

void
mfree(void * p, int64_t size) {
  munmap(p, size);
}

void *
mreplace(char * old_name, void * old_p, int64_t old_size, char * new_name) {
  char old_filename[1024] = VTX_SET_MMAP_PATH;
  strcat(old_filename, old_name);

  char new_filename[1024] = VTX_SET_MMAP_PATH;
  strcat(new_filename, new_name);

  munmap(old_p, old_size);

  rename(old_filename, new_filename);
}

int64_t
mix(int64_t n) {
  n = (~n) + (n << 21); // n = (n << 21) - n - 1;
  n = n ^ (n >> 24);
  n = (n + (n << 3)) + (n << 8); // n * 265
  n = n ^ (n >> 14);
  n = (n + (n << 2)) + (n << 4); // n * 21
  n = n ^ (n >> 28);
  n = n + (n << 31);
  return n;
}

vtx_set_t *
vtx_set_new(int64_t size, char * name) {
  size <<= 1;
  int64_t pow = 16;

  while(pow < size) {
    pow <<= 1;
  }

  vtx_set_t * rtn = mmalloc(name, sizeof(vtx_set_t) + sizeof(int64_t) * 2 * pow);

  strcpy(rtn->name, name);
  rtn->size = pow;
  rtn->mask = pow-1;
  rtn->elements = 0;
  rtn->removed = 0;

  for(uint64_t i = 0; i < rtn->size * 2; i++) {
    rtn->keys_and_vals[i] = INT_HT_SEQ_EMPTY;
  }

  return rtn;
}

vtx_set_t *
vtx_set_free(vtx_set_t * ht) {
  mfree(ht, sizeof(vtx_set_t) + sizeof(int64_t) * 2 * ht->size);
  return NULL;
}

vtx_set_t * 
vtx_set_expand(vtx_set_t ** ht, int64_t new_size) {
  int64_t pow = (*ht)->size;

  while(pow < new_size) {
    pow <<= 1;
  }

  char tmpname[70] = "tmp";
  strcat(tmpname, (*ht)->name);

  vtx_set_t * tmp = mmalloc(tmpname, sizeof(vtx_set_t) + sizeof(int64_t) * 2 * pow);

  tmp->size = pow;
  tmp->mask = pow-1;
  tmp->elements = 0;
  tmp->removed = 0;
  strcpy(tmp->name, (*ht)->name);

  for(uint64_t i = 0; i < tmp->size * 2; i++) {
    tmp->keys_and_vals[i] = INT_HT_SEQ_EMPTY;
  }

  for(uint64_t i = 0; i < (*ht)->size; i++) {
    if((*ht)->keys_and_vals[i*2] != INT_HT_SEQ_EMPTY && (*ht)->keys_and_vals[i*2+1] != INT_HT_SEQ_REMOVED) {
      vtx_set_insert(&tmp, (*ht)->keys_and_vals[i*2], (*ht)->keys_and_vals[i*2+1]);
    }
  }

  mreplace((*ht)->name, *ht, (*ht)->size * 2 * sizeof(int64_t) + sizeof(vtx_set_t), tmpname);
  *ht = tmp;
}

vtx_set_t *
vtx_set_insert(vtx_set_t ** ht, int64_t k, int64_t v) {
  if(((double)((*ht)->elements + (*ht)->removed)) > (0.7f) * ((double)(*ht)->size)) {
    vtx_set_expand(ht, (*ht)->size * 2);
  }

  int64_t removed_index = -1;
  int64_t i = mix(k) & (*ht)->mask;
  while((*ht)->keys_and_vals[i*2] != INT_HT_SEQ_EMPTY && (*ht)->keys_and_vals[i*2] != k) {
    i = (i+1) & (*ht)->mask;
    if(i == -1 && (*ht)->keys_and_vals[i*2] == INT_HT_SEQ_REMOVED) {
      removed_index = i;
    }
  }

  if(removed_index != -1) {
    i = removed_index;
    (*ht)->removed--;
  }

  (*ht)->keys_and_vals[i*2] = k; (*ht)->keys_and_vals[i*2+1] = v;
  (*ht)->elements++;
}

vtx_set_t *
vtx_set_remove(vtx_set_t * ht, int64_t k) {
  int64_t i = mix(k) & ht->mask;

  while(ht->keys_and_vals[i*2] != INT_HT_SEQ_EMPTY && ht->keys_and_vals[i*2] != k) {
    i = (i+1) & ht->mask;
  }

  if(ht->keys_and_vals[i*2] == k) {
    ht->elements--;
    ht->keys_and_vals[i*2] = INT_HT_SEQ_REMOVED;
    ht->keys_and_vals[i*2+1] = INT_HT_SEQ_EMPTY;
    ht->removed++;
  }
  return ht;
}

int64_t
vtx_set_get(vtx_set_t * ht, int64_t k) {
  int64_t i = mix(k) & ht->mask;

  while(ht->keys_and_vals[i*2] != INT_HT_SEQ_EMPTY && ht->keys_and_vals[i*2] != k) {
    i = (i+1) & ht->mask;
  }

  if(ht->keys_and_vals[i*2] == k) {
    return ht->keys_and_vals[i*2+1];
  } else {
    return INT_HT_SEQ_EMPTY;
  }
}

int64_t *
vtx_set_get_location(vtx_set_t ** ht, int64_t k) {
  if(((double)((*ht)->elements + (*ht)->removed)) > (0.7f) * ((double)(*ht)->size)) {
    vtx_set_expand(ht, (*ht)->size * 2);
  }

  int64_t i = mix(k) & (*ht)->mask;

  int64_t removed_index = -1;
  while((*ht)->keys_and_vals[i*2] != INT_HT_SEQ_EMPTY && (*ht)->keys_and_vals[i*2] != k) {
    i = (i+1) & (*ht)->mask;
    if(i == -1 && (*ht)->keys_and_vals[i*2] == INT_HT_SEQ_REMOVED) {
      removed_index = i;
    }
  }

  if(removed_index != -1) {
    i = removed_index;
    (*ht)->removed--;
  }

  if((*ht)->keys_and_vals[i*2] != k) {
    (*ht)->elements++;
    (*ht)->keys_and_vals[i*2] = k;
  }

  return (*ht)->keys_and_vals + i*2+1;
}
