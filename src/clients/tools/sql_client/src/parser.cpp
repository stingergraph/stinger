#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stack>

#include "stinger_core/stinger_error.h"
#include "sql_parser.h"


int64_t
parse_query (char * input, query_plan_t * query)
{
  state_t cur_state = SELECT;
  state_t next_state;
  /*int where_and = 0;
  int where_or = 0;*/
  char buf[30];
  char * token;
  char * saveptr;

  token = strtok_r (input, " ,", &saveptr);

  memset(query, 0, sizeof(query_plan_t));

  while (1) {
    switch (cur_state)
    {
      case SELECT:
		    {
		      if (token && strncasecmp(token, "SELECT", 6) == 0) {
			//printf(":: SELECT\n");
			token = strtok_r (NULL, " ,", &saveptr);
			next_state = FIELDS;
		      }
		      else {
			next_state = ERROR;
		      }
		    } break;

      case FIELDS:
		    {
		      //printf(":: FIELDS\n");
		      if (token && 1 == sscanf(token, "%[^',']", buf)) {
			//printf("%s\n", buf);
			token = strtok_r (NULL, " ,", &saveptr);
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
			//printf("%s\n", token);
			token = strtok_r (NULL, " ,", &saveptr);
		      }
		      else if (token && strncmp(token, "edges", 5) == 0) {
			valid = 1;
			query->table = EDGE_TBL;
			//printf("%s\n", token);
			token = strtok_r (NULL, " ,", &saveptr);
		      }
		      else {
			next_state = ERROR;
		      }

		      if (valid) {
			if (token && strncasecmp(token, "WHERE", 5) == 0) {
			  //token = strtok_r (NULL, " ,", &saveptr);
			  next_state = WHERE;
			}
			else if (token && strncasecmp(token, "ORDER", 5) == 0) {
			  token = strtok_r (NULL, " ,", &saveptr);
			  next_state = ORDERBY;
			}
			else if (token && strncasecmp(token, "LIMIT", 5) == 0) {
			  token = strtok_r (NULL, " ,", &saveptr);
			  next_state = LIMIT;
			}
		      }
		    } break;

      case WHERE:
		    {
		      //printf(":: WHERE\n");
		      query->activate_where = 1;
		      next_state = WHERE_COL;
		    } break;

      case WHERE_COL:
		    {
		      //printf(":: WHERE_COL\n");
		      //printf("%s\n", token);
		      parse_where_expr (saveptr, query);

		      token = strtok_r (NULL, " ,", &saveptr);
		      next_state = WHERE_VAL;
		    } break;

      case WHERE_OPER:
		    {
		      next_state = ERROR;
		    } break;

      case WHERE_VAL:
		    {
		      //printf(":: WHERE_VAL\n");

		      while ((token = strtok_r (NULL, " ,", &saveptr))) {
			if (token && strncasecmp(token, "ORDER", 5) == 0) {
			  token = strtok_r (NULL, " ,", &saveptr);
			  next_state = ORDERBY;
			  break;
			}
			else if (token && strncasecmp(token, "LIMIT", 5) == 0) {
			  token = strtok_r (NULL, " ,", &saveptr);
			  next_state = LIMIT;
			  break;
			}
		      }
		      
		      if (!token)
			next_state = DONE;
		    } break;

      case ORDERBY:
		    {
		      int valid = 0;
		      //printf(":: ORDERBY\n");
		      if (token && strncasecmp(token, "BY", 2) == 0) {
			valid = 1;
			query->activate_orderby = 1;
			token = strtok_r (NULL, " ,", &saveptr);
		      }
		      else {
			next_state = ERROR;
		      }

		      if (valid) {
			sscanf(token, "%s", buf);
			//printf("%s\n", buf);
			strncpy(query->orderby_column, buf, FIELD_LEN);
			token = strtok_r (NULL, " ,", &saveptr);
			next_state = ORDERBY_DIR;
		      }
		    } break;

      case ORDERBY_DIR:
		    {
		      //printf(":: ORDERBY_DIR\n");
		      if (token && strncasecmp(token, "ASC", 3) == 0) {
			//printf("%s\n", token);
			query->asc = 0;
			token = strtok_r (NULL, " ,", &saveptr);
		      }
		      else if (token && strncasecmp(token, "DESC", 4) == 0) {
			//printf("%s\n", token);
			query->asc = 1;
			token = strtok_r (NULL, " ,", &saveptr);
		      }
		      
		      if (token && strncasecmp(token, "LIMIT", 5) == 0) {
			next_state = LIMIT;
			token = strtok_r (NULL, " ,", &saveptr);
		      }
		      else {
			next_state = DONE;
		      }
		    } break;

      case LIMIT:
		    {
		      int64_t limit_number;
		      //printf(":: LIMIT\n");
		      query->activate_limit = 1;
		      sscanf(token, "%" SCNd64, &limit_number);
		      query->limit = limit_number;
		      //printf("%ld\n", (long) limit_number);

		      token = strtok_r (NULL, " ,", &saveptr);

		      if (token && strncasecmp(token, "OFFSET", 6) == 0) {
			next_state = OFFSET;
			token = strtok_r (NULL, " ,", &saveptr);
		      }
		      else {
			next_state = DONE;
		      }

		    } break;

      case OFFSET:
		    {
		      int64_t offset_number;
		      //printf(":: OFFSET\n");
		      if (token) {
			sscanf(token, "%" SCNd64, &offset_number);
			query->offset = offset_number;
			//printf("%ld\n", (long) offset_number);
			next_state = DONE;
		      }
		      else {
			next_state = ERROR;
		      }
		    } break;
      case DONE:
		    {
		      //printf("DONE\n");
		    } break;

      case ERROR:
		    {
		      //printf("ERROR\n");
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


int64_t
parse_where_expr (char * expr, query_plan_t * query)
{
  char buf[FIELD_LEN];
  char op_str[3];
  int64_t value;
  char * token;
  /*char * saveptr;*/
  //printf("expr is: %s\n", expr);

  std::stack<int> mystack;
  int64_t offset;


  token = expr;
  while (token[0] != '\0' && strncasecmp(token, "ORDER", 5) != 0 && strncasecmp(token, "LIMIT", 5) != 0) {
    /* 1: (
       2: )
       3: AND
       4: OR
       */

    /* if the item is an operator */
    if (strncasecmp(token, "AND", 3) == 0) {
      mystack.push(3);
      token += 4;
    }

    else if (strncasecmp(token, "OR", 2) == 0) {
      while (!mystack.empty() && (mystack.top() == 3)) {
	//printf("AND\n");
	mystack.pop();

	/* add the AND operator into the query plan */
	offset = query->nwhere_ops;
	if (offset < MAX_WHERE) {
	  query->where_ops[offset].conditional = 1;
	  query->nwhere_ops++;
	}
      }
      mystack.push(4);
      token += 3;
    }

    /* if the item is a "number", add it directly */
    else if (token[0] != '(' && token[0] != ')') {
      sscanf(token, "%[^'()=<>'] %[^0123456789] %" SCNd64, buf, op_str, &value);
      operator_t op = select_operator (op_str);
      //printf("field_name: %s, %s, %ld\n", buf, op_str, value);

      /* copy the operation into the query plan */
      offset = query->nwhere_ops;
      if (offset < MAX_WHERE) {
	strncpy(query->where_ops[offset].field_name, buf, FIELD_LEN);
	query->where_ops[offset].op = op;
	query->where_ops[offset].value = value;
	query->nwhere_ops++;
      }
      
      /* advance the string pointer */
      while (1) {
	if (token[0] == ')') break;
	if (token[0] == '\0') break;
	if (strncasecmp(token, "ORDER", 5) == 0) break;
	if (strncasecmp(token, "LIMIT", 5) == 0) break;
	if (strncasecmp(token, "AND", 3) == 0) break;
	if (strncasecmp(token, "OR", 2) == 0) break;
	token++;
      }
   
    }

    /* if the item is a left paren */
    else if (token[0] == '(') {
      mystack.push(1);
      token += 1;
      while (token[0] == ' ')
	token++;
    }

    /* if the item is a right paren */
    else if (token[0] == ')') {
      while (mystack.top() != 1) {
	if (mystack.top() == 3) {
	  //printf("AND\n");
	  offset = query->nwhere_ops;
	  if (offset < MAX_WHERE) {
	    query->where_ops[offset].conditional = 1;
	    query->nwhere_ops++;
	  }
	}
	else if (mystack.top() == 4) {
	  //printf("OR\n");
	  offset = query->nwhere_ops;
	  if (offset < MAX_WHERE) {
	    query->where_ops[offset].conditional = 2;
	    query->nwhere_ops++;
	  }
	}
	mystack.pop();
      }
      mystack.pop();
      token += 1;
      while (token[0] == ' ')
	token++;
    }
   
  }

  while (!mystack.empty()) {
    if (mystack.top() == 3) {
      //printf("AND\n");
      offset = query->nwhere_ops;
      if (offset < MAX_WHERE) {
	query->where_ops[offset].conditional = 1;
	query->nwhere_ops++;
      }
    }
    else if (mystack.top() == 4) {
      //printf("OR\n");
      offset = query->nwhere_ops;
      if (offset < MAX_WHERE) {
	query->where_ops[offset].conditional = 2;
	query->nwhere_ops++;
      }
    }
    mystack.pop();
  }

  return 0;
}


int64_t
resolve_predicates (query_plan_t * query)
{
  std::stack<int64_t> mystack;

  for (int64_t j = 0; j < query->nwhere_ops; j++) {
    switch (query->where_ops[j].conditional)
    {
      case 0: /* this is a result value */
	{
	  mystack.push(query->where_ops[j].result);
	  //LOG_D_A ("result: %d", query->where_ops[j].result);
	} break;

      case 1: /* this is an AND operator */
	{
	  int64_t a = mystack.top();
	  mystack.pop();
	  int64_t b = mystack.top();
	  mystack.pop();
	  int64_t res = a && b;
	  mystack.push(res);
	  //LOG_D_A ("AND: %d && %d = %d", a, b, res);
	} break;

      case 2: /* this is an OR operator */
	{
	  int64_t a = mystack.top();
	  mystack.pop();
	  int64_t b = mystack.top();
	  mystack.pop();
	  int64_t res = a || b;
	  mystack.push(res);
	  //LOG_D_A ("OR: %d || %d = %d", a, b, res);
	} break;

      default:
	{
	  /* error state */
	  LOG_E ("Default case");
	  return -1;
	} break;

    }
  }

  int64_t final = mystack.top();
  mystack.pop();
  if (!mystack.empty())
    LOG_W ("The stack was not empty.");

  //LOG_D_A ("returning %d", final);
  return final;
  
}
