//
// Created by jdeeb3 on 5/31/16.
//
#include "independent_sets.h"


//we are going to use the independent_set array as a mask for the verticies in the independent set
//if independent_set[vertex] = 1, then  vertex is in the set - else that vertex is not in the set
void
independent_set(stinger_t * S, int64_t NV, int64_t * independent_set){
    int64_t vertex_pool[NV]; //TODO this should be using malloc
    int64_t vertexCount = NV;
    srand(time(NULL));
    for (int64_t i = 0; i < NV; i ++){
        independent_set[i] = 0; // set all values to zero
        vertex_pool[i] = 1; //add all the verticies to the pool
    }

    while (vertexCount != 0){
        int r = rand()%NV;
        if (independent_set[r] != 1 && vertex_pool[r] == 1){
            //this means we have never seen the vertex before and the vertex is a valid choice
            independent_set[r] = 1; //add the vertex from the set
            //now we need to remove the vertex from the pool
            vertex_pool[r] = 0;
            vertexCount -= 1;
            STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, r){
                                        if (vertex_pool[STINGER_EDGE_DEST]) {
                                            vertex_pool[STINGER_EDGE_DEST] = 0;
                                            vertexCount -= 1;
                                        }

                                    }STINGER_FORALL_OUT_EDGES_OF_VTX_END();
            STINGER_FORALL_IN_EDGES_OF_VTX_BEGIN(S, r){
                                        if (vertex_pool[STINGER_EDGE_DEST]) {
                                            vertex_pool[STINGER_EDGE_DEST] = 0;
                                            vertexCount -= 1;
                                        }
                                    }STINGER_FORALL_IN_EDGES_OF_VTX_END();
        }

    }

}