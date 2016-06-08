//
// Created by jdeeb3 on 6/7/16.
//

#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_utils/timer.h"

#include "stinger_alg/graph_partition.h"

int
main(int argc, char *argv[]) {

    char name[1024];
    char type_str[1024];

    stinger_registered_alg * alg =
    stinger_register_alg(
            .name="graphpartition",
    .data_per_vertex=sizeof(int64_t) + sizeof(int64_t),
    .data_description="ll partitions partition_sizes",
    .host="localhost",
    );

    double num_vertices = stinger_max_active_vertex(alg->stinger) + 1;
    int num_partitons = 2;
    double partition_capacity = (double)num_vertices/(double)num_partitons;

    int opt = 0;
    while(-1 != (opt = getopt(argc, argv, "n:c:h"))) {
        switch(opt) {
            case 'n': {
                num_partitons = atof(optarg);
            } break;
            case 'c': {
                partition_capacity = atof(optarg);
            } break;
            default:
                printf("Unknown option '%c'\n", opt);
            case '?':
            case 'h': {
                printf(
                        "GraphPartition\n"
                                "==================================\n"
                                "\n"
                                "  -n        Set number of partitions (default: %ld)\n"
                                "  -e        Set partition capacity (default: %0.1e)\n"
                                "\n",num_partitons,partition_capacity);
                return(opt);
            }
        }
    }

    if(!alg) {
        LOG_E("Registering algorithm failed.  Exiting");
        return -1;
    }

    int64_t * partitions = (int64_t*)alg->alg_data;
    int64_t * partition_sizes = (int64_t *)(partitions + alg->stinger->max_nv);

    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
     * Initial static computation
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    stinger_alg_begin_init(alg); {
        if (stinger_max_active_vertex(alg->stinger) > 0)
            graph_partition(alg->stinger, stinger_max_active_vertex(alg->stinger) + 1, num_partitons, partition_capacity, partitions, partition_sizes);
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
                graph_partition(alg->stinger, stinger_max_active_vertex(alg->stinger) + 1, num_partitons, partition_capacity, partitions, partition_sizes);
            }

            stinger_alg_end_post(alg);
        }
    }

}