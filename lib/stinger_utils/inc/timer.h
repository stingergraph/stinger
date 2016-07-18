#if !defined (TIMER_H_)
#define TIMER_H_

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

void init_timer (void);
double timer (void);
double timer_getres (void);
void tic (void);
double toc (void);
void stats_tic (char *);
void stats_toc (void);
void print_stats ();

#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* TIMER_H_ */
