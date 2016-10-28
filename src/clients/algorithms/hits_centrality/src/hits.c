//
// Created by jdeeb3 on 6/8/16.
//

#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_alg/hits_centrality.h"

int
main(int argc, char *argv[])
{
    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
    * Options and arg parsing
    * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    int64_t num_iter = 10;
    char * alg_name = "hits_centrality";

    int opt = 0;
    while(-1 != (opt = getopt(argc, argv, "k:n:?h"))) {
        switch(opt) {

            case 'k': {
                num_iter = atol(optarg);
                if(num_iter < 1) {
                    num_iter = 10;
                }
            } break;

            case 'n': {
                alg_name = optarg;
            } break;

            default:
                printf("Unknown option '%c'\n", opt);
            case '?':
            case 'h': {
                printf(
                        "Hits Centrality\n"
                                "==================================\n"
                                "\n"
                                "Performs the iterative HITs algorithm on the vertices in the STINGER graph \n"
                                "The algorithm calculates authority and hub values per vetex using mutual recursion. \n"
                                "An authority value is computed as the sum of the scaled hub values, and conversely a \n"
                                "hub value is the sum of the scaled authority values of the pages it points to.\n"
                                "\n"
                                "  -k <num>  Set the maximum number of iterations (%ld by default)\n"
                                "  -n <str>  Set the algorithm name (%s by default)\n"
                                "\n", num_iter,  alg_name
                );
                return(opt);
            }
        }
    }

    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
    * Setup and register algorithm with the server
    * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    stinger_registered_alg * alg =
    stinger_register_alg(
            .name=alg_name,
    .data_per_vertex=sizeof(double) + sizeof(double),
    .data_description="ll hubs_scores authority_scores",
    .host="localhost",
    );

    if(!alg) {
        LOG_E("Registering algorithm failed.  Exiting");
        return -1;
    }

    double * hubs_scores = (double *)alg->alg_data;
    double * authority_scores = (int64_t *)(hubs_scores + alg->stinger->max_nv);

    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
    * Initial static computation
    * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    stinger_alg_begin_init(alg); {
        if (stinger_max_active_vertex(alg->stinger) > 0)
            hits_centrality(alg->stinger, stinger_max_active_vertex(alg->stinger) + 1, hubs_scores, authority_scores, num_iter);
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
            int64_t nv = stinger_max_active_vertex(alg->stinger) + 1;
            if (nv > 0) {
                hits_centrality(alg->stinger, nv, hubs_scores, authority_scores, num_iter);
            }

            stinger_alg_end_post(alg);
        }
    }


}