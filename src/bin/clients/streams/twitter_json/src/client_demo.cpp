#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <netdb.h>

extern "C" {
#include "stinger_core/stinger.h"
#include "stinger_utils/csv.h"
#include "stinger_utils/timer.h"
}

#include "twitter_json.h"

#define RAPIDJSON_ASSERT(X) if (!(X)) { throw std::exception(); }


using namespace gt::stinger;

#define E_A(X,...) fprintf(stderr, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__);
#define E(X) E_A(X,NULL)
#define V_A(X,...) fprintf(stdout, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__);
#define V(X) V_A(X,NULL)

enum csv_fields {
  FIELD_IS_DELETE,
  FIELD_SOURCE,
  FIELD_DEST,
  FIELD_WEIGHT,
  FIELD_TIME
};

/* convert month string into numeric (assumes capitalization) */
int64_t month(const char * month) {
  switch(month[0]) {
    case 'J': {
      if(month[1] == 'a') {
	return 0;
      } else if(month[2] == 'n') {
	return 151;
      } else {
	return 181;
      }
    } break;
    case 'F': {
      return 31;
    } break;
    case 'M': {
      if(month[2] == 'r') {
        return 59;
      } else {
	return 120;
      }
    } break;
    case 'A': {
      if(month[1] == 'p') {
	return 90;
      } else {
	return 212;
      }
    } break;
    case 'S': {
      return 242;
    } break;
    case 'O': {
      return 273;
    } break;
    case 'N': {
      return 304;
    } break;
    case 'D': {
      return 334;
    } break;

  }
}

#define CHAR2INT(X) ((X) - '0')
/* convert a twitter timestamp string into a simplistic int64 representation
 * that is readable when printed and quick to create: YYYYMMDDHHMMSS */
int64_t
twitter_timestamp(const char * time)
{
  /*
  Mon Sep 24 03:35:21 +0000 2012
  012345678901234567890123456789
  0         1         2
  */
		/* year */
  return CHAR2INT(time[26]) * 10000000000000 +
	 CHAR2INT(time[27]) *  1000000000000 +
	 CHAR2INT(time[28]) *   100000000000 +
	 CHAR2INT(time[29]) *    10000000000 +
	 /* month */
	 month(time + 4) *         100000000 +
	 /* day */
	 CHAR2INT(time[8]) *        10000000 +
	 CHAR2INT(time[9]) *         1000000 +
	 /* hour */
	 CHAR2INT(time[11]) *         100000 +
	 CHAR2INT(time[12]) *          10000 +
	 /* minute */
	 CHAR2INT(time[14]) *            1000 +
	 CHAR2INT(time[15]) *             100 +
	 /* second */
	 CHAR2INT(time[17]) *              10 +
	 CHAR2INT(time[18]);
}

int64_t
since_month(const char * time)
{
  /*
  Mon Sep 24 03:35:21 +0000 2012
  012345678901234567890123456789
  0         1         2
  */
  return 
	 /* month */
	 month(time + 4) *  86400 +
	 /* day */
	 (CHAR2INT(time[8]) * 10 + CHAR2INT(time[9])) * 86400 +
	 /* hour */
	 (CHAR2INT(time[11]) * 10 + CHAR2INT(time[12])) * 3600 +
	 /* minute */
	 (CHAR2INT(time[14]) * 10 + CHAR2INT(time[15])) * 60 + 
	 /* second */
	 (CHAR2INT(time[17]) * 10 + CHAR2INT(time[18]));
}

