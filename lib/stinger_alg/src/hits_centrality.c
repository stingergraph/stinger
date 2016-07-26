#include "stinger_core/stinger.h"
#include "hits_centrality.h"

//helper function to update the authority values for the verticies in the graph
void
authority_update(stinger_t * s, int64_t NV, double_t * authority_scores, double_t * hubs_scores){
    int64_t norm = 0; //we wil have to normalize each iteration
    for(int64_t v = 0; v< NV; v++){
        authority_scores[v] = 0;
        STINGER_FORALL_IN_EDGES_OF_VTX_BEGIN(s, v)
                                {
                                    authority_scores[v] += hubs_scores[STINGER_EDGE_DEST];

                                }
        STINGER_FORALL_IN_EDGES_OF_VTX_END();
        norm += pow(authority_scores[v], 2);
    }
    norm = sqrt(norm); //normalize using the sum of squared errors
    for(int64_t v = 0; v< NV; v++){
        authority_scores[v] = authority_scores[v]/norm;
    }
}
//helper function to update the hub values for the verticies in the graph
void
hub_update(stinger_t * s, int64_t NV, double_t * authority_scores, double_t * hubs_scores){
    int64_t norm = 0; //we wil have to normalize each iteration
    for(int64_t v = 0; v< NV; v++){
        hubs_scores[v] = 0;
        STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(s, v)
                                {
                                    hubs_scores[v] += authority_scores[STINGER_EDGE_DEST];

                                }
        STINGER_FORALL_OUT_EDGES_OF_VTX_END();
        norm += pow(hubs_scores[v], 2);
    }
    norm = sqrt(norm); //normalize using the sum of squared values
    for(int64_t v = 0; v< NV; v++){
        hubs_scores[v] = hubs_scores[v]/norm;
    }
}



/**
 * This is an implementation of the HITS algorithm
 * For best prformance, set k = 10
 */
void
hits_centrality(stinger_t * s, int64_t NV, double_t * hubs_scores, double_t * authority_scores, int64_t k) {
    for (int64_t i = 0; i < NV; i++) {
        //initialize all the hub and authority scores to one
        hubs_scores[i] = 1;
        authority_scores[i] = 1;
    }
    //run for k steps
    for (int64_t i = 0; i < k; i++) {
        authority_update(s, NV, authority_scores, hubs_scores);
        hub_update(s, NV, authority_scores, hubs_scores);

    }
}
