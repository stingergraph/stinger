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

typedef struct {
  int conditional;    /* 0: nothing, 1: AND, 2: OR */
  int column;
  operator_t op;
  int value;   /* TODO float, string */
} where_op_t;

typedef struct {
  int columns[30];
  int ncolumns;
  char tablename[100];

  int activate_where;
  where_op_t where_ops[10];

  int activate_orderby;
  int orderby_column;
  int asc;

  int activate_limit;
  int limit;
  int offset;
} query_plan_t;

operator_t
select_operator (char * token);


#endif /* _SQL_PARSER_H_ */
