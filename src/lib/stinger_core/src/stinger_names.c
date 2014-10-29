#include "xmalloc.h"
#include "x86_full_empty.h"
#include "stinger_names.h"
#include "stinger_error.h"

#include <stdint.h>
#include <string.h>

/**
* @file stinger-names.c
* @brief A simple hash structure to map strings to integers (permanently). Parallel safe.
* @author Rob McColl
*
* This structure uses one contiguous storage space and indices within that space
* such that it contains no pointers and makes only one contiguous allocation.
*
* Last modified: Fri May 24, 2013  03:42PM
* Created: Thu May 16, 2013  09:45AM
*/

/* NOTE: this is done so that the stinger_names_t is easier to map accross
  multiple memory spaces */

#define MAP_SN(X) \
  char * names = (char *)((X)->storage); \
  int64_t * to_name = (int64_t *)((X)->storage + (X)->to_name_start); \
  int64_t * from_name= (int64_t *)((X)->storage + (X)->from_name_start); \
  int64_t * to_int = (int64_t *)((X)->storage + (X)->to_int_start); 


static uint64_t
xor_hash(uint8_t * byte_string, int64_t length) {
  if(length < 0) length = 0;

  uint64_t out = 0;
  uint64_t * byte64 = (uint64_t *)byte_string;
  while(length > 8) {
    length -= 8;
    out ^= *(byte64);
    byte64++;
  }
  if(length > 0) {
    uint64_t last = 0;
    uint8_t * cur = (uint8_t *)&last;
    uint8_t * byte8 = (uint8_t *)byte64;
    while(length > 0) {
      length--;
      *(cur++) = *(byte8++);
    }
    out ^= last;
  }

  /* bob jenkin's 64-bit mix */
  out = (~out) + (out << 21); // out = (out << 21) - out - 1;
  out = out ^ (out >> 24);
  out = (out + (out << 3)) + (out << 8); // out * 265
  out = out ^ (out >> 14);
  out = (out + (out << 2)) + (out << 4); // out * 21
  out = out ^ (out >> 28);
  out = out + (out << 31);

  return out;
}

/**
* @brief Allocates a stinger_names_t and initialzies it.
*
* @param max_types The maximum number of types supported.
*
* @return A new stinger_names_t.
*/
stinger_names_t * 
stinger_names_new(int64_t max_types) {
  stinger_names_t * sn = xcalloc(sizeof(stinger_names_t) + 
		(max_types * (NAME_STR_MAX+1) * sizeof(char)) +  /* strings */
		(max_types * sizeof(int64_t) * 3) + /* from_name + to_name */
		(max_types * sizeof(int64_t) * 2), sizeof(uint8_t)); /* to_int */

  sn->to_name_start = max_types * (NAME_STR_MAX+1) * sizeof(char);
  sn->from_name_start = sn->to_name_start + max_types * sizeof(int64_t);
  sn->to_int_start = sn->from_name_start + max_types * sizeof(int64_t) * 2;

  sn->next_string = 1;
  sn->next_type = 0;
  sn->max_types = max_types;
  sn->max_names = max_types * (NAME_STR_MAX+1) - 1;

  return sn;
}

void
stinger_names_init(stinger_names_t * sn, int64_t max_types) {
//  stinger_names_t * sn = xcalloc(sizeof(stinger_names_t) + 
//		(max_types * (NAME_STR_MAX+1) * sizeof(char)) +  /* strings */
//		(max_types * sizeof(int64_t) * 3) + /* from_name + to_name */
//		(max_types * sizeof(int64_t) * 2), sizeof(uint8_t)); /* to_int */
//
  sn->to_name_start = max_types * (NAME_STR_MAX+1) * sizeof(char);
  sn->from_name_start = sn->to_name_start + max_types * sizeof(int64_t);
  sn->to_int_start = sn->from_name_start + max_types * sizeof(int64_t) * 2;

  sn->next_string = 1;
  sn->next_type = 0;
  sn->max_types = max_types;
  sn->max_names = max_types * (NAME_STR_MAX+1) - 1;

  return;
}

