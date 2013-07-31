#if !defined (TIMER_H_)
#define TIMER_H_

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

void init_timer (void);
double timer_getres (void);
void tic (void);
double toc (void);
void stats_tic (char *);
void stats_toc (void);
void print_stats ();

#if defined(__MTA__)
struct stats {
  char statname[257];
  int64_t clock, issues, concurrency, load, store, ifa;
};

struct stats stats_tic_data, stats_toc_data;
#endif

#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* TIMER_H_ */
