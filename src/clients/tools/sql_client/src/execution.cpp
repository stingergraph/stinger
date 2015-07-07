#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "sql_parser.h"

#define SEPARATOR() printf(" | ");
#define HRULE() printf(" +----------------------------------------------------+\n");

int64_t
execute_query (query_plan_t * query, stinger_t * S)
{
  int64_t ncolumns = query->ncolumns;
  
  /* for each activated column */
  HRULE();
  for (int64_t i = 0; i < ncolumns; i++) {
    SEPARATOR();
    printf("%s", query->columns[i].field_name);
  }
  SEPARATOR();
  printf("\n");

  int64_t limit = query->limit;
  int64_t offset = query->offset;
  int64_t rows = 0;
  int64_t valid = 0;  /* track number of valid rows, for limit/offset clauses */
  int64_t nv = S->max_nv;

  if (limit == 0)
    limit = nv;

  /* evaluate each vertex to see if it meets result set criteria */
  for (int64_t i = 0; i < nv; i++) {
    int64_t outdegree = stinger_outdegree (S, i);
    int64_t indegree = stinger_indegree (S, i);
    int64_t flag = 0;  /* used to determine if the where conditions were met */

    if (!outdegree && !indegree)
      continue;  /* not an active vertex */

    if (query->activate_where) {
      /* evaluate each predicate */
      for (int64_t j = 0; j < query->nwhere_ops; j++) {
	if (query->where_ops[j].conditional == 0) {
	  if (strncmp(query->where_ops[j].field_name, "outdegree", 9) == 0) {
	    query->where_ops[j].result = evaluate_where_operator (outdegree, query->where_ops[j].op, query->where_ops[j].value);
	  }
	  else
	  if (strncmp(query->where_ops[j].field_name, "indegree", 8) == 0) {
	    query->where_ops[j].result = evaluate_where_operator (indegree, query->where_ops[j].op, query->where_ops[j].value);
	  }
	  else
	  if (strncmp(query->where_ops[j].field_name, "weight", 5) == 0) {
	    query->where_ops[j].result = evaluate_where_operator (stinger_vweight_get (S, i), query->where_ops[j].op, query->where_ops[j].value);
	  }
	  else
	  if (strncmp(query->where_ops[j].field_name, "type", 4) == 0) {
	    query->where_ops[j].result = evaluate_where_operator (stinger_vtype_get (S, i), query->where_ops[j].op, query->where_ops[j].value);
	  }
	  else
	  if (strncmp(query->where_ops[j].field_name, "id", 2) == 0) {
	    query->where_ops[j].result = evaluate_where_operator (i, query->where_ops[j].op, query->where_ops[j].value);
	  }
	}
      }

      if (flag = resolve_predicates (query))
	valid++;
    }
    else {
      /* there was no WHERE clause */
      valid++;
    }

    /* this column meets the criteria */
    if (query->activate_where && !flag)
      continue;
    if (valid > query->offset) {
      rows++;

      for (int64_t j = 0; j < ncolumns; j++) {
	SEPARATOR();
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
	  char * physID;
	  uint64_t len;
	  if (-1 == stinger_mapping_physid_direct(S, i, &physID, &len)) {
	    printf("NULL");
	  }
	  else {
	    printf("%s", physID);
	  }
	}
	else
	if (strncmp(query->columns[j].field_name, "id", 2) == 0) {
	  printf("%ld", (long) i);
	}
	
      }
      SEPARATOR();
      printf("\n");
    }

    if (rows == query->limit)
      break;
  }

  HRULE();
  printf("%ld rows in set\n", (long) rows);






  return 0;
}


int64_t
evaluate_where_operator (int64_t a, operator_t op, int64_t b) 
{
  switch (op)
  {
    case EQUAL:	  return a == b;
    case GTE:	  return a >= b;
    case GREATER: return a > b;
    case LTE:	  return a <= b;
    case LESS:	  return a < b;
    case NOT_EQUAL: return a != b;
  }

  return 0;
}