size_t
stinger_names_size(int64_t max_types) {
  size_t rtn = sizeof(stinger_names_t) + 
		(max_types * (NAME_STR_MAX+1) * sizeof(char)) +  /* strings */
		(max_types * sizeof(int64_t) * 3) + /* from_name + to_name */
		(max_types * sizeof(int64_t) * 2); /* to_int */
  return rtn;
}

/**
* @brief Free the stinger_names_t and set the pointer to NULL.
*
* @param sn A pointer to a stinger_names struct
*
* @return NULL
*/
stinger_names_t *
stinger_names_free(stinger_names_t ** sn) {
  if(sn && *sn) {
    free(*sn);
    *sn = NULL;
  }
  return NULL;
}

/**
* @brief Lookup a type mapping or create new mapping (if one did not previously exist). 
*
* @param sn A pointer to a stinger_names struct
* @param name The string name of the type you wish to create or lookup.
*
* @return The type on success or -1 on failure.
*/
int
stinger_names_create_type(stinger_names_t * sn, const char * name, int64_t * out) {
  MAP_SN(sn)
  int64_t length = strlen(name); length = length > NAME_STR_MAX ? NAME_STR_MAX : length;
  int64_t index = xor_hash(name, length) % (sn->max_types * 2);
  int64_t init_index = index;

  if(sn->next_type > sn->max_types) {
	*out = -1;
    return -1;
  }

  while(1) {
    if(0 == readff((uint64_t *)from_name + index)) {
      int64_t original = readfe((uint64_t *)from_name + index);
      if(original) {
	if(strncmp(name,names + original, length) == 0) {
	  writeef((uint64_t *)from_name + index, original);
	  *out = to_int[index];
	  return 0;
	}
      } else {
	int64_t place = stinger_int64_fetch_add(&(sn->next_string), length+1);
	if(place + length >= sn->max_names) {
	  /* abort(); */
	  *out = INT64_MAX;
	  return -1;
	}
	strncpy(names + place, name, length);
	to_int[index] = stinger_int64_fetch_add(&(sn->next_type), 1);
	to_name[to_int[index]] = place;
	writeef((uint64_t *)from_name + index, (uint64_t)place);
	*out = to_int[index];
	return 1;
      }
      writeef((uint64_t *)&from_name + index, original);
    } else if(strncmp(name, names + readff((uint64_t *)from_name + index), length) == 0) {
      *out = to_int[index];
      return 0;
    }
    index++;
    index = index % (sn->max_types * 2); 
    if(index == init_index) 
      return -1;
  }
}

/**
* @brief Lookup a type mapping. 
*
* @param sn A pointer to a stinger_names struct
* @param name The string name of the type you wish to lookup.
*
* @return The type on success or -1 if the type does not exist.
*/
int64_t
stinger_names_lookup_type(stinger_names_t * sn, const char * name) {
  MAP_SN(sn)
  int64_t length = strlen(name); length = length > NAME_STR_MAX ? NAME_STR_MAX : length;
  int64_t index = xor_hash(name, length) % (sn->max_types * 2);
  int64_t init_index = index;

  while(1) {
    if(0 == readff((uint64_t *)from_name + index)) {
      return -1;
    } else if(strncmp(name, names + readff((uint64_t *)from_name + index), length) == 0) {
      return to_int[index];
    }
    index++;
    index = index % (sn->max_types * 2); 
    if(index == init_index) 
      return -1;
  }
}

/**
* @brief Lookup the corresponding string for a given type
*
* @param sn A pointer to a stinger_names struct
* @param type The integral type you want to look up.
*
* @return Returns string if the mapping exists or NULL.
*/
char *
stinger_names_lookup_name(stinger_names_t * sn, int64_t type) {
  MAP_SN(sn)
  if(type < sn->max_types) {
    return names + to_name[type];
  } else {
    return NULL;
  }
}

int64_t
stinger_names_count(stinger_names_t * sn) {
  return sn->next_type;
}

