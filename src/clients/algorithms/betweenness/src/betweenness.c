#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_alg/betweenness.h"

int
main(int argc, char *argv[])
{
    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
    * Options and arg parsing
    * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    int64_t num_samples = 256;
    double weighting = 0.5;
    uint8_t do_weighted = 1;
    char * alg_name = "betweenness_centrality";

    int opt = 0;
    while(-1 != (opt = getopt(argc, argv, "w:s:n:x?h"))) {
        switch(opt) {
            case 'w': {
                weighting = atof(optarg);
                if(weighting > 1.0) {
                    weighting = 1.0;
                } else if(weighting < 0.0) {
                    weighting = 0.0;
                }
            } break;

            case 's': {
                num_samples = atol(optarg);
                if(num_samples < 1) {
                    num_samples = 1;
                }
            } break;

            case 'n': {
                alg_name = optarg;
            } break;

            case 'x': {
                do_weighted = 0;
            } break;

            default:
                printf("Unknown option '%c'\n", opt);
            case '?':
            case 'h': {
                printf(
                    "Approximate Betweenness Centrality\n"
                    "==================================\n"
                    "\n"
                    "Measures an approximate BC on the STINGER graph by randomly sampling n vertices each pass and\n"
                    "performing one breadth-first traversal from the Brandes algorithm. The samples are then\n"
                    "aggregated and potentially weighted with the result from\n"
                    "the last pass\n"
                    "\n"
                    "  -s <num>  Set the number of samples (%ld by default)\n"
                    "  -w <num>  Set the weighintg (0.0 - 1.0) (%lf by default)\n"
                    "  -x        Disable weighting\n"
                    "  -n <str>  Set the algorithm name (%s by default)\n"
                    "\n", num_samples, weighting, alg_name
                );
                return(opt);
            }
        }
    }

    double old_weighting = 1 - weighting;

    LOG_V("Starting approximate betweenness centrality...");

    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
    * Setup and register algorithm with the server
    * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    stinger_registered_alg * alg =
    stinger_register_alg(
        .name=alg_name,
        .data_per_vertex=sizeof(int64_t) + sizeof(double),
        .data_description="dl bc times_found",
        .host="localhost",
    );

    if(!alg) {
        LOG_E("Registering algorithm failed.  Exiting");
        return -1;
    }

    double * bc = (double *)alg->alg_data;
    int64_t * times_found = (int64_t *)(bc + alg->stinger->max_nv);
    double * sample_bc = NULL;

    if(do_weighted) {
        sample_bc = xcalloc(sizeof(double), alg->stinger->max_nv);
    }

    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
    * Initial static computation
    * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    stinger_alg_begin_init(alg); {
        if (stinger_max_active_vertex(alg->stinger) > 0)
            sample_search(alg->stinger, stinger_max_active_vertex(alg->stinger) + 1, num_samples, bc, times_found);
    } stinger_alg_end_init(alg);

    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
    * Streaming Phase
    * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    while(alg->enabled) {
        /* Pre processing */
        if(stinger_alg_begin_pre(alg)) {
            /* nothing to do */
            stinger_alg_end_pre(alg);
        }

        /* Post processing */
        if(stinger_alg_begin_post(alg)) {
            int64_t nv = (stinger_mapping_nv(alg->stinger))?stinger_mapping_nv(alg->stinger)+1:0;
            if (nv > 0) {
                if(do_weighted) {
                    sample_search(alg->stinger, nv, num_samples, sample_bc, times_found);

                    OMP("omp parallel for")
                    for(int64_t v = 0; v < nv; v++) {
                        bc[v] = bc[v] * old_weighting + weighting* sample_bc[v];
                    }
                } else {
                    sample_search(alg->stinger, nv, num_samples, bc, times_found);
                }
            }

            stinger_alg_end_post(alg);
        }
    }

    LOG_I("Algorithm complete... shutting down");

    if(do_weighted) {
        free(sample_bc);
    }
    xfree(alg);
}
