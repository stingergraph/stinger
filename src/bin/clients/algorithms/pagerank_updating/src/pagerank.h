#if !defined(PAGERANK_HEADER_)
#define PAGERANK_HEADER_

int pagerank (const int64_t nv, struct stinger * S, double * x_in, const double * restrict v,
              const double alpha, const int maxiter,
              double * workspace);

int pagerank_restart (const int64_t nv, struct stinger * S, double * x_in, const double * restrict v,
                      const double alpha, const int maxiter,
                      double * workspace);

int pagerank_dpr (const int64_t nv, struct stinger * S,
                  int64_t * x_deg, int64_t * x_idx, double * x_val,
                  const double alpha, const int maxiter,
                  int64_t * b_deg, int64_t * b_idx, double * b_val,
                  const double norm1_pr,
                  int64_t * dpr_deg_in, int64_t * dpr_idx, double * dpr_val,
                  int64_t * mark,
                  int64_t * iworkspace,
                  double * dworkspace,
                  double * dzero_workspace,
                  int64_t * total_vol_out);

int pagerank_dpr_held (const int64_t nv, struct stinger * S,
                       int64_t * x_deg, int64_t * x_idx, double * x_val,
                       const double alpha, const int maxiter,
                       int64_t * b_deg, int64_t * b_idx, double * b_val,
                       const double norm1_pr,
                       int64_t * dpr_deg_in, int64_t * dpr_idx, double * dpr_val,
                       int64_t * mark,
                       int64_t * iworkspace,
                       double * dworkspace,
                       double * dzero_workspace,
                       int64_t * total_vol_out);

#endif /* PAGERANK_HEADER_ */
