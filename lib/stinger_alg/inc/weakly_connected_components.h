#ifndef STINGER_WEAKLY_CONNECTED_COMPONENTS_H_
#define STINGER_WEAKLY_CONNECTED_COMPONENTS_H_
#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"

int64_t parallel_shiloach_vishkin_components_of_type (struct stinger * S,  int64_t * component_map, int64_t type);
int64_t compute_component_sizes (struct stinger * S,  int64_t * component_map, int64_t * component_size);


#endif
