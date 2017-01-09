/* -*- mode: C; mode: folding; fill-column: 70; -*- */
#define _XOPEN_SOURCE 600
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(_OPENMP)
#include "omp.h"
#endif


#include "stinger_core/stinger_atomics.h"
#include "stinger_utils/stinger_utils.h"
#include "stinger_core/stinger.h"
#include "stinger_utils/timer.h"
#include "stinger_core/xmalloc.h"

#define ACTI(k) (action[2*(k)])
#define ACTJ(k) (action[2*(k)+1])

#define PERCENTAGE_DEFAULT 20.0


/* handles for I/O memory */
static int64_t * restrict graphmem;
static int64_t * restrict actionmem;

char * dimacs_graph_name;
char * stinger_graph_name = INITIAL_GRAPH_NAME_DEFAULT;
char * stinger_action_name = ACTION_STREAM_NAME_DEFAULT;


static double percentage=0.5;


static struct stinger * S;

static double * update_time_trace;


void usage (FILE * out, char *progname)
{

  fprintf (out,
           "%s [--percentage|-p #] dimacs-graph-name [stinger-graph-name.bin] [stinger-action-stream.bin]\n"
           "\tDefaults:\n"
           "\t   percentage of undirected edges, default vale = %d\n"
           "\t   dimacs initial-graph name = User input\n"           
           "\t   stinger graph name, default vale = \"%s\"\n"
           "\t   stinger actions stream name, default vale = \"%s\"\n",
           progname,
           PERCENTAGE_DEFAULT, 
           INITIAL_GRAPH_NAME_DEFAULT, ACTION_STREAM_NAME_DEFAULT);

}

/**
* @brief Parses command line arguments.
*
* Parses the command line input as given by usage().  Batch size, number of
* batches, initial graph filename, and action stream filename are given by
* to the caller if they were specified on the command line.
*
* @param argc The number of arguments
* @param argv[] The array of arguments
* @param initial_graph_name Path/filename of the initial input graph on disk
* @param action_stream_name Path/filename of the action stream on disk
* @param batch_size Number of edge actions to consider in one batch
* @param nbatch Number of batchs to process
*/
void
parse_args_dimacs (const int argc, char *argv[], char ** input_graph_name,
            char **output_graph_name, char **output_action_name,
            double* percentage)
{
  int k = 1;
  float seenPercentage=0;
  if (k >= argc)
    return;
  while (k < argc && argv[k][0] == '-') {
    if (0 == strcmp (argv[k], "--percentage") || 0 == strcmp (argv[k], "-p")) {
      if (seenPercentage)
        goto err;
      seenPercentage = 1;
      ++k;
      if (k >= argc)
        goto err;
      *percentage = strtod (argv[k], NULL)/100 ;
      if (seenPercentage > 1.0 || seenPercentage<0)
        goto err;
      ++k;
    } else if (0 == strcmp (argv[k], "--help")
             || 0 == strcmp (argv[k], "-h") || 0 == strcmp (argv[k], "-?")) {
      usage (stdout, argv[0]);
      exit (EXIT_SUCCESS);
      return;
    } else if (0 == strcmp (argv[k], "--")) {
      ++k;
      break;
    }
  }
  if (k < argc)
    *input_graph_name = argv[k++];
  if (k < argc)
    *output_graph_name = argv[k++];
  if (k < argc)
    *output_action_name = argv[k++];
  return;
 err:
  usage (stderr, argv[0]);
  exit (EXIT_FAILURE);
  return;
}

void readGraphDIMACS(char* filePath, int64_t** prmoff, int64_t** prmind, int64_t* prmnv, int64_t* prmne){
  FILE *fp = fopen (filePath, "r");
  int64_t nv,ne;

  char* line=NULL;
  // Read data from file
  int64_t temp,lineRead;
  size_t bytesRead=0;
  getline (&line, &bytesRead, fp);  

  sscanf (line, "%ld %ld", &nv, &ne);

  free(line);
  int64_t * off = (int64_t *) malloc ((nv + 2) * sizeof (int64_t));
  int64_t * ind = (int64_t *) malloc ((ne * 2) * sizeof (int64_t));
  off[0] = 0;
  off[1] = 0;
  int64_t counter = 0;
  int64_t u;
  line=NULL;
  bytesRead=0;

  //    for (u = 1; fgets (line, &bytesRead, fp); u++)
  for (u = 1; (temp=getline (&line, &bytesRead, fp))!=-1; u++){   
    int64_t neigh = 0;
    int64_t v = 0;
    char *ptr = line;
    int read = 0;
    char tempStr[1000];
    lineRead=0;
    while (lineRead<bytesRead && (read=sscanf (ptr, "%s", tempStr)) > 0){
      v=atoi(tempStr);
      read=strlen(tempStr);
      ptr += read+1;
      lineRead=read+1;
      neigh++;
      ind[counter++] = v;
    }
    off[u + 1] = off[u] + neigh;
    free(line);   
    bytesRead=0;
  }
  fclose (fp);

  nv++;
  ne*=2;
  *prmnv=nv;
  *prmne=ne;
  *prmind=ind;
  *prmoff=off;
}




