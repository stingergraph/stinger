#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <assert.h>

#include "stinger_core/stinger_atomics.h"
#include "stinger_core/stinger.h"

#include "compat.h"

static inline void setup_y (const int64_t nv, const double beta, double * y)
{
  if (0.0 == beta) {
    for (int64_t i = 0; i < nv; ++i)
      y[i] = 0.0;
  } else if (-1.0 == beta) {
    for (int64_t i = 0; i < nv; ++i)
      y[i] = -y[i];
  } else if (!(1.0 == beta)) {
    for (int64_t i = 0; i < nv; ++i)
      y[i] *= beta;
  }
  /* if 1.0 == beta, do nothing. */
}

/* Fragile macro, do not pass unevaluated expressions as arguments. */
/* It's a macro to avoid a sequence point that could force fetching x[i]... */
#define ALPHAXI_VAL(alpha, xi) (alpha == 0.0? 0.0 : (alpha == 1.0? xi : (alpha == -1.0? -xi : (alpha * xi))))
#define DEGSCALE(axi, degi) (degi == 0? 0.0 : axi / degi);

void stinger_dspmTv (const int64_t nv, const double alpha, const struct stinger *S, const double * x, const double beta, double * y)
{
  setup_y (nv, beta, y);

  for (int64_t i = 0; i < nv; ++i) {
    const double alphaxi = ALPHAXI_VAL (alpha, x[i]);
    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
      const int64_t j = STINGER_EDGE_DEST;
      const double aij = STINGER_EDGE_WEIGHT;
      y[j] += aij * alphaxi;
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }
}

void stinger_unit_dspmTv (const int64_t nv, const double alpha, const struct stinger *S, const double * x, const double beta, double * y)
{
  setup_y (nv, beta, y);

  for (int64_t i = 0; i < nv; ++i) {
    const double alphaxi = ALPHAXI_VAL (alpha, x[i]);
    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
      const int64_t j = STINGER_EDGE_DEST;
      y[j] += alphaxi;
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }
}

void stinger_dspmTv_degscaled (const int64_t nv, const double alpha, const struct stinger *S, const double * x, const double beta, double * y)
{
  setup_y (nv, beta, y);

  for (int64_t i = 0; i < nv; ++i) {
    const double alphaxi = ALPHAXI_VAL (alpha, x[i]);
    const int64_t degi = stinger_outdegree_get (S, i);
    if (degi > 0) {
      const double alphaxi_deg = DEGSCALE (alphaxi, degi);
      STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
        const int64_t j = STINGER_EDGE_DEST;
        const double aij = STINGER_EDGE_WEIGHT;
        y[j] += aij * alphaxi_deg;
      } STINGER_FORALL_EDGES_OF_VTX_END();
    }
  }
}

void stinger_unit_dspmTv_degscaled (const int64_t nv, const double alpha, const struct stinger *S, const double * x, const double beta, double * y)
{
  setup_y (nv, beta, y);

  for (int64_t i = 0; i < nv; ++i) {
    const double alphaxi = ALPHAXI_VAL (alpha, x[i]);
    const int64_t degi = stinger_outdegree_get (S, i);
    if (degi > 0) {
      const double alphaxi_deg = DEGSCALE (alphaxi, degi);
      assert (alphaxi_deg <= alphaxi);
      STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
        const int64_t j = STINGER_EDGE_DEST;
        y[j] += alphaxi_deg;
      } STINGER_FORALL_EDGES_OF_VTX_END();
    }
  }
}

static void setup_workspace (const int64_t nv, int64_t ** loc_ws, double ** val_ws)
{
  if (!*loc_ws) {
    *loc_ws = xmalloc (nv * sizeof (**loc_ws));
    for (int64_t k = 0; k < nv; ++k) (*loc_ws)[k] = -1;
  }
}

static void setup_sparse_y (const double beta,
                            int64_t y_deg, int64_t * y_idx, double * y_val,
                            int64_t * loc_ws, double * val_ws /*UNUSED*/)
{
  if (0.0 == beta) {
    for (int64_t k = 0; k < y_deg; ++k) {
      const int64_t i = y_idx[k];
      loc_ws[i] = k;
      y_val[k] = 0.0;
    }
  } else if (1.0 == beta) {
    /* Still have to set up the pattern... */
    for (int64_t k = 0; k < y_deg; ++k) {
      const int64_t i = y_idx[k];
      loc_ws[i] = k;
    }
  } else if (-1.0 == beta) {
    for (int64_t k = 0; k < y_deg; ++k) {
      const int64_t i = y_idx[k];
      loc_ws[i] = k;
      y_val[k] = -y_val[k];
    }
  } else if (1.0 != beta) {
    for (int64_t k = 0; k < y_deg; ++k) {
      const int64_t i = y_idx[k];
      const double yi = y_val[k];
      loc_ws[i] = k;
      y_val[k] = beta * y_val[k];
    }
  }
}

void stinger_dspmTspv (const int64_t nv, const double alpha, const struct stinger *S, const int64_t x_deg, const int64_t * x_idx, const double * x_val, const double beta, int64_t * y_deg_ptr, int64_t * y_idx, double * y_val, int64_t * loc_ws_in, double * val_ws_in /*UNUSED*/)
{
  int64_t y_deg = * y_deg_ptr;
  int64_t * restrict loc_ws = loc_ws_in;
  double * restrict val_ws = val_ws_in;

  setup_workspace (nv, &loc_ws, &val_ws);
  setup_sparse_y (beta, y_deg, y_idx, y_val, loc_ws, val_ws);

  for (int64_t xk = 0; xk < x_deg; ++xk) {
    const int64_t i = x_idx[xk];
    const double alphaxi = ALPHAXI_VAL (alpha, x_val[xk]);
    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
      const int64_t j = STINGER_EDGE_DEST;
      const double aij = STINGER_EDGE_WEIGHT;
      int64_t yk = loc_ws[j];
      if (yk < 0) {
        yk = loc_ws[j] = y_deg++;
        assert (yk < nv);
        y_idx[yk] = j;
        y_val[yk] = aij * alphaxi;
      } else
        y_val[yk] += aij * alphaxi;
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }

  if (!val_ws_in) free (val_ws);
  if (!loc_ws_in) free (loc_ws);
  *y_deg_ptr = y_deg;
}

void stinger_unit_dspmTspv (const int64_t nv, const double alpha, const struct stinger *S, const int64_t x_deg, const int64_t * x_idx, const double * x_val, const double beta, int64_t * y_deg_ptr, int64_t * y_idx, double * y_val, int64_t * loc_ws_in, double * val_ws_in /*UNUSED*/)
{
  int64_t y_deg = * y_deg_ptr;
  int64_t * restrict loc_ws = loc_ws_in;
  double * restrict val_ws = val_ws_in;

  setup_workspace (nv, &loc_ws, &val_ws);
  setup_sparse_y (beta, y_deg, y_idx, y_val, loc_ws, val_ws);

  for (int64_t xk = 0; xk < x_deg; ++xk) {
    const int64_t i = x_idx[xk];
    const double alphaxi = ALPHAXI_VAL (alpha, x_val[xk]);
    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
      const int64_t j = STINGER_EDGE_DEST;
      int64_t yk = loc_ws[j];
      if (yk < 0) {
        yk = loc_ws[j] = y_deg++;
        assert (yk < nv);
        y_idx[yk] = j;
        y_val[yk] = alphaxi;
      } else
        y_val[yk] += alphaxi;
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }

  if (!val_ws_in) free (val_ws);
  if (!loc_ws_in) free (loc_ws);
  *y_deg_ptr = y_deg;
}
