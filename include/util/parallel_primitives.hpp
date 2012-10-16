#ifndef  PARALLEL_PRIMITIVES_HPP
#define  PARALLEL_PRIMITIVES_HPP

#include <stdint.h>
#include "util/parallel_macros.hpp"

namespace Stinger {
  namespace Util {
    template<typename type_t>
    type_t prefixSum(type_t * array, size_t count);

    template<typename type_t>
    type_t reduceSum(type_t * array, size_t count);

    template<typename type_t>
    void fill(type_t * array, size_t count, type_t value);

    template<typename type_t>
    void sequence(type_t * array, size_t count, type_t start = 0);

    template<typename type_t>
    void copy(type_t * source, size_t count, type_t * destination);
  }
}

template<typename type_t>
inline type_t Stinger::Util::prefixSum(type_t * array, size_t count) {
  static type_t *buf;

  int nt, tid;
  int64_t slice_begin, slice_end, t1, k, tmp;

  nt = omp_get_num_threads ();
  tid = omp_get_thread_num ();

  OMP("omp master")
    buf = (type_t *)alloca (nt * sizeof (type_t));
  OMP("omp flush (buf)");
  OMP("omp barrier");

  slice_begin = (tid * count) / nt;
  slice_end = ((tid + 1) * count) / nt; 

  /* compute sums of slices */
  tmp = 0;
  for (k = slice_begin; k < slice_end; ++k)
    tmp += array[k];
  buf[tid] = tmp;

  /* prefix sum slice sums */
  OMP("omp barrier");
  OMP("omp single")
    for (k = 1; k < nt; ++k)
      buf[k] += buf[k-1];
  OMP("omp barrier");

  /* get slice sum */
  if (tid)
    t1 = buf[tid-1];
  else
    t1 = 0;

  /* prefix sum our slice including previous slice sums */
  array[slice_begin] += t1;
  for (k = slice_begin + 1; k < slice_end; ++k) {
    array[k] += array[k-1];
  }
  OMP("omp barrier");

  return array[count-1];
}

template<typename type_t>
inline type_t Stinger::Util::reduceSum(type_t * array, size_t count) {
  type_t sum = 0;
  OMP("omp parallel for reduction(+:sum)")
  for(uint64_t i = 0; i < count; i++) {
    sum += array[i];
  }
  return sum;
}

template<typename type_t>
inline void Stinger::Util::fill(type_t * array, size_t count, type_t value) {
  OMP("#pragma omp parallel for")
  for(uint64_t i = 0; i < count; i++) {
    array[i] = value;
  }
}

template<typename type_t>
inline void Stinger::Util::sequence(type_t * array, size_t count, type_t start = 0) {
  OMP("#pragma omp parallel for")
  for(uint64_t i = 0; i < count; i++) {
    array[i] = (type_t)i + start;
  }
}

template<typename type_t>
inline void Stinger::Util::copy(type_t * source, size_t count, type_t * destination) {
  OMP("omp parallel for")
  for(uint64_t i = 0; i < count; i++) {
    destination[i] = source[i];
  }
}

#endif  /*PARALLEL_PRIMITIVES_HPP*/
