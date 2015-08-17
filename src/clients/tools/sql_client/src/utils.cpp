#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "sql_parser.h"

void print_query_plan (query_plan_t * query)
{
  printf("SELECT ");

  for (int64_t i = 0; i < query->ncolumns; i++) {
    printf("%s", query->columns[i].field_name);
    if (i != query->ncolumns-1)
      printf(",");
    printf(" ");
  }

  printf("FROM ");
  if (query->table == VERTEX_TBL)
    printf("vertices ");
  if (query->table == EDGE_TBL)
    printf("edges ");

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

  if (query->activate_orderby) {
    printf("ORDER BY ");
    printf("%s ", query->orderby_column);
    if (query->asc == 0)
      printf("ASC ");
    if (query->asc == 1)
      printf("DESC ");
  }

  if (query->activate_limit) {
    printf("LIMIT %ld OFFSET %ld ", (long) query->limit, (long) query->offset);
  }

  printf("\n");
}
