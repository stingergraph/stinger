#ifndef _SERVER_H
#define _SERVER_H

#include "proto/stinger-batch.pb.h"
#include "send_rcv.h"

static inline char
ascii_tolower (char x)
{
  if (x <= 'Z' && x >= 'A')
    return x - ('Z'-'z');
  return x;
}

template <class T>
void
src_string (const T& in, std::string& out)
{
  out = in.source_str();
  std::transform(out.begin(), out.end(), out.begin(), ascii_tolower);
}
template <class T>
void
dest_string (const T& in, std::string& out)
{
  out = in.destination_str();
  std::transform(out.begin(), out.end(), out.begin(), ascii_tolower);
}

static bool dropped_vertices = false;

//struct community_state cstate;
static int64_t n_components, n_nonsingleton_components, max_compsize;
static int64_t min_batch_ts, max_batch_ts;
static double processing_time, spdup;
static int64_t * comp_vlist;
static int64_t * comp_mark;

MTA("mta inline")
MTA("mta expect parallel")
static inline int64_t
int64_fetch_add (int64_t * p, int64_t incr)
{
#if defined(__MTA__)
  return int_fetch_add (p, incr);
#elif defined(_OPENMP)
#if defined(__GNUC__)
  return __sync_fetch_and_add (p, incr);
#elif defined(__INTEL_COMPILER)
  return __sync_fetch_and_add (p, incr);
#else
#error "Atomics not defined..."
#endif
#else
  int64_t out = *p;
  *p += incr;
  return out;
#endif
}

int
process_batch(stinger_t * S, StingerBatch & batch,
	      struct community_state * cstate);
void
components_init(struct stinger * S, int64_t nv, int64_t * component_map);

void
components_batch(struct stinger * S, int64_t nv, int64_t * component_map);

#define V_A(X,...) do { fprintf(stdout, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__); fflush (stdout); } while (0)
#define V(X) V_A(X,NULL)

static void
split_day_in_year (int diy, int * month, int * dom)
{
  int mn = 0;
#define OUT do { *month = mn; *dom = diy; return; } while (0)
#define MONTH(d_) if (diy < (d_)) OUT; diy -= (d_); ++mn
  MONTH(31);
  MONTH(28); 
  MONTH(31);
  MONTH(30);
  MONTH(31);
  MONTH(30);
  MONTH(31);
  MONTH(31);
  MONTH(30);
  MONTH(31);
  MONTH(30);
  MONTH(31);
  assert (0);
  *month = -1;
  *dom = -1;
}

static const char *
month_name[12] = { "Jan", "Feb", "Mar", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static void
ts_to_str (int64_t ts_in, char * out, size_t len)
{
  int64_t ts = ts_in;
  ts += (31 + 28)*24*3600; /* XXX: Sample starts in March. */
  ts -= 7*3600; /* XXX: Pacific is seven hours behind UTC. */
  const bool pst =
    (ts < (2*3600 + (9 + 31+28)*24*3600)) &&
    (ts < (2*3600 + (3 + 31+28+31+30+31+30+31+31+30+31)*24*3600))
	 ; /* 2am 10 Mar to 2am 3 Nov */
  if (pst) ts += 3600;
  const int sec = ts%60;
  const int min = (ts/60)%60;
  const int hr = (ts/(60*60))%24;
  const int day_in_year = ts_in / (60*60*24);
  int month, dom;
  split_day_in_year (day_in_year, &month, &dom);
  snprintf (out, len, "%2d:%02d:%02d P%cT %d %s 2013",
	    hr, min, sec,
	    (pst? 'S' : 'D'),
	    dom, month_name[month]);
}

#endif  /* _SERVER_H */
