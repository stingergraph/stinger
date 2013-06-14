#ifndef  STINGER_WORKFLOW_H
#define  STINGER_WORKFLOW_H

#include "stinger.h"
#include "stinger-return.h"

/**
* @file stinger-workflow.h
* @brief STINGER workflow and named result store system. Blackboard, Mediator
* @author Rob McColl
* @date 2013-02-13
*
* A workflow is a management object that connects several data streams
* and algorithms to a single STINGER object.  To user a workflow, create it,
* register all of your algorithms and streams with it, and then run the workflow.
* You can reconfigure which algorithms and streams will be run and how they will
* be run through the command line at run time.  The workflow handles initializing
* the streams and algorithms and provides them with any settings that they 
* receive from command line input or configuration files (as well as 
* tools to parse these).  It also stores pointers to the workspaces used 
* by each algorithm and stream.  The workflow will first call all streams' init
* functions to build the static graph, then allow each algorithm to 
* do any initial processing on the graph.  Afterward, the graph 
* enters streaming mode in which it polls each stream for a batch of
* edges, notifies each algorithm of the batch, applies the batch,
* and notifies each algorithm afterward.  This process will continue until
* no algorithms remain or no streams are still generating.  Algorithms 
* and streams can remove themselves through returning STINGER_REMOVE.
*
* In addition to providing control flow, the workflow also serves as a store
* of optional named results.  Algorithms can store their results here and
* can request references to the results of other algorithms.  In this way,
* multiple algorithms can be strung together to create a greater workflow.
*
* Lastly, do not restrict algorithms to being only graph algorithms.  They 
* can be any type of graph consumer or consumer of the results of other algorithms.
* For example, the result_writer algorithm simply writes out all of the named 
* result store results to disk every batch.
*/

/* included here for the algorithm and stream definitions below, defined farther down */
typedef struct stinger_workflow stinger_workflow_t;

/**
* @brief A single edge insertion/increment or edge deletion.
* 
* Note that this will be inserted undirected and that all insertions are treated as
* increments (so if the edge already exists, the weight will be incremented by the 
* weight given here and the recent timestamp on the edge will be updated appropriately).
* Deletions are indicated by bitwise inverse of the source and destination. Deletions
* completely remove the edge.
*/
typedef struct edge_action {
  int64_t type; 
    /**< The edge type as an integer (must follow rules for edge type) */
    
  vindex_t source; 
    /**< The edge source as a vertex index. Negative indices indicate deletion where the 
     * index is the bitwise inverse of the vertex index (so that edges to zero can be deleted).*/

  vindex_t dest; 
    /**< The edge destination as a vertex index. Negative indices indicate deletion where the 
     * index is the bitwise inverse of the vertex index (so that edges to zero can be deleted).*/

  int64_t weight; 
    /**< The weight of the inserted or incremented edge. */

  int64_t time; 
    /**< The time at which the insertion / deletion takes place. */
} edge_action_t;

