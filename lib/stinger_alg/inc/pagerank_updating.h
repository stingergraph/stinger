#if !defined(PAGERANK_UPDATING_HEADER_)
#define PAGERANK_UPDATING_HEADER_

int pagerank (const int64_t nv, struct stinger * S, double * x_in, const double * restrict v,
              double * residual_in,
              const double alpha, const int maxiter,
              double * workspace);

int pagerank_restart (const int64_t nv, struct stinger * S, double * x_in, const double * restrict v,
                      double * residual_in,
                      const double alpha, const int maxiter,
                      double * workspace);

void pagerank_dpr_pre (const int64_t nv, struct stinger * S,
                       int64_t * b_deg, int64_t * b_idx, double * b_val,
                       const int64_t x_deg, int64_t * x_idx, double * x_val,
                       const double * pr_val, int64_t * mark, double * dzero_workspace,
                       int64_t * pr_vol);

int pagerank_dpr (const int64_t nv, struct stinger * S,
                  int64_t * x_deg, int64_t * x_idx, double * x_val,
                  const double alpha, const int maxiter,
                  int64_t * b_deg, int64_t * b_idx, double * b_val,
                  int64_t * dpr_deg_in, int64_t * dpr_idx, double * dpr_val,
                  const double * residual_in,
                  const double * old_pr_in,
                  int64_t * mark,
                  int64_t * iworkspace,
                  double * dworkspace,
                  double * dzero_workspace,
                  int64_t * total_vol_out);

int pagerank_dpr_held (const int64_t nv, struct stinger * S,
                       int64_t * x_deg, int64_t * x_idx, double * x_val,
                       const double alpha, const int maxiter,
                       int64_t * b_deg, int64_t * b_idx, double * b_val,
                       int64_t * dpr_deg_in, int64_t * dpr_idx, double * dpr_val,
                       const double * residual_in,
                       const double * old_pr_in,
                       int64_t * mark,
                       int64_t * iworkspace,
                       double * dworkspace,
                       double * dzero_workspace,
                       int64_t * total_vol_out,
                       const double holdscale);

int pers_pagerank (const int64_t nv, struct stinger * S,
                   int64_t * x_deg_in, int64_t * x_idx_in, double * x_val_in,
                   const double alpha, const int maxiter,
                   const int64_t v_deg, const int64_t * v_idx_in, const double * v_val_in,
                   int64_t * res_deg_in, int64_t * res_idx_in, double * res_val_in,
                   int64_t * mark,
                   double * dzero_workspace,
                   int64_t * total_vol_out);

int limited_pers_pagerank (const int64_t nv, struct stinger * S,
                           int64_t * x_deg_in, int64_t * x_idx_in, double * x_val_in,
                           const double alpha, const int maxiter, const int64_t nv_limit,
                           const int64_t v_deg, const int64_t * v_idx_in, const double * v_val_in,
                           int64_t * res_deg_in, int64_t * res_idx_in, double * res_val_in,
                           int64_t * mark,
                           double * dzero_workspace,
                           int64_t * total_vol_out);

#endif /* PAGERANK_UPDATING_HEADER_ */
