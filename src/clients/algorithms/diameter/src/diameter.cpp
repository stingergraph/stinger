//
// Created by jdeeb3 on 6/8/16.
//

#include <stdint.h>
#include <stinger_net/stinger_alg.h>

extern "C" {
#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_utils/timer.h"
}
#include "stinger_alg/diameter.h"


int
main(int argc, char *argv[]) {
    int64_t source_vertex = 0;
    int64_t dist = 0; //this is a placeholder that stores the length of the diameter in the graph, do not change this value
    bool ignore_weights = false;

    char * alg_name = "pseudo_diameter";
    stinger_register_alg_params params;
    params.name= "pseudo_diameter";
    params.data_per_vertex=sizeof(int64_t);
    params.data_description="l vertexPool";
    params.host="localhost";
    stinger_registered_alg *alg = stinger_register_alg_impl(params);

   /* stinger_registered_alg *alg =
            stinger_register_alg(
                    .name="pseudo_diameter",
                    .data_per_vertex=sizeof(int64_t),
                    .data_description="l vertexPool",
                    .host="localhost",
            );*/
    int weighting;
    int opt = 0;
    while(-1 != (opt = getopt(argc, argv, "w:s:n:?h"))) {
        switch(opt) {
            case 'w': {
                weighting = atof(optarg);
                if(weighting > 1.0) {
                    ignore_weights = true;
                } else if(weighting <= 0.0) {
                    ignore_weights = false;
                }
            } break;
            case 's': {
                source_vertex = atol(optarg);
                if(source_vertex < 0 || source_vertex > alg->stinger->max_nv ) {
                    //we need to check to make sure that the source vertex is set to a real vertex
                    source_vertex = 0;
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
                        "Pseudo-Diameter\n"
                                "==================================\n"
                                "\n"
                                "Finds an approximate graph diameter on the STINGER graph.\n"
                                "It works by starting from a source vertex, and finds an end vertex that is farthest away.\n"
                                "This process is repeated by treating that end vertes as the new starting vertex, \n"
                                "and ends when the graph distance no longer increases. \n"
                                "\n"
                                "  -s <num>  Set source vertex (%d by default)\n"
                                "  -n <str>  Set the algorithm name (%s by default)\n"
                                "  -w <num>  Set the weighting (0.0 - 1.0) (%lf by default)\n"
                                "\n", 0, weighting, alg_name, 0
                );
                return(opt);
            }
        }
    }

    if(!alg) {
        LOG_E("Registering algorithm failed.  Exiting");
        return -1;
    }

    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
     * Initial static computation
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    stinger_alg_begin_init(alg); {
        if (stinger_max_active_vertex(alg->stinger) > 0)
            pseudo_diameter(alg->stinger,stinger_max_active_vertex(alg->stinger) + 1 , source_vertex, dist, ignore_weights);
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
                pseudo_diameter(alg->stinger,stinger_max_active_vertex(alg->stinger) + 1 , source_vertex, dist, ignore_weights);
            }

            stinger_alg_end_post(alg);
        }
    }


}