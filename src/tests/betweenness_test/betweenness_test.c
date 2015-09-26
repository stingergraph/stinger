#include <stdlib.h>
#include <stdio.h>
#include "stinger_alg/betweenness.h"
#include "stinger_core/stinger.h"
#include "stinger_core/stinger_shared.h"
#include "stinger_alg/rmat.h"
#include "stinger_utils/timer.h"

#define LOG_AT_E 1

#define NUM_EDGES (1 << 20)

int main(int argc, char * argv[]) {
  struct stinger_config_t * stinger_config = (struct stinger_config_t *)xcalloc(1,sizeof(struct stinger_config_t));
  stinger_config->nv = 1<<20;
  stinger_config->nebs = 1<<22;
  stinger_config->netypes = 2;
  stinger_config->nvtypes = 2;
  stinger_config->memory_size = 0;
  struct stinger * S = stinger_new_full(stinger_config);
  xfree(stinger_config);

  int64_t i, j;
  int64_t scale = 0;
  int64_t nv = stinger_max_nv(S);

  int64_t tmp_nv = nv;
  while (tmp_nv >>= 1) ++scale;

  fprintf(stderr,"nv: %ld\n",nv);

  double a = 0.55;
  double b = 0.15;
  double c = 0.15;
  double d = 0.25;

  dxor128_env_t env;
  dxor128_seed(&env, 0);

  fprintf(stderr,"Adding edges...");

  OMP("omp parallel for")
  for (int64_t z=0; z < NUM_EDGES; z++) {
    rmat_edge (&i, &j, scale, a, b, c, d, &env);

    stinger_insert_edge_pair(S, 1, i, j, 1, 1);
  }

  fprintf(stderr,"Done.\n");

  double * bc_d = (double *)xcalloc(nv, sizeof(double));
  int64_t * found_d = (int64_t *)xcalloc(nv, sizeof(int64_t));
  double * bc_u = (double *)xcalloc(nv, sizeof(double));
  int64_t * found_u = (int64_t *)xcalloc(nv, sizeof(int64_t));

  srand(100);

  fprintf(stderr,"Sample Parallelism BC...");

  tic();

  sample_search(S, nv, 256, bc_d, found_d);

  double directed_time = toc();

  fprintf(stderr,"Done.\n");

  srand(100);

  fprintf(stderr,"Intra-sample Parallelism BC...");

  tic();

  //sample_search_parallel(S, nv, 256, bc_u, found_u);


  double undirected_time = toc();

  fprintf(stderr,"Done.\n");

  fprintf(stderr,"\nIntra-sample: %lf s\nInter-sample: %lf s\n",undirected_time,directed_time);

  FILE * fp = fopen("bc.csv", "w");

  fprintf(fp,"vertex,bc_intersample,bc_intrasample\n");

  for (int64_t z=0; z < nv; z++) {
    fprintf(fp,"%ld,%lf,%lf",z,bc_d[z],bc_u[z]);
    fprintf(fp,"\n");
  }

  fclose(fp);
}