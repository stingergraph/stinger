#ifndef STINGER_KCORE_H_
#define STINGER_KCORE_H_

#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_utils/timer.h"

void kcore_find(stinger_t *S, int64_t * labels, int64_t * counts, int64_t nv, int64_t * k_out);

#endif
