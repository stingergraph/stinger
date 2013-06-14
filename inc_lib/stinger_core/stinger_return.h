#ifndef  STINGER_RETURN_H
#define  STINGER_RETURN_H

#define STINGER_RETURN_TYPES \
  TYPE(STINGER_SUCCESS), \
  TYPE(STINGER_REMOVE), \
  TYPE(STINGER_ALLOC_FAILED), \
  TYPE(STINGER_GENERIC_ERR), \
  TYPE(STINGER_NOT_FOUND), \
  TYPE(MAX_STINGER_RETURN)

#define TYPE(X) X
typedef enum {
  STINGER_RETURN_TYPES
} stinger_return_t;
#undef TYPE

const char * 
stinger_return_to_string(stinger_return_t ret);

stinger_return_t
stinger_return_from_string(char * str);

#endif  /*STINGER_RETURN_H*/
