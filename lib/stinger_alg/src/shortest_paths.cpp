//
// Created by jdeeb3 on 5/20/16.
//This file contains algorithms for calculating shortest paths
//
#include "shortest_paths.h"

//these algorithms need to be sorted based on cost to reach them,
//thus we need a new structure to handle vertex cost pairs
typedef struct{
    int64_t vertex;
    int64_t cost;
}weighted_vertex_t;

//this is a simple comparison function for the queue that sorts based on the cost to reach a vertex
bool
comp(weighted_vertex_t a, weighted_vertex_t b)
{
    return a.cost < b.cost;
}
typedef bool(*CompareType)(weighted_vertex_t a, weighted_vertex_t b);

//This algorithm finds the shortest path between two given verticies in the graph
//ONLY WORKS FOR NON_NEGATIVE POSITIVE WEIGHTS
//this algorithm is a modified version of dijkstras but can be manipulated to have a heurisitc
//takes in the following inputs: the graph itself, the number of verticies in the graph, the source vertex, and the destination vertex
//returns the path length of the shortest path
int64_t
a_star(stinger_t * S, int64_t NV, int64_t source_vertex, int64_t dest_vertex, bool ignore_weights){

    std::vector<int64_t> cost_so_far(NV);
    for (int64_t v = 0; v < NV; v++){
        cost_so_far[v] = std::numeric_limits<int64_t>::max(); // initialize all the values to infinity
    }
    //represents the verties to look at in the graph
    std::priority_queue<weighted_vertex_t, std::vector<weighted_vertex_t>, CompareType> frontier (comp);
    //add the initial vertex to the priority queue
    weighted_vertex_t source;
    source.vertex = source_vertex;
    source.cost = 0;
    cost_so_far[source_vertex] = 0;
    frontier.push(source);

    while (!frontier.empty()) {
        weighted_vertex_t current = frontier.top();
        frontier.pop();

        if (current.vertex == dest_vertex) {
            //we found our goal!
            return cost_so_far[current.vertex];
        }

        STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, current.vertex)
                                {
                                    //for all the neighbors of the current vertex
                                    int64_t new_cost;
                                    if (ignore_weights){
                                        new_cost= cost_so_far[current.vertex] + 1; // disregard the weights, instead count paths
                                    }
                                    else{
                                        new_cost = cost_so_far[current.vertex] + STINGER_EDGE_WEIGHT;
                                    }
                                    if (new_cost < cost_so_far[STINGER_EDGE_DEST] || cost_so_far[STINGER_EDGE_DEST] == std::numeric_limits<int64_t>::max()) {
                                        cost_so_far[STINGER_EDGE_DEST] = new_cost;
                                        weighted_vertex_t next;
                                        next.vertex = STINGER_EDGE_DEST;
                                        next.cost = new_cost;
                                        frontier.push(next);
                                    }
                                }
        STINGER_FORALL_OUT_EDGES_OF_VTX_END();

    }
    //we failed to reach this vertex - meaning that there is no path between these two verticies
    return cost_so_far[dest_vertex]; //should return max integer value as a placeholder for infinity
}

//This algorithm finds the shortest path from a given source vertex to all other verticies in the graph
//returns an array where each index represents the shortest path length to that vertes from the source
//this is an implementation of dijkstra's and so will not work for negative edge weights
std::vector<int64_t>
dijkstra(stinger_t * S, int64_t NV, int64_t source_vertex, bool ignore_weights){
    std::vector<int64_t> cost_so_far (NV);
    for (int64_t v = 0; v < NV; v++){
        cost_so_far[v] = std::numeric_limits<int64_t>::max(); // initialize all the values to infinity
    }
    cost_so_far[source_vertex] = 0;
    //represents the verticies to look at in the graph
    std::priority_queue<weighted_vertex_t, std::vector<weighted_vertex_t>, CompareType> frontier (comp);
    //add the initial vertex to the priority queue
    weighted_vertex_t source;
    source.vertex = source_vertex;
    source.cost = cost_so_far[source_vertex];
    frontier.push(source);

    while (!frontier.empty()) {
        weighted_vertex_t current = frontier.top();
        frontier.pop();

        STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, current.vertex){
                                    int64_t new_cost;
                                    if (ignore_weights){
                                        new_cost= cost_so_far[current.vertex] + 1; // disregard the weights, instead count paths
                                    }
                                    else{
                                        new_cost = cost_so_far[current.vertex] + STINGER_EDGE_WEIGHT;
                                    }
                                    if(new_cost < cost_so_far[STINGER_EDGE_DEST]){
                                        cost_so_far[STINGER_EDGE_DEST] = new_cost;
                                        weighted_vertex_t next;
                                        next.vertex = STINGER_EDGE_DEST;
                                        next.cost = new_cost;
                                        frontier.push(next);
                                    }
        }STINGER_FORALL_OUT_EDGES_OF_VTX_END();
    }
    return cost_so_far;
}

//this is a slower way to find all pairs shortest paths, but  since there is no adjaceny matrix
//this approach might end up being faster.
std::vector<std::vector <int64_t> >
all_pairs_dijkstra(stinger_t * S, int64_t NV){
    std::vector<std::vector <int64_t> > dist(NV, std::vector<int64_t>(NV));
    for (int64_t v = 0; v < NV; v++){
        std::vector<int64_t> row = dijkstra(S, NV, v, false);
        for (int64_t i = 0; i < NV; i++){
            dist[v][i] = row[i];
        }
        //dist[v] = row;
    }
    return dist;
}
int64_t
mean_shortest_path(stinger * S, int64_t NV){
    std::vector<std::vector <int64_t> > all_paths = all_pairs_dijkstra(S, NV);
    int64_t total = 0;
    for (int64_t i = 0; i < NV; i++){
        for (int64_t j = 0; j< NV; j++){
            total += all_paths[i][j];
        }
    }
    return total/(NV*NV);
}