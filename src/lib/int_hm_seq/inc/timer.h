#if !defined (TIMER_H_)
#define TIMER_H_

void init_timer (void);
double timer_getres (void);
void tic (void);
double toc (void);
void stats_tic (char *);
void stats_toc (void);
void print_stats ();

struct stats {
#if defined(__MTA__)
  char statname[257];
  int64_t clock, issues, concurrency, load, store, ifa;
#else
#endif
};

struct stats stats_tic_data, stats_toc_data;

#endif /* TIMER_H_ */
