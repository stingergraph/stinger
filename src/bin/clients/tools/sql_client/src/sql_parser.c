#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "sql_parser.h"


typedef enum { ID, NAME, TYPE, WEIGHT } field_t;

typedef enum { 
  SELECT,
  FIELDS,
  FROM,
  TABLES,
  WHERE,
  WHERE_COL,
  WHERE_OPER,
  WHERE_VAL,
  ORDERBY,
  ORDERBY_DIR,
  LIMIT,
  OFFSET,
  DONE,
  ERROR
} state_t;

int main (int argc, char *argv[])
{

  state_t cur_state = SELECT;
  state_t next_state;
  int where_and = 0;
  int where_or = 0;

  char input[100] = "SELECT name,type,weight FROM vertices WHERE weight > 2 ORDER BY name LIMIT 10";
  char buf[30];
  printf("input string is:[%s]\n", input);

  char * token;

  token = strtok (input, " ,");

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
			next_state = FIELDS;
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
			printf("%s\n", token);
			token = strtok (NULL, " ,");
		      }
		      else if (token && strncmp(token, "edges", 5) == 0) {
			valid = 1;
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
		      next_state = WHERE_COL;
		    } break;

      case WHERE_COL:
		    {
		      printf(":: WHERE_COL\n");
		      /* Process here */
		      printf("%s\n", token);
		      token = strtok (NULL, " ,");
		      next_state = WHERE_OPER;
		    } break;

      case WHERE_OPER:
		    {
		      printf(":: WHERE_OPER\n");
		      printf("%s\n", token);
		      operator_t op = select_operator (token);
		      printf("op %d\n", op);
		      token = strtok (NULL, " ,");
		      next_state = WHERE_VAL;
		    } break;

      case WHERE_VAL:
		    {
		      printf(":: WHERE_VAL\n");
		      int val;
		      sscanf(token, "%d", &val);
		      printf("%d\n", val);

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
			next_state = WHERE_COL;
		      }
		      else if (token && strncasecmp(token, "OR", 2) == 0) {
			token = strtok (NULL, " ,");
			next_state = WHERE_COL;
		      }
		    } break;

      case ORDERBY:
		    {
		      int valid = 0;
		      printf(":: ORDERBY\n");
		      if (token && strncasecmp(token, "BY", 2) == 0) {
			valid = 1;
			token = strtok (NULL, " ,");
		      }
		      else {
			next_state = ERROR;
		      }

		      if (valid) {
			sscanf(token, "%s", buf);
			printf("%s\n", buf);
			token = strtok (NULL, " ,");
			next_state = ORDERBY_DIR;
		      }
		    } break;

      case ORDERBY_DIR:
		    {
		      printf(":: ORDERBY_DIR\n");
		      if (token && strncasecmp(token, "ASC", 3) == 0) {
			printf("%s\n", token);
			token = strtok (NULL, " ,");
		      }
		      else if (token && strncasecmp(token, "DESC", 4) == 0) {
			printf("%s\n", token);
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
		      int limit_number;
		      printf(":: LIMIT\n");
		      sscanf(token, "%d", &limit_number);
		      printf("%d\n", limit_number);

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
		      int offset_number;
		      printf(":: OFFSET\n");
		      if (token) {
			sscanf(token, "%d", &offset_number);
			printf("%d\n", offset_number);
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
