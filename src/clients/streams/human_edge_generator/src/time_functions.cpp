#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "human_edge_generator.h"


int64_t
get_current_timestamp(void)
{
  time_t rawtime;
  struct tm * ptm;

  time (&rawtime);
  ptm = gmtime (&rawtime);

  /* YYYYMMDDHHMMSS */

  int64_t timestamp = 0;
  timestamp += (ptm->tm_year + 1900) * 10000000000;
  timestamp += (ptm->tm_mon + 1) *       100000000;
  timestamp += ptm->tm_mday *              1000000;
  timestamp += ptm->tm_hour *                10000;
  timestamp += ptm->tm_min *                   100;
  timestamp += ptm->tm_sec;

  return timestamp;
}