/**
* @brief An algorithm to be run within the workflow.
*
* Algorithms consist of a collection of function pointers (any of which can be NULL), a 
* text string name that is used to refer to the algorithm instance, and a workspace pointer that
* the algorithm instance can use to store any information as needed. In the workflow model when
* stinger_workflow_run() is called, ALGNAME_settings() will be called first, then ALGNAME_init(),
* then ALGNAME_before_batch() and ALGNAME_after_batch() will be called repeatedly as batches of 
* edges are added to the graph.  Lastly, ALGNAME_cleanup() will be called either when all generators
* have stopped producing edges or when one of the other functions of that algorithm instance returns
* STINGER_REMOVE.
*
* Algorithm functions will all always be given the STINGER, a reference to the workflow, and the
* algorithm workspace.
*/
typedef struct stinger_alg {
  char * name; 
    /**< A string handle used to refer to refer to the algorithm for configuration, removal, etc. */

  void * workspace; 
    /**< A pointer to the workspace used by the algorithm.  A reference to this is passed to each 
     * of the functions of the algorithm.  It will not be used or modified by the workflow. */

  stinger_return_t	(*init)		(stinger_t *, stinger_workflow_t *, void **); 
    /**< A function ponter for the static initialization of the algorithm.  This function will 
     * be passed the initial static STINGER graph so that it can do any initial computation, 
     * setup the algorithm workspace, register any named results, etc. */

  stinger_return_t	(*settings)	(stinger_t *, stinger_workflow_t *, void **, char *); 
    /**< A pointer to the settings function. This function will be passed a configuration string 
     * that can be interpreted any way that the function wishes, but for consistency so far,
     * most settings strings are in the form of key-value pairs with colons used to separate
     * keys from values and commas used to separate pairs.  Example: key1:value1,key2:value2  
     * Additionally, the workflow provides a convenience function stinger_workflow_to_keyvalue() 
     * for parsing this format into to string arrays. */

  stinger_return_t	(*before_batch)	(stinger_t *, stinger_workflow_t *, void **, int64_t batch, edge_action_t *, int64_t);
    /**< This function will be called before each batch with a view of the STINGER before the batch 
     * is applied and the list of actions that are about to be applied.  Edge actions that contain 
     * negative source or destination are deletions where the source and destination are bitwise 
     * inverse of the vertex IDs.  */

  stinger_return_t	(*after_batch)	(stinger_t *, stinger_workflow_t *, void **, int64_t batch, edge_action_t *, int * , int64_t);
    /**< This function will be called after each batch has been applied with a view of the STINGER
     * after the insertions and deletions have taken place. Additionally, the result returned by the 
     * stinger_incr_edge_pair() or stinger_remove_edge_pair() will be given in an array indicating
     * for each direction of the edge whether or not the insertion actually creaeted a new edge or
     * the deletion actually removed an existing edge.. */

  stinger_return_t	(*cleanup)	(stinger_t *, stinger_workflow_t *, void **);
    /**< This function will be called for the algorithm to free its workspace, de-register its named
     * results, and do any other finalization, cleanup, and memory freeing as needed.  This will
     * be called at the end of stinger_workflow_run() or when any functions of this algorithm
     * instance return STINGER_REMOVE. */
} stinger_alg_t;

/**
* @brief A graph data source that serves to create the initial graph and provides batches of updates.
*
* A stream instance consists of a collection of functions, a string name that is used to
* refer to the stream, and a workspace pointer.  A reference ot the workspace is given to
* each stream function that can be used to store the state of the stream.  Any function of
* the stream can be NULL.  Any function that returns STINGER_REMOVE will cause the stream to 
* be removed formthe workflow and its cleanup function to be called.  The functions of the
* stream will be called by stinger_workflow_run() in the order: settings, then init, then repeatedly
* gen batch until the stream removes itself or all algorithms have been removed from the workflow.
* Updates can be edge insertion and removals (as well as mapping new vertices).
*/
typedef struct stinger_stream {
  char *	name;
    /**< A string handle used to refer to refer to the stream for configuration, removal, etc. */

  void *	workspace;
    /**< A pointer to the workspace used by the stream.  A reference to this is passed to each 
     * of the functions of the stream.  It will not be used or modified by the workflow. */

  stinger_return_t	(*init)		(stinger_t *, stinger_workflow_t *, void **);
    /**< A function ponter for the static initialization of the graph.  This function will 
     * be passed the STINGER graph so that it can insert any initial edges, map any initial
     * vertices, etc.  Keep in mind that other strams may have operated on the graph prior
     * to this stream being initialized - so it is likely not guaranteed to call functions
     * like stinger_set_initial_edges() that assume the graph is empty. */

  stinger_return_t	(*settings)	(stinger_t *, stinger_workflow_t *, void **, char *);
    /**< A pointer to the settings function. This function will be passed a configuration string 
     * that can be interpreted any way that the function wishes, but for consistency so far,
     * most settings strings are in the form of key-value pairs with colons used to separate
     * keys from values and commas used to separate pairs.  Example: key1:value1,key2:value2  
     * Additionally, the workflow provides a convenience function stinger_workflow_to_keyvalue() 
     * for parsing this format into to string arrays. */

  stinger_return_t	(*stream_batch)	(stinger_t *, stinger_workflow_t *, void **, int64_t batch, edge_action_t **, int64_t*);
    /**< This function will be given the stinger, the stream workspace, a reference  TODO */

  stinger_return_t	(*cleanup)	(stinger_t *, stinger_workflow_t *, void **);
    /**< This function will be called for the stream to free its workspace, de-register its named
     * results, and do any other finalization, cleanup, and memory freeing as needed.  This will
     * be called at the end of stinger_workflow_run() or when any functions of this stream
     * instance return STINGER_REMOVE. */
} stinger_stream_t;

