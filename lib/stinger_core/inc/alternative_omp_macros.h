#if !defined(ALTERNATIVE_OMP_MACROS_H_)
#define ALTERNATIVE_OMP_MACROS_H_

/*
 * Including this file changes the way the OMP macro works.
 * This version of the macro accepts tokens instead of a string,
 * and makes the "omp" token implicit. This allows more flexibility
 * in composing the OpenMP pragma with other macros.
 *
 * Maintaining two different incompatible OMP macro definitions
 * is not a good long term plan. This header is just a temporary
 * hack to make integration easier.
 */

#if defined(_OPENMP)

    // Remove old definitions from stinger_defs.h
    #if defined(OMP)
        #undef OMP
    #endif
    #if defined(OMP_SIMD)
        #undef OMP_SIMD
    #endif

    // Two-stage macro: prepend omp token, then stringify tokens and emit pragma
    #define OMP_(x) _Pragma(#x)
    #define OMP(x) OMP_(omp x)
    // Enable simd token if OpenMP supports it
    #if _OPENMP >= 201307
        #define OMP_SIMD simd
    #else
        #define OMP_SIMD
    #endif

#endif /* if defined(_OPENMP) */

#endif /* ALTERNATIVE_OMP_MACROS_H_ */