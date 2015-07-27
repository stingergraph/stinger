#include <stdlib.h>
#include <stdio.h>
#include "stinger_alg/adamic_adar.h"
#include "stinger_core/stinger.h"
#include "stinger_core/stinger_shared.h"
#include "stinger_alg/rmat.h"
#include "stinger_utils/timer.h"

#define NUM_EDGES (1 << 16)

int main(int argc, char * argv[]) {
  struct stinger * S = stinger_new_full(1<<13, 1<<16, 2, 2);

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

  fprintf(stderr,"Adamic Adar...\n");

  double total_time = 0.0;
  double epoch_time = 0.0;
  double algorithm_time = 0.0;
  
  FILE * fp = fopen("adamic.csv", "w");

  fprintf(fp,"Vertex,Outdegree,num_candidates,time (s)\n");

  for (int64_t z=0; z < nv; z++) {
    int64_t * candidates = NULL;
    double * scores = NULL;

    tic();

    int64_t num_candidates = adamic_adar(S, z, -1, &candidates, &scores);

    algorithm_time = toc();

    fprintf(fp,"%ld,%ld,%ld,%lf\n",z,stinger_outdegree(S,z),num_candidates,algorithm_time);

    total_time += algorithm_time;
    epoch_time += algorithm_time;

    if (z != 0 && z % 1000 == 0) {
      fprintf(stderr,"%ld vertices in Total %lf (%lf s last 1000)\n",z,total_time,epoch_time);
      epoch_time = 0.0;
    }

    if (candidates != NULL) {
      xfree(candidates);
    }
    if (scores != NULL) {
      xfree(scores);
    }
  }

  fclose(fp);

  fprintf(stderr,"Done.\n");

  fprintf(stderr,"\nTime: %lf s\n",total_time);
}