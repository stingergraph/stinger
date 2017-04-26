#ifndef STINGER_STATIC_COMPONENTS_H_
#define STINGER_STATIC_COMPONENTS_H_

#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"

int64_t parallel_shiloach_vishkin_components (struct stinger * S, int64_t nv, int64_t * component_map);

#endif
