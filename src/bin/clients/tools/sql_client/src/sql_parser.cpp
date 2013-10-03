#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "stinger_core/stinger.h"
#include "stinger_net/mon_handling.h"
#include "stinger_net/stinger_mon.h"
#include "sql_parser.h"

using namespace gt::stinger;


int main (int argc, char *argv[])
{
  query_plan_t query;

  char input[100] = "SELECT name,type,weight FROM vertices WHERE weight > 2 ORDER BY name LIMIT 10";
  printf("input string is:[%s]\n", input);

  mon_connect(10103, "localhost", "sql_client");
  StingerMon & mon = StingerMon::get_mon();

  int64_t num_algs = mon.get_num_algs();
  printf("num_algs = %ld\n", (long) num_algs);
  
  /* Parse the query and form a plan */
  parse_query (input, &query);

  printf("*** testing ***\n");
  print_query_plan (&query);

  /* Get the STINGER pointer and prepare to execute the plan */
  mon.get_alg_read_lock();

  stinger_t * S = mon.get_stinger();

  /* Execute the plan */
  execute_query (&query, S);

  /* Release the lock */
  mon.release_alg_read_lock();

  /* Return the results */

  return 0;
}