int main (const int argc, char *argv[])
{
  parse_args_dimacs (argc, argv, &dimacs_graph_name, &stinger_graph_name, &stinger_action_name, &percentage );
  STATS_INIT();


  int64_t nv, ne, naction;
  int64_t *off,*ind;

  readGraphDIMACS(dimacs_graph_name,&off,&ind,&nv,&ne);

  PRINT_STAT_INT64("DIMACS nv", nv);
  PRINT_STAT_INT64("DIMACS (directed) ne", ne);

  int64_t *soff,*sind,*actionEdgeList, *edgeInArray;

  soff = (int64_t *) malloc ((nv + 1) * sizeof (int64_t));
  sind = (int64_t *) malloc ((ne) * sizeof (int64_t));
  actionEdgeList = (int64_t *) malloc ((2*ne) * sizeof (int64_t));
  edgeInArray = (int64_t *) malloc ((ne) * sizeof (int64_t));

  memset(edgeInArray,0, sizeof(int64_t)*ne);

  int64_t actionCounter=0, graphCounter=0;
  int64_t threshhold=ne*percentage/2; // Dividing by two to take into account directed edges.
  time_t t; srand((unsigned) time(&t));

  while(graphCounter<threshhold){

    for (int64_t src = 0; src < nv; src++){
      if(graphCounter>=threshhold)
        break;
      for(int64_t iter=off[src]; iter<off[src+1]; iter++){
        int64_t dest=ind[iter];
        int64_t temp = ((int64_t)rand()+(int64_t)(rand())<<31) % ne;
        if (temp<=threshhold && graphCounter<threshhold)
        {
          // Checking if it has already been marked by its opposite directed edge
          if(edgeInArray[iter]==0){
            // Going over all the adjacencies of the vertex to find opposite edge
            for(int64_t w=off[dest]; w<off[dest+1];w++){
              if(ind[w]==src){
                if(edgeInArray[ind[w]]==0){
                  edgeInArray[iter]=1;
                  edgeInArray[w]=1;
                  graphCounter+=2; // Remember, undirected graph
                }
                break; // Search is no longer necessary.
              }
            }
          }
        }
      }
    }
  }

  soff[0]=0;
  graphCounter=0;
  for (int64_t src = 0; src < nv; src++){
    soff[src+1]=soff[src];
    for(int64_t iter=off[src]; iter<off[src+1]; iter++){
      int64_t dest=ind[iter];
  
      if (edgeInArray[iter]){
        sind[graphCounter++]=dest;
        soff[src+1]++;
      }
      else{
        actionEdgeList[actionCounter]=src;
        actionEdgeList[actionCounter+1]=dest;
        actionCounter+=2;
      }
    }
  }
  actionCounter/=2;


  int64_t temp[2];
  for(int64_t a=0;a<actionCounter; a++){
    int64_t pos=(((int64_t)(rand()))<<31) % actionCounter;
    temp[0] = actionEdgeList[a*2]; temp[1] = actionEdgeList[a*2+1];
    actionEdgeList[a*2]   = actionEdgeList[pos*2]; actionEdgeList[a*2+1] = actionEdgeList[pos*2+1];
    actionEdgeList[pos*2] = temp[0]; actionEdgeList[pos*2+1] = temp[1];
  }

  FILE* fdgraph= fopen (stinger_graph_name, "w");
  if(fdgraph==NULL){
      printf("error number is :%d \n",errno);
      perror ("Error opening initial graph");
      abort ();
  }

  // Save DIMACS graph in STINGER format
  // Note, for each edge e=(u,v) in the graph, the edge e'=(v,u) is also in the graph.

  PRINT_STAT_INT64("STINGER nv", nv);
  PRINT_STAT_INT64("STINGER (directed) ne", graphCounter);

  memset(edgeInArray,0, sizeof(int64_t)*ne);
  uint64_t dummy=0x1234ABCDul;
  fwrite(&dummy         , sizeof(int64_t),1,fdgraph);
  fwrite(&nv           , sizeof(int64_t),1,fdgraph);
  fwrite(&graphCounter , sizeof(int64_t),1,fdgraph);
  fwrite(soff          , sizeof(int64_t),nv+1,fdgraph);
  fwrite(sind          , sizeof(int64_t),graphCounter,fdgraph);
  fwrite(edgeInArray   , sizeof(int64_t),graphCounter,fdgraph);
  fclose(fdgraph);

  // Save action stream in STINGER format.
  // Note, for each edge e=(u,v) in the stream, the edge e'=(v,u) is also in the stream.

  PRINT_STAT_INT64("STINGER undirected actions", actionCounter/2);
  PRINT_STAT_INT64("STINGER   directed actions", actionCounter);

  FILE* fdaction= fopen (stinger_action_name, "w");
  fwrite(&dummy         , sizeof(int64_t),1,fdaction);
  fwrite(&actionCounter , sizeof(int64_t),1,fdaction);
  fwrite(actionEdgeList , sizeof(int64_t),actionCounter*2,fdaction);

  free(edgeInArray);

  free(actionEdgeList);
  free(soff);
  free(sind);

  free(off);
  free(ind);

  STATS_END();
}
