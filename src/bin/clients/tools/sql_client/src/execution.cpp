#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "stinger_core/stinger.h"
#include "sql_parser.h"

#define SEPARATOR() printf(" | ");

int64_t
execute_query (query_plan_t * query, stinger_t * S)
{
  int64_t ncolumns = query->ncolumns;
  
  /* for each activated column */
  for (int64_t i = 0; i < ncolumns; i++) {
    printf("%s", query->columns[i].field_name);
    if (i != ncolumns-1) SEPARATOR();
  }
  printf("\n");

  int64_t nv = STINGER_MAX_LVERTICES;

  for (int64_t i = 0; i < nv; i++) {
    int64_t outdegree = stinger_outdegree (S, i);
    int64_t indegree = stinger_indegree (S, i);

    if (!outdegree && !indegree)
      continue;

    for (int64_t j = 0; j < ncolumns; j++) {
      if (strncmp(query->columns[j].field_name, "outdegree", 9) == 0) {
	printf("%ld", (long) outdegree);
      }
      else
      if (strncmp(query->columns[j].field_name, "indegree", 8) == 0) {
	printf("%ld", (long) indegree);
      }
      else
      if (strncmp(query->columns[j].field_name, "weight", 5) == 0) {
	printf("%ld", (long) stinger_vweight_get (S, i));
      }
      else
      if (strncmp(query->columns[j].field_name, "type", 4) == 0) {
	printf("%ld", (long) stinger_vtype (S, i));
      }
      else
      if (strncmp(query->columns[j].field_name, "name", 4) == 0) {
	printf("TODO");
      }
      else
      if (strncmp(query->columns[j].field_name, "id", 2) == 0) {
	printf("%ld", (long) i);
      }
      
      if (j != ncolumns-1) SEPARATOR();
    }
  }




  /* is there a where clause? */
  if (query->activate_where) {
    printf("WHERE ");
    for (int64_t i = 0; i < query->nwhere_ops; i++) {
      printf("%s %d %ld ", query->where_ops[i].field_name, query->where_ops[i].op, (long) query->where_ops[i].value);
      if (query->where_ops[i].conditional == 1)
	printf("AND ");
      if (query->where_ops[i].conditional == 2)
	printf("OR ");
    }
  }

  /* is there an order by clause? */
  if (query->activate_orderby) {
    printf("ORDER BY ");
    printf("%s ", query->orderby_column);
    if (query->asc == 0)
      printf("ASC ");
    if (query->asc == 1)
      printf("DESC ");
  }

  /* is there a limit clause */
  if (query->activate_limit) {
    printf("LIMIT %ld OFFSET %ld ", (long) query->limit, (long) query->offset);
  }

  return 0;
}
