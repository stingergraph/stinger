#ifndef _SQL_PARSER_H_
#define _SQL_PARSER_H_

#define ID       0
#define NAME     1
#define TYPE     2
#define WEIGHT   3
#define SRC_VTX  4
#define TGT_VTX  5
#define FIRST    6
#define LAST     7

#define FIELD_LEN 128
#define MAX_COLUMNS 30
#define MAX_WHERE 10

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

typedef enum {
  AND,		      /* AND, && */
  BETWEEN,	      /* BETWEEN ... AND ... */
  MOD,		      /* MOD, % */
  DIV_INT,	      /* DIV (integer division) */
  NOT_BETWEEN,	      /* NOT BETWEEN .. AND .. */
  OR,		      /* OR, || */
  XOR,		      /* XOR */
  BIT_AND,	      /* & */
  BIT_INVERT,	      /* ~ */
  BIT_OR,	      /* | */
  BIT_XOR,	      /* ^ */
  DIVIDE,	      /* / */
  EQUAL,	      /* = */
  GTE,		      /* >= */
  GREATER,	      /* > */
  LEFT_SHIFT,	      /* << */
  LTE,		      /* <= */
  LESS,		      /* < */
  SUBTRACT,	      /* - */
  NOT_EQUAL,	      /* !=, <> */
  ADD,		      /* + */
  RIGHT_SHIFT,	      /* >> */
  MULTIPLY,	      /* * */
  OP_ERROR
} operator_t;

typedef enum {
  VERTEX_TBL,
  EDGE_TBL
} table_t;

typedef struct {
  int64_t conditional;    /* 0: nothing, 1: AND, 2: OR */
  char field_name[FIELD_LEN];
  operator_t op;
  int64_t value;   /* TODO float, string */
} where_op_t;

typedef struct {
  char field_name[FIELD_LEN];
  char type;
  void * data;
  int64_t len;
  int64_t offset;
} column_t;

typedef struct {
  column_t columns[MAX_COLUMNS];
  int64_t ncolumns;
  table_t table;

  int64_t activate_where;
  where_op_t where_ops[MAX_WHERE];
  int64_t nwhere_ops;

  int64_t activate_orderby;
  char orderby_column[FIELD_LEN];
  int64_t asc;   /* 0 for ASC, 1 for DESC */

  int64_t activate_limit;
  int64_t limit;
  int64_t offset;
} query_plan_t;


int64_t
parse_query (char * input, query_plan_t * query);

operator_t
select_operator (char * token);

int64_t
add_column_to_query (query_plan_t * query, char * column);

void
print_query_plan (query_plan_t * query);

#endif /* _SQL_PARSER_H_ */
