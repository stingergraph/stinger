//
// Created by jdeeb3 on 6/8/16.
//

#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_utils/timer.h"

#include "stinger_alg/independent_sets.h"

int
main(int argc, char *argv[]) {

    stinger_registered_alg * alg =
    stinger_register_alg(
            .name="independentset",
    .data_per_vertex=sizeof(int64_t),
    .data_description="l independent_sets",
    .host="localhost",
    );

    char * alg_name = "independent_set";

    int opt = 0;
    while(-1 != (opt = getopt(argc, argv, "n:h"))) {
        switch(opt) {
            case 'n': {
                alg_name = optarg;
            } break;
            default:
                printf("Unknown option '%c'\n", opt);
            case '?':
            case 'h': {
                printf(
                        "IndependetSet\n"
                                "==================================\n"
                                "This algorithm finds a set of vertices that form an independent set. \n"
                                "The algorithms works by randomly adding verticies to the graph such that no edges exist\n"
                                "between any verticies in the selected set. The algoirhtm stops where there are no more\n"
                                "vertices that can be added.\n"
                                "\n"
                                "  -n <str>  Set the algorithm name (%s by default)\n"
                                "\n",alg_name);
                return(opt);
            }
        }
    }

    if(!alg) {
        LOG_E("Registering algorithm failed.  Exiting");
        return -1;
    }

    int64_t * ind_sets = (int64_t*)alg->alg_data;

    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
     * Initial static computation
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    stinger_alg_begin_init(alg); {
        if (stinger_max_active_vertex(alg->stinger) > 0)
            independent_set(alg->stinger, stinger_max_active_vertex(alg->stinger) + 1, ind_sets);
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
                independent_set(alg->stinger, stinger_max_active_vertex(alg->stinger) + 1, ind_sets);
            }

            stinger_alg_end_post(alg);
        }
    }

}