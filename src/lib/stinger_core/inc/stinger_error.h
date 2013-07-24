#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/**
* @file stinger_error.h
* @brief Yet another logging system :-)
*
* Levels: Fatal, Error, Warning, Info, Verbose, and Debug
* You may filter to a specific level by defining LOG_AT_X where X is F,E,W,I,V,D
* Filters are inclusive of all lower levels (e.g. W includes F and E).
*
* Unless otherwise specified, output is routed to stderr.  To override, define 
* LOG_OUTPUT to a file handle before including this file.
*
* @author Rob McColl
* @date 2013-07-19
*/

#ifndef LOG_OUTPUT
  #define LOG_OUTPUT stderr
#endif

#ifdef LOG_F
#undef LOG_F
#endif
#ifdef LOG_F_A
#undef LOG_F_A
#endif

#ifdef LOG_E
#undef LOG_E
#endif
#ifdef LOG_E_A
#undef LOG_E_A
#endif

#ifdef LOG_W
#undef LOG_W
#endif
#ifdef LOG_W_A
#undef LOG_W_A
#endif

#ifdef LOG_I
#undef LOG_I
#endif
#ifdef LOG_I_A
#undef LOG_I_A
#endif

#ifdef LOG_V
#undef LOG_V
#endif
#ifdef LOG_V_A
#undef LOG_V_A
#endif

#ifdef LOG_D
#undef LOG_D
#endif
#ifdef LOG_D_A
#undef LOG_D_A
#endif

#define LOG_D_A(X,...)
#define LOG_D(X)

#define LOG_V_A(X,...)
#define LOG_V(X)

#define LOG_I_A(X,...)
#define LOG_I(X)

#define LOG_W_A(X,...)
#define LOG_W(X)

#define LOG_E_A(X,...)
#define LOG_E(X)


#define LOG_F_A(X, ...) do { fprintf(LOG_OUTPUT, "FATAL: %s %d: " X "\n", __func__, __LINE__, __VA_ARGS__); fflush(LOG_OUTPUT); } while(0);
#define LOG_F(X) LOG_F_A(X, NULL)
#ifndef LOG_AT_F

#undef LOG_E_A
#undef LOG_E
#define LOG_E_A(X, ...) do { fprintf(LOG_OUTPUT, "ERROR: %s %d: " X "\n", __func__, __LINE__, __VA_ARGS__); fflush(LOG_OUTPUT); } while(0);
#define LOG_E(X) LOG_E_A(X, NULL)
#ifndef LOG_AT_E

#undef LOG_W_A
#undef LOG_W
#define LOG_W_A(X, ...) do { fprintf(LOG_OUTPUT, "WARNING: %s %d: " X "\n", __func__, __LINE__, __VA_ARGS__); fflush(LOG_OUTPUT); } while(0);
#define LOG_W(X) LOG_W_A(X, NULL)
#ifndef LOG_AT_W

#undef LOG_I_A
#undef LOG_I
#define LOG_I_A(X, ...) do { fprintf(LOG_OUTPUT, "INFO: %s %d: " X "\n", __func__, __LINE__, __VA_ARGS__); fflush(LOG_OUTPUT); } while(0);
#define LOG_I(X) LOG_I_A(X, NULL)
#ifndef LOG_AT_A

#undef LOG_V_A
#undef LOG_V
#define LOG_V_A(X, ...) do { fprintf(LOG_OUTPUT, "VERBOSE: %s %d: " X "\n", __func__, __LINE__, __VA_ARGS__); fflush(LOG_OUTPUT); } while(0);
#define LOG_V(X) LOG_V_A(X, NULL)
#ifndef LOG_AT_V

#undef LOG_D_A
#undef LOG_D
#define LOG_D_A(X, ...) do { fprintf(LOG_OUTPUT, "DEBUG: %s %d: " X "\n", __func__, __LINE__, __VA_ARGS__); fflush(LOG_OUTPUT); } while(0);
#define LOG_D(X) LOG_D_A(X, NULL)

#endif
#endif
#endif
#endif
#endif

#ifdef __cplusplus
}
#endif