typedef enum stinger_named_result_type {
  NR_I64,
  NR_DBL,
  NR_U8,
  NR_I64PAIRS
} stinger_named_result_type_t;

typedef struct stinger_named_result {
  char			      name[1024];
  uint64_t	      	      elements;
  stinger_named_result_type_t type;
  uint8_t		      data[0];
} stinger_named_result_t;

struct stinger_workflow {
  uint8_t		    integrated;
  stinger_alg_t		  * algorithms;
  int64_t		    alg_count;
  int64_t		    alg_size;
  stinger_stream_t	  * streams;
  int64_t		    stream_count;
  int64_t		    stream_size;
  stinger_named_result_t  **named_results;
  int64_t		    named_result_count;
  int64_t		    named_result_size;
  int64_t		    port;
  uint8_t		    web_on;
  stinger_t		  * S;
};

stinger_workflow_t * 
stinger_workflow_new(stinger_t * S);

stinger_workflow_t * 
stinger_workflow_free(stinger_workflow_t ** workflow);

stinger_workflow_t * 
stinger_workflow_register_alg(stinger_workflow_t * workflow, char * name, void * workspace,
  stinger_return_t (*init)(stinger_t *, stinger_workflow_t *, void **),
  stinger_return_t (*settings)	(stinger_t *, stinger_workflow_t *, void **, char *),
  stinger_return_t (*before_batch)(stinger_t *, stinger_workflow_t *, void **, int64_t, edge_action_t *, int64_t),
  stinger_return_t (*after_batch)(stinger_t *, stinger_workflow_t *, void **, int64_t, edge_action_t *, int *, int64_t),
  stinger_return_t (*cleanup)(stinger_t *, stinger_workflow_t *, void **));

stinger_workflow_t *
stinger_workflow_unregister_alg_by_index(stinger_workflow_t * workflow, int64_t index);

stinger_workflow_t *
stinger_workflow_unregister_alg(stinger_workflow_t * workflow, char * name);

stinger_workflow_t * 
stinger_workflow_register_stream(stinger_workflow_t * workflow, char * name, void * workspace,
  stinger_return_t	(*init)		(stinger_t *, stinger_workflow_t *, void **),
  stinger_return_t	(*settings)	(stinger_t *, stinger_workflow_t *, void **, char *),
  stinger_return_t	(*stream_batch)	(stinger_t *, stinger_workflow_t *, void **, int64_t, edge_action_t **, int64_t*),
  stinger_return_t	(*cleanup)	(stinger_t *, stinger_workflow_t *, void **));

stinger_workflow_t *
stinger_workflow_unregister_stream_by_index(stinger_workflow_t * workflow, int64_t index);

stinger_workflow_t *
stinger_workflow_unregister_stream(stinger_workflow_t * workflow, char * name);

stinger_named_result_t *
stinger_workflow_new_named_result(stinger_workflow_t * workflow, char * name, stinger_named_result_type_t type, uint64_t count);

void
stinger_workflow_write_named_results(stinger_workflow_t * workflow, char * path, uint64_t batch);

stinger_return_t
stinger_workflow_delete_named_result(stinger_workflow_t * workflow, char * name);

stinger_named_result_t *
stinger_workflow_get_named_result(stinger_workflow_t * workflow, char * name);

stinger_named_result_type_t
stinger_named_result_type_get(stinger_named_result_t * nr);

char *
stinger_named_result_name_get(stinger_named_result_t * nr);

uint64_t
stinger_named_result_count_get(stinger_named_result_t * nr);

void *
stinger_named_result_read_data(stinger_named_result_t * nr);

void *
stinger_named_result_write_data(stinger_named_result_t * nr);

void
stinger_named_result_commit_data(stinger_named_result_t * nr);

void
stinger_workflow_to_keyvalue(char * str, int len, char *** keys, char *** values, int * num);

void
stinger_workflow_run(int argc, char ** argv, stinger_workflow_t * workflow, int print_time);


#endif  /*STINGER_WORKFLOW_H*/
