#ifndef STINGER_ADAMIC_ADAR_H
#define STINGER_ADAMIC_ADAR_H

#include "stinger_core/stinger_atomics.h"
#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

int64_t adamic_adar(stinger_t * S, int64_t source, int64_t etype, int64_t ** candidates, double ** scores);

#ifdef __cplusplus
}
#endif

#endif
