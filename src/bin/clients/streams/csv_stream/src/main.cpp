#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <time.h>
#include <netdb.h>
#include <vector>

#include "stinger_net/proto/stinger-batch.pb.h"

#include "stinger_core/stinger_error.h"

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_utils/csv.h"
#include "stinger_utils/timer.h"
#include "stinger_net/send_rcv.h"
#include "stinger_utils/stinger_sockets.h"
#include "random.h"

#include "stringfuncs.h"

using namespace gt::stinger;

#define E_A(X,...) fprintf(stderr, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__);
#define E(X) E_A(X,NULL)
#define V_A(X,...) fprintf(stdout, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__);
#define V(X) V_A(X,NULL)


int
main(int argc, char *argv[])
{
  /* global options */
  char *ins_file = 0, *del_file = 0;
  int port = 10105;
  int batch_ins = 1;
  int batch_del = 1;
  int num_batches = 1;
  struct hostent * server = NULL;
  int t_sleep = 5;
  int opt = 0;
  std::string file_type;
  int offset = 0;
  while(-1 != (opt = getopt(argc, argv, "i:d:t:o:x:y:n:s:p:a"))) {
    switch(opt) {
      case 'i': {
    ins_file = optarg;
    } break;
      case 'd': {
    del_file = optarg;
    } break;
      case 't': {
    file_type = optarg;
    if(file_type!="al" && file_type!="el")
    {
        printf("ERROR: File type (t) should be 'al' (adjacency list) or 'el' (edge list).\n");
        exit(-1);
    }
      } break;
      case 'o': {
	offset = atol(optarg);
      } break;
      case 'x': {
	batch_ins = atol(optarg);
      } break;
      case 'y': {
	batch_del = atol(optarg);
      } break;
      case 'n': {
	num_batches = atol(optarg);
      } break;
      case 's': {
	t_sleep = atoi(optarg);
      } break;
      case 'p': {
	port = atoi(optarg);
      } break;
      case 'a': {
	server = gethostbyname(optarg);
	if(NULL == server) {
	  E_A("ERROR: server %s could not be resolved.", optarg);
	  exit(-1);
	}
      } break;
      case '?':
      case 'h': {
	printf("Usage:    %s -i insert_file -d delete_file -t file_type (adjacency list/edge list) [-o offset_lines] [-x batch_insertions] [-y batch_deletions] [-n num_batches] [-p port] [-a server_addr]\n", argv[0]);
	//printf("Defaults:\n\tport: %d\n\tserver: localhost\n\tnum_vertices: %d\n-i forces the use of integers in place of strings\n", port, nv);
	exit(0);
      } break;
    }
  }

  V_A("Running with: port: %d\n", port);

  /* connect to localhost if server is unspecified */
  if(NULL == server) {
      printf("SERVER = localhost\n");
    server = gethostbyname("localhost");
    if(NULL == server) {
      E_A("ERROR: server %s could not be resolved.", "localhost");
      exit(-1);
    }
  }

  /* start the connection */
  int sock_handle = connect_to_batch_server (server, port);

  /* actually generate and send the batches */

  std::string ins_str, del_str;
  int64_t ins_src = 0, del_src = 0;
  std::vector<std::string> insertions;
  std::vector<std::string> deletions;
  std::ifstream ins (ins_file);
  std::ifstream del (del_file);
  int i_eof = 1;
  int d_eof = 1;
  if(ins.is_open()) i_eof = 0;
  if(del.is_open()) d_eof = 0;
  if (ins.is_open() || del.is_open())
  {
      printf("woogoo\n");
    for(int64_t i=0;i<offset;i++)
    {
        if(ins.is_open()) getline(ins, ins_str);
        if(del.is_open()) getline(del, del_str);
    }
    printf("woohoo\n");
    for(int64_t i=0;i<num_batches;i++)
    {
        if(i_eof == 1 && d_eof == 1) break;
        StingerBatch batch;
        batch.set_make_undirected(true);
        batch.set_type(NUMBERS_ONLY);
        batch.set_keep_alive(true);
        
        if(i_eof == 0)
        {
            for(int64_t j=0;j<batch_ins;j++)
            {
                if(insertions.size() == 0)
                {
                    ins_src++;
                    if(!getline(ins, ins_str))
                    {
                        i_eof = 1;
                        break;
                    }
                    insertions = stringsplit(ins_str);
                    printf("Insert Newline\n");
                }
                EdgeInsertion * insertion = batch.add_insertions();
                
                std::string src_str;
                std::string dest_str = insertions.back();
                insertion->set_destination(atol(dest_str.c_str()));
                insertion->set_destination_str(dest_str);
                insertions.pop_back();
                
                if(file_type == "el")
                {
                    src_str = insertions.front();
                    insertion->set_source(atol(src_str.c_str()));
                    insertion->set_source_str(src_str);
                    insertions.pop_back();
                }
                else
                {
                    insertion->set_source(ins_src);
                    std::ostringstream src_stream;
                    src_stream << ins_src;
                    src_str = src_stream.str();
                    insertion->set_source_str(src_str);
                }
                  
                insertion->set_weight(1);
                insertion->set_time(i*batch_ins+j);
                printf("Inserting <%s, %s>\n", src_str.c_str(), dest_str.c_str());
            }
        }
        
        if(d_eof == 0)
        {
            for(int64_t j=0;j<batch_del;j++)
            {
                if(deletions.size() == 0)
                {
                    del_src++;
                    if(!getline(del, del_str))
                    {
                        d_eof = 1;
                        break;
                    }
                    deletions = stringsplit(del_str);
                    printf("Delete Newline\n");
                }
                EdgeDeletion * deletion = batch.add_deletions();
                
                std::string src_str;
                std::string dest_str = deletions.back();
                deletion->set_destination(atol(dest_str.c_str()));
                deletion->set_destination_str(dest_str);
                deletions.pop_back();
                
                if(file_type == "el")
                {
                    src_str = deletions.front();
                    deletion->set_source(atol(src_str.c_str()));
                    deletion->set_source_str(src_str);
                    deletions.pop_back();
                }
                else
                {
                    deletion->set_source(del_src);
                    std::ostringstream src_stream;
                    src_stream << del_src;
                    src_str = src_stream.str();
                    deletion->set_source_str(src_str);
                }
                
                printf("Deleting <%s, %s>\n", src_str.c_str(), dest_str.c_str());
            }
        }
        
        int64_t total_actions = batch.insertions_size() + batch.deletions_size();
        if(total_actions > 0)
        {
            V("Sending messages.");
            
            send_message(sock_handle, batch);
            sleep(t_sleep);
        }
    }
  }

  ins.close();
  del.close();
  
  StingerBatch batch;
  batch.set_make_undirected(true);
  batch.set_type(NUMBERS_ONLY);
  batch.set_keep_alive(false);
  send_message(sock_handle, batch);

  return 0;
}
