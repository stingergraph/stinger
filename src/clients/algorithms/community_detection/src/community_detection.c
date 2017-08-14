//
// Created by jdeeb3 on 6/22/16.
//

#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/formatting.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_utils/timer.h"
#include "stinger_alg/modularity.h"

int
main(int argc, char *argv[])
{
    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
    * Options and arg parsing
    * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    int64_t max_iter = 5;
    char * alg_name = "community_detection";

    int opt = 0;
    while(-1 != (opt = getopt(argc, argv, "i:n:x?h"))) {
        switch(opt) {
            case 'i': {
                max_iter = atol(optarg);
                if(max_iter < 1) {
                    max_iter = 5;
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
                        "Louvain Community detection\n"
                                "==================================\n"
                                "\n"
                                "This approach returns the first level of Louvain's Community detection\n"
                                "\n"
                                "  -i <num>  Set the max number of iterations (%" PRId64 " by default)\n"
                                "  -n <str>  Set the algorithm name (%s by default)\n"
                                "\n", max_iter,alg_name
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
    .data_per_vertex=sizeof(int64_t),
    .data_description="l partitions",
    .host="localhost",
    );

    if(!alg) {
        LOG_E("Registering algorithm failed.  Exiting");
        return -1;
    }

    int64_t * partitions = (int64_t *)alg->alg_data;


    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
    * Initial static computation
    * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    stinger_alg_begin_init(alg); {
        if (stinger_max_active_vertex(alg->stinger) > 0)
            community_detection(alg->stinger, stinger_max_active_vertex(alg->stinger) + 1, partitions, max_iter);
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
                community_detection(alg->stinger, nv, partitions, max_iter);
            }

            stinger_alg_end_post(alg);
        }
    }

    LOG_I("Algorithm complete... shutting down");
}
