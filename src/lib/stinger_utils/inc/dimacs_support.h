#ifndef _DIMACS_SUPPORT_H
#define _DIMACS_SUPPORT_H

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

int
load_dimacs_graph (struct stinger * S, const char * filename);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* _DIMACS_SUPPORT_H */
