#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <time.h>
#include <netdb.h>

#include "stinger_core/stinger.h"
#include "stinger_core/formatting.h"
#include "stinger_core/xmalloc.h"
#include "stinger_utils/timer.h"

#include "netflow_stream.h"

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#define NETFLOW_BATCH_TIME 2.5
#define NETFLOW_BATCH_SIZE 10000
#define MAX_LINE 5000

using namespace gt::stinger;

int main(int argc, char *argv[])
{
	/* global options */
	int port = 10102;
	char const * hostname = NULL;

	int opt = 0;
	uint64_t batch_time = 0;
	uint64_t batch_size = 0;
	while(-1 != (opt = getopt(argc, argv, "p:a:s:t:"))) {
		switch(opt) {
			case 'p': {
				port = atoi(optarg);
			} break;

			case 'a': {
				hostname = optarg;
			} break;

			case '?':
			case 'h': {
				printf("Usage:  %s [-p port] [-a server_addr] [[-t batch_delay (seconds)]|[-s batch_size]]\n", argv[0]);
				printf("Defaults:\n\tport: %d\n\tserver: localhost\n\tdelay: %f seconds\nbatch_size: %d", port, NETFLOW_BATCH_TIME, NETFLOW_BATCH_SIZE);
				exit(0);
			} break;

			// Time between netflow batches
			case 't': {
				batch_time = atoi(optarg);
			} break;

			// Size of netflow batches
			case 's': {
				batch_size = atoi(optarg);
			} break;
		}
	}

	LOG_D_A ("Running with: port: %d", port);

	/* connect to localhost if server is unspecified */
	if(NULL == hostname) {
		hostname = "localhost";
	}

	if(batch_time == 0){
		batch_time = NETFLOW_BATCH_TIME;
	}
	if(batch_size == 0){
		batch_size = NETFLOW_BATCH_SIZE;
	}
	LOG_D_A ("Sending batches every %" PRIu64 " seconds", batch_time);

	/* start the connection */
	int sock_handle = connect_to_server (hostname, port);

	/* actually generate and send the batches */
	int64_t batch_num = 0;

	/*int line_count;*/
	char *result;
	char input_line[MAX_LINE];
	std::string line;
	StingerBatch batch;
	batch.set_make_undirected(true);
	batch.set_type(STRINGS_ONLY);
	batch.set_keep_alive(true);
	std::vector<EdgeInsertion> batch_buffer;

	int64_t insertIdx =0;
	/*int64_t edgeIdx =0;*/

	double global_start = dpc_tic();
	double start = dpc_tic();
	double timeInSeconds = dpc_toc(start);
	while((result = fgets(input_line, MAX_LINE, stdin )) != NULL) {
		NetflowParse p;
		if(p.isHeader(input_line)) {
			continue;
		}

		p.parseLine(input_line);

		/*
		// DEBUG Ensure unique entry
		std::stringstream bnum;
		bnum << batch_num;

		if(batch_num % 3 == 0) {
			p.src += bnum.str();
			p.dest += bnum.str();
		}
		// END Unique
		*/

		EdgeInsertion * insertion = batch.add_insertions();
		insertion->set_source_str(p.src);
		insertion->set_destination_str(p.dest);
		insertion->set_type_str(p.protocol);
		insertion->set_weight(p.bytes);
		insertion->set_time(p.time);
		if (ferror(stdin)) {
			perror("Error reading stdin.");
		}

		// Assume batches are separated by number of insertions if batch_time is 0 or negative
		if(batch_time <= 0) {
			if (batch_num == 0 || batch_num % batch_size == 0) {
				int batch_size = batch.insertions_size() + batch.deletions_size();
				LOG_D ("Sending message");
				LOG_D_A ("%" PRId64 ": %d actions. EX: %s:<%s, %s> (w: %d) at time %" PRId64, batch_num, batch_size, p.protocol.c_str(), p.src.c_str(), p.dest.c_str(), p.bytes, p.time);
				send_message(sock_handle, batch);
				batch.clear_insertions();
				batch_num++;
				start = dpc_tic();
			} else {
				batch_buffer.push_back(*insertion);
			}
		}
		// Process new batch of insertions after batch_time seconds
		else {
			timeInSeconds = dpc_toc(start);
			if(timeInSeconds > batch_time){
				int batch_size = batch.insertions_size() + batch.deletions_size();
				// LOG_D_A ("%f Seconds, %ld Batches", timeInSeconds, batch_num);
				LOG_D_A ("%" PRId64 ": %d actions. EX: %s:<%s, %s> (w: %d) at time %" PRId64 " after %f seconds", batch_num, batch_size, p.protocol.c_str(), p.src.c_str(), p.dest.c_str(), p.bytes, p.time, dpc_toc(start));
				send_message(sock_handle, batch);
				batch.clear_insertions();
				batch_buffer.clear();
				start = dpc_tic();
				batch_num++;
			} else {
				batch_buffer.push_back(*insertion);
			}
		}
		insertIdx++;
	}

	LOG_D_A("Sent %" PRId64 " batches in %f seconds. Constructing last batch from buffer ", batch_num, dpc_toc(global_start));

	batch.clear_insertions();
	for (std::vector<EdgeInsertion>::iterator it = batch_buffer.begin() ; it != batch_buffer.end(); ++it) {
		EdgeInsertion * insertion = batch.add_insertions();
		insertion->set_source_str(it->source_str());
		insertion->set_destination_str(it->destination_str());
		insertion->set_type_str(it->type_str());
		insertion->set_weight(it->weight());
		insertion->set_time(it->time());
		// LOG_I_A ("%s:<%s, %s> (w: %ld) at time %ld", insertion->type_str().c_str(), insertion->source_str().c_str(), insertion->destination_str().c_str(), insertion->weight(), insertion->time());
	}

	send_message(sock_handle, batch);
	LOG_D_A("Sent %zu messages from buffer. Total time: %f",batch_buffer.size(), dpc_toc(global_start));

	return 0;
}
#ifdef __APPLE__
//apple doesn't do CLOCK_MONOTONIC
double dpc_tic(void) {
    static mach_timebase_info_data_t s_timebase_info;
    if(s_timebase_info.denom == 0)
    {
        (void) mach_timebase_info(&s_timebase_info);
    }
    
    double start = ((mach_absolute_time() * s_timebase_info.numer) / (1e9 * s_timebase_info.denom));
    
    return start;
}

double dpc_toc (double start) {
    static mach_timebase_info_data_t s_timebase_info;
    
    //unlikely you'd call stop before start but put this here anyway
    if(s_timebase_info.denom == 0)
    {
        (void) mach_timebase_info(&s_timebase_info);
    }
    
    double stop = ((mach_absolute_time() * s_timebase_info.numer) / (1e9 * s_timebase_info.denom));
    
    return (stop - start);
}

#else

double dpc_tic(void) {
    double start = timer();

 	return start;
}

double dpc_toc (double start) {
        double stop = timer();

        return (stop-start);
}
#endif
