//
// Created by jdeeb3 on 5/24/16.
//
#include "diameter.h"
#include "shortest_paths.h"
/**
 * this algorithm gives an approximation of the graph diameter.
 * It works by starting from a source vertex, and finds an end vertex that is farthest away
 * This process is repeated by treating that end vertex as the new starting vertex and ends
 * when the graph distance no longer increases
 * Inputs: S- the graph itself, nv - the total number of active verticies in the graph, source - a starting vertex
 * dist - the variable that will hold the resulting diameter, and ignore_weights - a flag that controls wheither a user
 * wants to consider the weights along the edge or not.
 */
int64_t
pseudo_diameter(stinger_t * S,int64_t NV , int64_t source, int64_t dist, bool ignore_weights){
    //int64_t source = 1; // start at the first vertex in the graph, this could be any vertex
    dist = 0;
    int64_t target = source;

    std::vector<int64_t> paths(NV);
    while(1){
        int64_t new_source = target;
        paths = dijkstra(S, NV, new_source, ignore_weights);
        int64_t max = std::numeric_limits<int64_t>::min();
        int64_t max_index = 0;
        for( int64_t i = 0; i< NV; i++){
            if (paths[i] > max and paths[i] != std::numeric_limits<int64_t>::max()){
                max = paths[i];
                max_index = i;
            }
        }
        if (max > dist){
            target = max_index;
            //source = new_source;
            dist = max;
        }
        else{
            break;
        }
    }
    return dist;
}
/**
 * this algorithm gives an exact diameter for the graph.
 * It runs Dijkstras for every vertex in the graph, and returns the maximum shortest path
 * Inputs: S- the graph itself, nv - the total number of active verticies in the graph
*/

int64_t
exact_diameter(stinger_t * S, int64_t NV){
    std::vector<std::vector <int64_t> > all_pairs (NV, std::vector<int64_t>(NV));
    all_pairs = all_pairs_dijkstra(S, NV);
    int64_t max = std::numeric_limits<int64_t>::min();
    for( int64_t i = 0; i< NV; i++){
        for (int64_t j = 0; j < NV; j++){
            if (all_pairs[i][j] > max and all_pairs[i][j] != std::numeric_limits<int64_t>::max()){
                max = all_pairs[i][j];
            }
        }
    }
    return max;
}