int
main(int argc, char *argv[])
{
  /* global options */
  int64_t input_rate = 6;
  int64_t output_rate = 1;
  int src_port = 10102;
  int dst_port = 10101;
  int batch_size = 100000;
  int num_batches = -1;
  int is_json = 0;
  uint64_t buffer_size = 1ULL << 28ULL;
  int use_strings = 0;
  struct sockaddr_in server_addr = { 0 };
  struct hostent * server = NULL;
  char * filename = NULL;

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "p:b:d:a:sjx:y:i:o:"))) {
    switch(opt) {
      case 'p': {
	src_port = atoi(optarg);
      } break;

      case 'd': {
	dst_port = atoi(optarg);
      } break;

      case 'i': {
	input_rate = atoi(optarg);
      } break;

      case 'o': {
	output_rate = atoi(optarg);
      } break;

      case 'b': {
	buffer_size = atol(optarg);
      } break;

      case 'x': {
	batch_size = atol(optarg);
      } break;

      case 'y': {
	num_batches = atol(optarg);
      } break;

      case 'j': {
	is_json = 1;
      } break;

      case 'a': {
	server = gethostbyname(optarg);
	if(NULL == server) {
	  E_A("ERROR: server %s could not be resolved.", optarg);
	  exit(-1);
	}
      } break;

      case 's': {
	use_strings = 1;
      } break;

      case '?':
      case 'h': {
	printf("Usage:    %s [-p src_port] [-d dst_port] [-a server_addr] [-b buffer_size] [-s for strings] [-x batch_size] [-y num_batches] [-i in_seconds] [-o out_seconds] [-j] [<input file>]\n", argv[0]);
	printf("Defaults: src_port: %d dest_port: %d server: localhost buffer_size: %lu use_strings: %d\n", src_port, dst_port, (unsigned long) buffer_size, use_strings);
	printf("\nCSV file format is is_delete,source,dest,weight,time where weight and time are\n"
		 "optional 64-bit integers and source and dest are either strings or\n"
		 "64-bit integers depending on options flags. is_delete should be 0 for edge\n"
		 "insertions and 1 for edge deletions.\n"
		 "\n"
		 "-j turns on JSON parsing mode, which expects a JSON twitter file rather than CSV\n"
		 "   this file will be turned into a graph of mentions\n"
		 "-i controls how many seconds of the input file will be read into one batch\n"
		 "-o controls how often batches will be sent in seconds\n"
		 "   using both -i and -o allows you to control the playback rate of the file (with -i 1 -o 1\n"
		 "   being read one second of Twitter data and send it once per second)\n");
	exit(0);
      } break;
    }
  }

  if (optind < argc && 0 != strcmp (argv[optind], "-"))
    filename = argv[optind];

  V_A("Running with: src_port: %d dst_port: %d buffer_size: %lu string-vertices: %d\n", src_port, dst_port, (unsigned long) buffer_size, use_strings);

  if(NULL == server) {
    server = gethostbyname("localhost");
    if(NULL == server) {
      E_A("ERROR: server %s could not be resolved.", "localhost");
      exit(-1);
    }
  }

  server_addr.sin_family = AF_INET;
  memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  server_addr.sin_port = dst_port;

  int sock_handle;
  struct sockaddr_in sock_addr = { AF_INET, (in_port_t)src_port, 0};

  if(-1 == (sock_handle = socket(AF_INET, SOCK_STREAM, 0))) {
    perror("Socket create failed");
    exit(-1);
  }

  /*if(-1 == bind(sock_handle, (const sockaddr *)&sock_addr, sizeof(sock_addr))) {
    perror("Socket bind failed");
    exit(-1);
  }*/

  /*if(-1 == listen(sock_handle, 1)) {
    perror("Socket listen failed");
    exit(-1);
  }*/

  uint8_t * buffer = (uint8_t *)malloc(buffer_size);
  if(!buffer) {
    perror("Buffer alloc failed");
    exit(-1);
  }

  if(-1 == connect(sock_handle, (const sockaddr *)&server_addr, sizeof(server_addr))) {
    perror("Connection failed");
    exit(-1);
  }


  FILE * fp = (filename? fopen(filename, "r") : stdin);
  if (!fp) {
    char errmsg[257];
    snprintf (errmsg, 256, "Opening \"%s\" failed", filename);
    errmsg[256] = 0;
    perror (errmsg);
    exit (-1);
  }

  if(!is_json) {
    char * buf = NULL, ** fields = NULL;
    uint64_t bufSize = 0, * lengths = NULL, fieldsSize = 0, count = 0;
    int64_t line = 0;
    int batch_num = 0;

    StingerBatch batch;
    batch.set_make_undirected(true);
    batch.set_type(use_strings ? STRINGS_ONLY : NUMBERS_ONLY);

    while(!feof(fp)) {
      V("Parsing messages.");

      for(int e = 0; e < batch_size && !feof(fp); e++) {
	line++;
	readCSVLineDynamic(',', fp, &buf, &bufSize, &fields, &lengths, &fieldsSize, &count);

	if(count <= 1)
	  continue;
	if(count < 3) {
	  E_A("ERROR: too few elements on line %ld", (long) line);
	  continue;
	}

	/* is insert? */
	if(fields[FIELD_IS_DELETE][0] == '0') {
	  EdgeInsertion * insertion = batch.add_insertions();

	  if(use_strings) {
	    insertion->set_source_str(fields[FIELD_SOURCE]);
	    insertion->set_destination_str(fields[FIELD_DEST]);
	  } else {
	    insertion->set_source(atol(fields[FIELD_SOURCE]));
	    insertion->set_destination(atol(fields[FIELD_DEST]));
	  }

	  if(count > 3) {
	    insertion->set_weight(atol(fields[FIELD_WEIGHT]));
	    if(count > 4) {
	      insertion->set_time(atol(fields[FIELD_TIME]));
	    }
	  }

	} else {
	  EdgeDeletion * deletion = batch.add_deletions();

	  if(use_strings) {
	    deletion->set_source_str(fields[FIELD_SOURCE]);
	    deletion->set_destination_str(fields[FIELD_DEST]);
	  } else {
	    deletion->set_source(atol(fields[FIELD_SOURCE]));
	    deletion->set_destination(atol(fields[FIELD_DEST]));
	  }
	}
      }

      V("Sending messages.");

      std::string out_buffer;

      batch.SerializeToString(&out_buffer);

      int64_t len = out_buffer.size();

      write(sock_handle, &len, sizeof(int64_t));
      write(sock_handle, out_buffer.c_str(), len);

      if((batch_num >= num_batches) && (num_batches == -1)) {
	break;
      } else {
	batch_num++;
      }
    }

    int64_t len = -1;
    write(sock_handle, &len, sizeof(int64_t));
  } else {
    char * line = NULL;
    size_t line_len = 0;

    int batch_num = 0;

    int64_t batch_start = 0;

    tic();

    while(!feof(fp)) {

      V("Parsing messages.");

      rapidjson::Document d;
      StingerBatch batch;
      batch.set_make_undirected(true);
      batch.set_type(STRINGS_ONLY);
      batch.set_keep_alive(true);

      int64_t tweet_time = 0;
      int64_t e = 0;
      V_A("%ld %ld %ld", batch_num, batch_start, tweet_time);

      for(; (tweet_time - batch_start) < input_rate && !feof(fp); e++) {
	getline(&line, &line_len, fp);
	try {
	  d.Parse<0>(line);
	  tweet_time = since_month(d["created_at"].GetString());

	  if(!batch_start)
	    batch_start = tweet_time;

	  if(!d.HasMember("delete")) {
	    for(int m = 0; m < d["entities"]["user_mentions"].Size(); m++) {
	      EdgeInsertion * insertion = batch.add_insertions();
	      insertion->set_source_str(d["user"]["screen_name"].GetString());
	      insertion->set_destination_str(d["entities"]["user_mentions"][m]["screen_name"].GetString());
	      insertion->set_weight(1);
	      insertion->set_time(tweet_time);
	    }
	  }
	} catch (std::exception e) {
	  /* pass */
	}
      }

      batch_start = tweet_time;
      V_A("%ld %ld %ld", batch_num, batch_start, tweet_time);

      V_A("Sending %ld tweets.", e);
      //batch.PrintDebugString();

      int64_t micro = output_rate * 1000000 - toc();
      if(micro > 0) {
	V_A("Sleeping for %ld", micro);
	usleep(micro);
      } else {
	E_A("Can't keep up! %ld", micro);
      }

      V_A("Sending size %d", batch.ByteSize());

      send_message(sock_handle, batch);

      if((batch_num >= num_batches) && (num_batches != -1)) {
	break;
      } else {
	batch_num++;
      }
    }

    StingerBatch batch;
    batch.set_make_undirected(true);
    batch.set_type(STRINGS_ONLY);
    batch.set_keep_alive(false);
    send_message(sock_handle, batch);
  }

  free(buffer);
  return 0;
}

extern "C" {
void
grotty_nasty_stats_hack (char *ugh)
{
  *ugh = 0;
}
}
