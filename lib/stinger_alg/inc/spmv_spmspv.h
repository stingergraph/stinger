#if !defined(SPMV_SPMSPV_HEADER_)
#define SPMV_SPMSPV_HEADER_

/*
  Operation:
  - y = alpha * A^T * x + beta * y

  Variants:
  - y dense, sparse
  - x dense, sparse
  - unit, edge weight  
*/

#include "stinger_core/stinger.h"

#ifdef __cplusplus
extern "C" {
#endif

void
stinger_dspmTv (const int64_t nv,
                const double alpha, const struct stinger *S, const double * x,
                const double beta, double * y);

void
stinger_unit_dspmTv (const int64_t nv,
                     const double alpha, const struct stinger *S, const double * x,
                     const double beta, double * y);

void
stinger_dspmTv_degscaled (const int64_t nv,
                          const double alpha, const struct stinger *S, const double * x,
                          const double beta, double * y);

void
stinger_unit_dspmTv_degscaled (const int64_t nv,
                               const double alpha, const struct stinger *S, const double * x,
                               const double beta, double * y);

void
stinger_dspmTspv (const int64_t nv,
                  const double alpha, const struct stinger *S,
                  const int64_t x_deg, const int64_t * x_idx, const double * x_val,
                  const double beta,
                  int64_t * y_deg, int64_t * y_idx, double * y_val,
                  int64_t * loc_ws, double * val_ws);

void
stinger_unit_dspmTspv (const int64_t nv,
                       const double alpha, const struct stinger *S,
                       const int64_t x_deg, const int64_t * x_idx, const double * x_val,
                       const double beta,
                       int64_t * y_deg, int64_t * y_idx, double * y_val,
                       int64_t * loc_ws, double * val_ws);

void
stinger_dspmTspv_degscaled (const int64_t nv,
                            const double alpha, const struct stinger *S,
                            const int64_t x_deg, const int64_t * x_idx, const double * x_val,
                            const double beta,
                            int64_t * y_deg, int64_t * y_idx, double * y_val,
                            int64_t * loc_ws, double * val_ws);

void
stinger_unit_dspmTspv_degscaled (const int64_t nv,
                                 const double alpha, const struct stinger *S,
                                 const int64_t x_deg, const int64_t * x_idx, const double * x_val,
                                 const double beta,
                                 int64_t * y_deg, int64_t * y_idx, double * y_val,
                                 int64_t * loc_ws, double * val_ws, int64_t *);

void
stinger_unit_dspmTspv_degscaled_held (const double holdthresh, const int64_t nv,
                                      const double alpha, const struct stinger *S,
                                      const int64_t x_deg, const int64_t * x_idx, const double * x_val,
                                      const double beta,
                                      int64_t * y_deg, int64_t * y_idx, double * y_val,
                                      int64_t * loc_ws, double * val_ws, int64_t *);

#ifdef __cplusplus
}
#endif
     
#endif /* SPMV_SPMSPV_HEADER_ */
