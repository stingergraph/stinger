/* Emulation of Full Empty Bits Using atomic check-and-swap.
 *
 * NOTES:
 * - Using these functions means that the MARKER value defined
 *   below must be reserved in your application and CANNOT be
 *   considered a normal value.  Feel free to change the value to
 *   suit your application.
 * - Improper use of these functions can and will result in deadlock.
 *
 * author: rmccoll3@gatech.edu
 */

#include  "x86_full_empty.h"

uint64_t
readfe(volatile uint64_t * v) {
  stinger_memory_barrier();
  uint64_t val;
  while(1) {
    val = *v;
    while(val == MARKER) {
      val = *v;
    }
    if(val == stinger_int64_cas((volatile int64_t *) v, val, MARKER))
      break;
  }
  return val;
}

uint64_t
writeef(volatile uint64_t * v, uint64_t new_val) {
  stinger_memory_barrier();
  uint64_t val;
  while(1) {
    val = *v;
    while(val != MARKER) {
      val = *v;
    }
    if(MARKER == stinger_int64_cas((volatile int64_t *) v, MARKER, new_val))
      break;
  }
  return val;
}

uint64_t
readff(volatile const uint64_t * v) {
  stinger_memory_barrier();
  uint64_t val = *v;
  while(val == MARKER) {
    val = *v;
  }
  return val;
}

uint64_t
writeff(volatile uint64_t * v, uint64_t new_val) {
  stinger_memory_barrier();
  uint64_t val;
  while(1) {
    val = *v;
    while(val == MARKER) {
      val = *v;
    }
    if(val == stinger_int64_cas((volatile int64_t *) v, val, new_val))
      break;
  }
  return val;
}

uint64_t
writexf(volatile uint64_t * v, uint64_t new_val) {
  stinger_memory_barrier();
  *v = new_val;
  stinger_memory_barrier();
  return new_val;
}
