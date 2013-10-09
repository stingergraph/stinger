#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "sql_parser.h"


int64_t
parse_query (char * input, query_plan_t * query)
{
  state_t cur_state = SELECT;
  state_t next_state;
  int where_and = 0;
  int where_or = 0;
  char buf[30];
  char * token;

  token = strtok (input, " ,");

  memset(query, 0, sizeof(query_plan_t));

  while (1) {
    switch (cur_state)
    {
      case SELECT:
		    {
		      if (token && strncasecmp(token, "SELECT", 6) == 0) {
			printf(":: SELECT\n");
			token = strtok (NULL, " ,");
			next_state = FIELDS;
		      }
		      else {
			next_state = ERROR;
		      }
		    } break;

      case FIELDS:
		    {
		      printf(":: FIELDS\n");
		      if (token && 1 == sscanf(token, "%[^',']", buf)) {
			printf("%s\n", buf);
			token = strtok (NULL, " ,");
		      }

		      if (strncasecmp(buf, "FROM", 4) == 0) {
			next_state = TABLES;
		      }
		      else {
			if (add_column_to_query (query, buf) == 0)
			  next_state = FIELDS;
			else
			  next_state = ERROR;
		      }
		    } break;
      case FROM:
		    {
		    } break;

      case TABLES:
		    {
		      int valid = 0;
		      if (token && strncmp(token, "vertices", 8) == 0) {
			valid = 1;
			query->table = VERTEX_TBL;
			printf("%s\n", token);
			token = strtok (NULL, " ,");
		      }
		      else if (token && strncmp(token, "edges", 5) == 0) {
			valid = 1;
			query->table = EDGE_TBL;
			printf("%s\n", token);
			token = strtok (NULL, " ,");
		      }
		      else {
			next_state = ERROR;
		      }

		      if (valid) {
			if (token && strncasecmp(token, "WHERE", 5) == 0) {
			  token = strtok (NULL, " ,");
			  next_state = WHERE;
			}
			else if (token && strncasecmp(token, "ORDER", 5) == 0) {
			  token = strtok (NULL, " ,");
			  next_state = ORDERBY;
			}
			else if (token && strncasecmp(token, "LIMIT", 5) == 0) {
			  token = strtok (NULL, " ,");
			  next_state = LIMIT;
			}
		      }
		    } break;

      case WHERE:
		    {
		      printf(":: WHERE\n");
		      query->activate_where = 1;
		      next_state = WHERE_COL;
		    } break;

      case WHERE_COL:
		    {
		      printf(":: WHERE_COL\n");
		      printf("%s\n", token);
		      int64_t offset = query->nwhere_ops;
		      if (offset < MAX_WHERE) {
			strncpy(query->where_ops[offset].field_name, token, FIELD_LEN);
			token = strtok (NULL, " ,");
			next_state = WHERE_OPER;
		      } else {
			next_state = ERROR;
		      }
		    } break;

      case WHERE_OPER:
		    {
		      printf(":: WHERE_OPER\n");
		      printf("%s\n", token);
		      operator_t op = select_operator (token);
		      int64_t offset = query->nwhere_ops;
		      query->where_ops[offset].op = op;
		      printf("op %d\n", op);
		      token = strtok (NULL, " ,");
		      next_state = WHERE_VAL;
		    } break;

      case WHERE_VAL:
		    {
		      printf(":: WHERE_VAL\n");
		      int64_t val;
		      sscanf(token, "%lld", &val);
		      int64_t offset = query->nwhere_ops;
		      query->where_ops[offset].value = val;
		      query->nwhere_ops++;
		      printf("%ld\n", (long) val);

		      token = strtok (NULL, " ,");
		      if (token && strncasecmp(token, "ORDER", 5) == 0) {
			token = strtok (NULL, " ,");
			next_state = ORDERBY;
		      }
		      else if (token && strncasecmp(token, "LIMIT", 5) == 0) {
			token = strtok (NULL, " ,");
			next_state = LIMIT;
		      }
		      else if (token && strncasecmp(token, "AND", 3) == 0) {
			token = strtok (NULL, " ,");
			query->where_ops[offset].conditional = 1;
			next_state = WHERE_COL;
		      }
		      else if (token && strncasecmp(token, "OR", 2) == 0) {
			token = strtok (NULL, " ,");
			query->where_ops[offset].conditional = 2;
			next_state = WHERE_COL;
		      }
		      else {
			next_state = DONE;
		      }
		    } break;

      case ORDERBY:
		    {
		      int valid = 0;
		      printf(":: ORDERBY\n");
		      if (token && strncasecmp(token, "BY", 2) == 0) {
			valid = 1;
			query->activate_orderby = 1;
			token = strtok (NULL, " ,");
		      }
		      else {
			next_state = ERROR;
		      }

		      if (valid) {
			sscanf(token, "%s", buf);
			printf("%s\n", buf);
			strncpy(query->orderby_column, buf, FIELD_LEN);
			token = strtok (NULL, " ,");
			next_state = ORDERBY_DIR;
		      }
		    } break;

      case ORDERBY_DIR:
		    {
		      printf(":: ORDERBY_DIR\n");
		      if (token && strncasecmp(token, "ASC", 3) == 0) {
			printf("%s\n", token);
			query->asc = 0;
			token = strtok (NULL, " ,");
		      }
		      else if (token && strncasecmp(token, "DESC", 4) == 0) {
			printf("%s\n", token);
			query->asc = 1;
			token = strtok (NULL, " ,");
		      }
		      
		      if (token && strncasecmp(token, "LIMIT", 5) == 0) {
			next_state = LIMIT;
			token = strtok (NULL, " ,");
		      }
		      else {
			next_state = DONE;
		      }
		    } break;

      case LIMIT:
		    {
		      int64_t limit_number;
		      printf(":: LIMIT\n");
		      query->activate_limit = 1;
		      sscanf(token, "%lld", &limit_number);
		      query->limit = limit_number;
		      printf("%ld\n", (long) limit_number);

		      token = strtok (NULL, " ,");

		      if (token && strncasecmp(token, "OFFSET", 6) == 0) {
			next_state = OFFSET;
			token = strtok (NULL, " ,");
		      }
		      else {
			next_state = DONE;
		      }

		    } break;

      case OFFSET:
		    {
		      int64_t offset_number;
		      printf(":: OFFSET\n");
		      if (token) {
			sscanf(token, "%lld", &offset_number);
			query->offset = offset_number;
			printf("%ld\n", (long) offset_number);
			next_state = DONE;
		      }
		      else {
			next_state = ERROR;
		      }
		    } break;
      case DONE:
		    {
		      printf("DONE\n");
		    } break;

      case ERROR:
		    {
		      printf("ERROR\n");
		    } break;

      default:
		    {
		    } break;




    }

    if (cur_state == DONE || cur_state == ERROR)
      break;

    cur_state = next_state;
  }

  return 0;
}


