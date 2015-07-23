#include <stdlib.h>
#include <stdio.h>
#include "stinger_alg/pagerank.h"
#include "stinger_core/stinger.h"
#include "stinger_core/stinger_shared.h"
#include "stinger_alg/rmat.h"
#include "stinger_utils/timer.h"

#define NUM_EDGES (1 << 23)

int main(int argc, char * argv[]) {
	struct stinger * S = stinger_new_full(1<<20, 1<<22, 2, 2);

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

	double * tmp_pr = (double *)xcalloc(nv, sizeof(double));
	double * pr_d = (double *)xcalloc(nv, sizeof(double));
	double * pr_u = (double *)xcalloc(nv, sizeof(double));

	OMP("omp parallel for")
	for (int64_t z=0; z < nv; z++) {
		pr_d[z] = 1 / ((double)nv);
	}

	fprintf(stderr,"Directed Pagerank...");

	// Time directed pagerank
	tic();

	page_rank_directed(S, nv, pr_d, tmp_pr, EPSILON_DEFAULT, DAMPINGFACTOR_DEFAULT, 100);

	double directed_time = toc();

	fprintf(stderr,"Done.\n");

	OMP("omp parallel for")
	for (int64_t z=0; z < nv; z++) {
		pr_u[z] = 1 / ((double)nv);
		tmp_pr[z] = 0;
	}

	fprintf(stderr,"Undirected Pagerank...");

	// Time undirected pagerank

	tic();

	page_rank(S, nv, pr_u, tmp_pr, EPSILON_DEFAULT, DAMPINGFACTOR_DEFAULT, 100);

	double undirected_time = toc();

	fprintf(stderr,"Done.\n");

	fprintf(stderr,"\nUndirected: %lf s\nDirected: %lf s\n",undirected_time,directed_time);

	FILE * fp = fopen("pagerank.csv", "w");

	fprintf(fp,"vertex,pagerank_directed,pagerank_undirected\n");

	for (int64_t z=0; z < nv; z++) {
		fprintf(fp,"%ld,%lf,%lf",z,pr_d[z],pr_u[z]);
		fprintf(fp,"\n");
	}

	fclose(fp);
}