#ifndef _SQL_PARSER_H_
#define _SQL_PARSER_H_

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

operator_t
select_operator (char * token);


#endif /* _SQL_PARSER_H_ */
