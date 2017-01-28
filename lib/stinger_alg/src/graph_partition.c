#include "graph_partition.h"
#include <float.h>


/*
 * heuristic that assigns a vertex to the partition where it has the most edges in
 * weighted by a linear function that pentalized large partitions so as to avoid trivial solutions
 * Inputs: stinger graph, number of vertices, boolean vertex partition, total size of the partition
 */
double_t
linear_deterministic_greedy(stinger_t * S, int64_t NV, int64_t partition_label, int64_t * vertex_partition, int64_t vertex_partition_size, int64_t source_vertex, double_t capacity_constraint)
{
	//Need to get a list of the source vetex's neighbors
	//uint8_t vertex_neighbors[NV]; // TODO: chage this to use malloc
	//for (uint64_t v = 0; v < NV; v++){
		//vertex_neighbors[v] = 0;
	//}
    int count = 0;
    //OMP ("omp parallel for reduction(+:count)")
	STINGER_PARALLEL_FORALL_OUT_EDGES_OF_VTX_BEGIN(S,source_vertex)
    {
        if (vertex_partition[STINGER_EDGE_DEST] == partition_label){
            count += STINGER_EDGE_WEIGHT; //How to support negative edge weights?
        }
        //vertex_neighbors[STINGER_EDGE_DEST] = STINGER_EDGE_WEIGHT; //How to support negative edge weights?


    }STINGER_PARALLEL_FORALL_OUT_EDGES_OF_VTX_END();

	//int count = 0;
	//for (uint64_t v = 0; v < NV; v++) {
		//if (vertex_partition[v] && vertex_neighbors[v]){
			//we are trying the count the number of verticies in the partition that are also in the neighborhood of the source vertex
			//count += vertex_neighbors[v];
		//}
	//}
	double_t weight = 1 - (double_t) vertex_partition_size/capacity_constraint;
    return (double_t)count*weight;

}

/*This function partitons a graph into an aribitrary number of parts 
Inputs: the graph to be partitioned, total number of verticies, non-zero number of partitions
 returns a list of the verticies in the graph assigned to a partition number. Partition label of zeros means that the
 vertex has been unassigned
*/
void
graph_partition(struct stinger * S, uint64_t NV, uint64_t num_partitions, double_t partition_capacity, int64_t * partitions, int64_t * partitions_sizes)
{
    srand(time(NULL));
    //int64_t partitions[NV];
    //int64_t partitions_sizes[num_partitions];

    //instatiate these to zeros
    //for(uint64_t i = 0; i < num_partitions; i++){
        //partitions_sizes[i] = 0;
    //}
    //for(uint64_t j = 0; j < NV; j++){
        //partitions[j] = 0;
    //}

	//for each vertex in the graph,
	//assign it to a partition given a heuristic
    //OMP ("omp parallel for")
    for (uint64_t v = 0; v < NV; v++) {
        uint64_t max_partition = 0;
        double_t max_score = -DBL_MAX;
        for (uint64_t i = 0; i < num_partitions; i++){
            //evaluate heuristic score per each partition
            double_t partition_score = linear_deterministic_greedy(S, NV, i+1, partitions, partitions_sizes[i], v, partition_capacity);
            if (partition_score == max_score){
                //break ties based on size of partitions
                if (partitions_sizes[max_partition] > partitions_sizes[i+1]){
                    max_partition = i+1;
                }
                else if (partitions_sizes[max_partition] == partitions_sizes[i+1]){
                    //break ties randomly
                    int r = rand()%1;
                    if (r < 0.5){
                        max_partition = i+1;
                    }
                }
            }
            else if (partition_score > max_score) {
                max_partition = i+1;
                max_score = partition_score;
            }
        }
        //add the vertex to the winning partition
        partitions[v] = max_partition;
        partitions_sizes[max_partition] += 1;
    }
    //TODO fix returns, what would be a good output?
    for (int64_t i = 0; i<NV; i++){
        printf("Verticies Added %" PRId64 " => %" PRId64 "\n", i, partitions[i]);
    }
}


