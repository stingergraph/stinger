#ifndef  STINGER_LA_H
#define  STINGER_LA_H

#include "stinger_core/stinger.h"

typedef int64_t eweight_t; 

eweight_t
stinger_add(eweight_t a, eweight_t b);

eweight_t
stinger_mult(eweight_t a, eweight_t b);

eweight_t
stinger_max(eweight_t a, eweight_t b);

eweight_t
stinger_min(eweight_t a, eweight_t b);

eweight_t
stinger_and(eweight_t a, eweight_t b);

eweight_t
stinger_or(eweight_t a, eweight_t b);

int
stinger_matvec(stinger_t * S, eweight_t * vec_in, int64_t nv, eweight_t * vec_out, 
  eweight_t (*multiply)(eweight_t, eweight_t), eweight_t (*reduce)(eweight_t, eweight_t), 
  eweight_t empty);


#endif  /*STINGER_LA_H*/
