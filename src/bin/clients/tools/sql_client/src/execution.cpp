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

  int64_t limit = query->limit;
  int64_t offset = query->offset;
  int64_t rows = 0;
  int64_t valid = 0;
  int64_t nv = STINGER_MAX_LVERTICES;

  if (limit == 0)
    limit = nv;

  for (int64_t i = 0; i < nv; i++) {
    int64_t outdegree = stinger_outdegree (S, i);
    int64_t indegree = stinger_indegree (S, i);
    int64_t flag = 0;

    if (!outdegree && !indegree)
      continue;

    if (query->activate_where) {
      for (int64_t j = 0; j < query->nwhere_ops; j++) {
	//int64_t conditional;    /* 0: nothing, 1: AND, 2: OR */
	//char field_name[FIELD_LEN];
	//operator_t op;
	//int64_t value;
	if (strncmp(query->where_ops[j].field_name, "outdegree", 9) == 0) {
	  flag = evaluate_where_operator (outdegree, query->where_ops[j].op, query->where_ops[j].value);
	}
	else
	if (strncmp(query->where_ops[j].field_name, "indegree", 8) == 0) {
	  flag = evaluate_where_operator (indegree, query->where_ops[j].op, query->where_ops[j].value);
	}
	else
	if (strncmp(query->where_ops[j].field_name, "weight", 5) == 0) {
	  flag = evaluate_where_operator (stinger_vweight_get (S, i), query->where_ops[j].op, query->where_ops[j].value);
	}
	else
	if (strncmp(query->where_ops[j].field_name, "type", 4) == 0) {
	  flag = evaluate_where_operator (stinger_vtype_get (S, i), query->where_ops[j].op, query->where_ops[j].value);
	}
	else
	if (strncmp(query->where_ops[j].field_name, "id", 2) == 0) {
	  flag = evaluate_where_operator (i, query->where_ops[j].op, query->where_ops[j].value);
	}




      }

      if (flag)
	valid++;
    }
    else {
      valid++;
    }

    /* this column meets the criteria */
    if (query->activate_where && !flag)
      continue;
    if (valid > query->offset) {
      rows++;

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
	
	if (j != ncolumns-1) SEPARATOR();
      }
      printf("\n");
    }

    if (rows == query->limit)
      break;
  }






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
