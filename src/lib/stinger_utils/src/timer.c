#define _XOPEN_SOURCE 600
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
//#include "stinger-defs.h"
#include "timer.h"

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef __MTA__

#include <sys/mta_task.h>
#include <machine/runtime.h>

void
init_timer (void)
{
  /* Empty. */
}

double
timer (void)
{
  return ((double) mta_get_clock (0) / mta_clock_freq ());
}

double
timer_getres (void)
{
  /* A guess. */
  return 1.0 / mta_clock_freq ();
}


#elif defined(_OPENMP)

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

/* Cray XMT Performance Counter Support */
#if defined(__MTA__)
static int64_t load_ctr = -1, store_ctr, ifa_ctr;

static void
init_stats (void)
{
  load_ctr = mta_rt_reserve_task_event_counter (3, RT_LOAD);
  assert (load_ctr >= 0);
  store_ctr = mta_rt_reserve_task_event_counter (2, RT_STORE);
  assert (store_ctr >= 0);
  ifa_ctr = mta_rt_reserve_task_event_counter (1, RT_INT_FETCH_ADD);
  assert (ifa_ctr >= 0);
  memset (&stats_tic_data, 0, sizeof (stats_tic_data));
  memset (&stats_toc_data, 0, sizeof (stats_toc_data));
}

static void
get_stats (struct stats *s)
{
MTA("mta fence")
  s->clock = mta_get_task_counter(RT_CLOCK);
MTA("mta fence")
  s->issues = mta_get_task_counter(RT_ISSUES);
MTA("mta fence")
  s->concurrency = mta_get_task_counter(RT_CONCURRENCY);
MTA("mta fence")
  s->load = mta_get_task_counter(RT_LOAD);
MTA("mta fence")
  s->store = mta_get_task_counter(RT_STORE);
MTA("mta fence")
  s->ifa = mta_get_task_counter(RT_INT_FETCH_ADD);
MTA("mta fence")
}

void
stats_tic (char *statname)
{
  if (load_ctr < 0)
    init_stats ();
  memset (&stats_tic_data.statname, 0, sizeof (stats_tic_data.statname));
  strncpy (&stats_tic_data.statname, statname, 256);
  get_stats (&stats_tic_data);
}

void
stats_toc (void)
{
  get_stats (&stats_toc_data);
}

void
print_stats (void)
{
  if (load_ctr < 0) return;
  printf ("alg : %s\n", stats_tic_data.statname);
#define PRINT(v) do { printf (#v " : %ld\n", (stats_toc_data.v - stats_tic_data.v)); } while (0)
  PRINT (clock);
  PRINT (issues);
  PRINT (concurrency);
  PRINT (load);
  PRINT (store);
  PRINT (ifa);
#undef PRINT
  printf ("endalg : 1\n");
}

#else

/**
* @brief Start recording performance statistics from hardware counters
*
* @param c String describing statistics to measure
*/
void
stats_tic (char *c /*UNUSED*/)
{
}

/**
* @brief End recording performance statistics
*/
void
stats_toc (void)
{
}

/**
* @brief Print out performance counters to stdout
*/
void
print_stats (void)
{
}
#endif
