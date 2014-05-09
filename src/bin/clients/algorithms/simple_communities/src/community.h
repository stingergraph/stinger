#if !defined(COMMUNITY_HEADER_)
#define COMMUNITY_HEADER_

#define ALG_HEAVIEST_EDGE 0
#define ALG_CONDUCTANCE 1
#define ALG_CNM 2
#define ALG_MB 3

#define ALG_GREEDY_PASS 0
#define ALG_GREEDY_MAXIMAL 1
#define ALG_TREE 2

extern double global_score_time, global_match_time, global_aftermatch_time,
  global_contract_time, global_other_time,
  mb_nstd;
extern int global_verbose;

#define NHIST 4
struct community_hist {
  double hist[NHIST];
  int nhist, curhist;
};

int64_t community (int64_t * c, struct el * restrict g /* destructive */,
                   const int score_alg, const int match_alg,
                   const int double_self,
                   const int decimate,
                   const int64_t maxsz, int64_t maxnum,
                   const int64_t min_degree, const int64_t max_degree,
                   int64_t max_nsteps,
                   const double covlevel, const double decrease_factor,
                   const int use_hist,
                   struct community_hist * restrict,
                   int64_t * restrict cmap_global, const int64_t nv_global,
                   int64_t * csize_out,
                   int64_t * ws, size_t wslen,
                   void*);

int64_t
update_community (int64_t * restrict cmap_global, const int64_t nv_global,
                  int64_t * restrict csize,
                  int64_t * restrict max_csize,
                  int64_t * restrict n_nonsingletons_out,

                  struct el * restrict g /* destructive */,
                  const int score_alg, const int match_alg, const int double_self,
                  const int decimate,
                  const int64_t maxsz, int64_t maxnum,
                  const int64_t min_degree, const int64_t max_degree,
                  int64_t max_nsteps,
                  const double covlevel, const double decrease_factor,
                  const int use_hist,
                  struct community_hist * restrict hist,

                  int64_t * ws, size_t wslen,
                  void * lockspace_in);

double eval_conductance_cgraph (const struct el g, int64_t * ws);
double eval_modularity_cgraph (const struct el g, int64_t * ws);
void eval_cov (const struct el g, const int64_t * restrict m,
              const int64_t orig_nv, const int64_t max_in_weight,
              const int64_t nonself_in_weight,
              double * cov_out, double * mirrorcov_out,
              int64_t * ws);

void contract_self (struct el * restrict g,
                    int64_t * restrict ws /* NV+1 + 2*NE */);

#endif /* COMMUNITY_HEADER_ */
