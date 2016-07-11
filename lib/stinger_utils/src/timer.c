#define _XOPEN_SOURCE 600
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "timer.h"

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

#if defined(_OPENMP)

  void
  init_timer (void)
  {
    /* Empty. */
  }

  double
  timer (void)
  {
    return omp_get_wtime ();
  }

  double
  timer_getres (void)
  {
    return omp_get_wtick ();
  }

#elif defined(__APPLE__)

  void
  init_timer (void)
  {
    /* Empty. */
  }

  double
  timer (void)
  {
    UnsignedWide uw = AbsoluteToNanoseconds(UpTime());
    double nano = ((((unsigned long long)uw.hi)<<32)|(uw.lo));
    return (nano / (1e9));
  }

  double
  timer_getres (void)
  {
    return 1e-9;
  }

#else /* Elsewhere */

  static clockid_t clockid;

  #if defined(CLOCK_REALTIME_ID)
  #define CLKID CLOCK_REALTIME_ID
  #define CLKIDNAME "CLOCK_REALTIME_ID"
  #elif defined(CLOCK_THREAD_CPUTIME_ID)
  #define CLKID CLOCK_THREAD_CPUTIME_ID
  #define CLKIDNAME "CLOCK_THREAD_CPUTIME_ID"
  #elif defined(CLOCK_REALTIME_ID)
  #warning "Falling back to realtime clock."
  #define CLKID CLOCK_REALTIME_ID
  #define CLKIDNAME "CLOCK_REALTIME_ID"
  #else
  #error "Cannot find a clock!"
  #endif

  /**
  * @brief Initialize the system timer
  */
  void
  init_timer (void)
  {
    int err;
    err = clock_getcpuclockid (0, &clockid);
    if (err >= 0)
      return;
    fprintf (stderr, "Unable to find CPU clock, falling back to "
	     CLKIDNAME "\n");
    clockid = CLKID;
  }

  double
  timer (void)
  {
    struct timespec tp;
    clock_gettime (clockid, &tp);
    return (double) tp.tv_sec + 1.0e-9 * (double) tp.tv_nsec;
  }

  /**
  * @brief Get the resolution of the system clock
  *
  * @return Clock Resolution
  */
  double
  timer_getres (void)
  {
    struct timespec tp;
    clock_getres (clockid, &tp);
    return (double) tp.tv_sec + 1.0e-9 * (double) tp.tv_nsec;
  }

#endif

static double last_tic = -1.0;

/**
* @brief Start the timer
*/
void
tic (void)
{
  last_tic = timer ();
}

/**
* @brief Stop the timer and return the time taken
*
* @return Time since last tic()
*/
double
toc (void)
{
  const double t = timer ();
  const double out = t - last_tic;
  last_tic = t;
  return out;
}