void
stinger_names_print(stinger_names_t * sn) {
  MAP_SN(sn)

  for(int64_t i = 0; i < sn->max_types*2; i++) {
    printf("FROM_NAME %ld %s TO_INT %ld TO_NAME %ld\n", from_name[i], from_name[i] ? names + from_name[i] : "", to_int[i], to_int[i] ? to_name[to_int[i]] : 0);
  }
}

/**
* @brief Save the strings stored in this names_t to a binary file.
*
* Format: [int64_t NAME_STR_MAX] [int64_t count(names)]
*	  [int64_t len_0] [chars string_0]
*	  [int64_t len_1] [chars string_1] 
*	  ... [chars string_count(names)-1]
*
* Strings in the file are not NULL terminated.
*
* @param sn The names_t to be written.
* @param fp The file to write into.
*/
void
stinger_names_save(stinger_names_t * sn, FILE * fp) {
  MAP_SN(sn)

  int64_t max_len = NAME_STR_MAX;
  fwrite(&max_len, sizeof(int64_t), 1, fp);
  fwrite(&(sn->next_type), sizeof(int64_t), 1, fp);

  for(int64_t i = 0; i < sn->next_type; i++) {
    int64_t len = strlen(names + to_name[i]);
    fwrite(&len, sizeof(int64_t), 1, fp);
    fwrite(names + to_name[i], sizeof(char), len, fp);
  }
}

/**
* @brief Load strings from a binary file into a newly created names_t.
*
* Format follows stinger_names_save()
*
* @param sn The names_t to write into.
* @param fp The file containing the data.
*/
void
stinger_names_load(stinger_names_t * sn, FILE * fp) {
  int64_t max_len = NAME_STR_MAX;
  fread(&max_len, sizeof(int64_t), 1, fp);
  
  if(max_len > NAME_STR_MAX) {
    LOG_W_A("Max names length in file (%ld) is greater than the library supports (%ld)."
    " Names will be truncated\n", (long) max_len, (long) NAME_STR_MAX);
  }

  int64_t in_file = 0;
  fread(&(in_file), sizeof(int64_t), 1, fp);

  char * cur_name = xcalloc(sizeof(char), max_len);

  for(int64_t i = 0; i < in_file; i++) {
    int64_t len = 0;
    fread(&len, sizeof(int64_t), 1, fp);
    fread(cur_name, sizeof(char), len, fp);
    cur_name[len] = '\0';

    int64_t out = 0;
    stinger_names_create_type(sn, cur_name, &out);

    if(out != i) {
      LOG_W_A("Mapping does not match expected when loading file (%ld != %ld).  Was the names_t "
	"not empty?", (long) i, (long) out);
    }
  }

  free(cur_name);
}

#if defined(STINGER_stinger_names_TEST)
#include <stdio.h>

int main(int argc, char *argv[]) {
  stinger_names_t * sn = stinger_names_new(12);

  int64_t t = stinger_names_create_type(sn, "This is a test");
  if(t < 0) {
    printf("Failed to create!\n");
    stinger_names_print(sn);
    return -1;
  }

  if(t != stinger_names_create_type(sn, "This is a test")) {
    printf("Failed when creating duplicate!\n");
    stinger_names_print(sn);
    return -1;
  }

  if(t != stinger_names_lookup_type(sn, "This is a test")) {
    printf("Failed when looking up existing!\n");
    stinger_names_print(sn);
    return -1;
  }

  if(t+1 != stinger_names_create_type(sn, "This is a test 2")) {
    printf("Failed when creating new type!\n");
    stinger_names_print(sn);
    return -1;
  }

  if(-1 != stinger_names_lookup_type(sn, "This wrong")) {
    printf("Failed when looking up non-existant!\n");
    stinger_names_print(sn);
    return -1;
  }

  if(strcmp("This is a test", stinger_names_lookup_name(sn, t))) {
    printf("Failed when looking up name!\n");
    stinger_names_print(sn);
    return -1;
  }

  stinger_names_free(&sn);
  return 0;
}
#endif