operator_t
select_operator (char * token)
{
  if (strncmp(token, "BETWEEN", 7) == 0) { return BETWEEN; }
  /* NOT BETWEEN */
  if (strncmp(token, "AND", 3) == 0) { return AND; }
  if (strncmp(token, "DIV", 3) == 0) { return DIV_INT; }
  if (strncmp(token, "MOD", 3) == 0) { return MOD; }
  if (strncmp(token, "XOR", 3) == 0) { return XOR; }
  if (strncmp(token, "OR", 2) == 0) { return OR; }
  if (strncmp(token, "!=", 2) == 0) { return NOT_EQUAL; }
  if (strncmp(token, "<>", 2) == 0) { return NOT_EQUAL; }
  if (strncmp(token, "&&", 2) == 0) { return AND; }
  if (strncmp(token, "||", 2) == 0) { return OR; }
  if (strncmp(token, ">=", 2) == 0) { return GTE; }
  if (strncmp(token, "<=", 2) == 0) { return LTE; }
  if (strncmp(token, "<<", 2) == 0) { return LEFT_SHIFT; }
  if (strncmp(token, ">>", 2) == 0) { return RIGHT_SHIFT; }
  if (strncmp(token, "%", 1) == 0) { return MOD; }
  if (strncmp(token, "&", 1) == 0) { return BIT_AND; }
  if (strncmp(token, "~", 1) == 0) { return BIT_INVERT; }
  if (strncmp(token, "|", 1) == 0) { return BIT_OR; }
  if (strncmp(token, "^", 1) == 0) { return BIT_XOR; }
  if (strncmp(token, "/", 1) == 0) { return DIVIDE; }
  if (strncmp(token, "=", 1) == 0) { return EQUAL; }
  if (strncmp(token, ">", 1) == 0) { return GREATER; }
  if (strncmp(token, "<", 1) == 0) { return LESS; }
  if (strncmp(token, "-", 1) == 0) { return SUBTRACT; }
  if (strncmp(token, "+", 1) == 0) { return ADD; }
  if (strncmp(token, "*", 1) == 0) { return MULTIPLY; }

  return OP_ERROR;
}

int64_t
add_column_to_query (query_plan_t * query, char * column)
{
  int64_t offset = query->ncolumns;
  if (offset < MAX_COLUMNS) {
    strncpy (query->columns[offset].field_name, column, FIELD_LEN);
    query->ncolumns++;
    return 0;
  }
  return -1;
}

