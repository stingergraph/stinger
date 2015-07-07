#ifndef _METISISH_SUPPORT_H
#define _METISISH_SUPPORT_H

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

int
load_metisish_graph (struct stinger * S, char * filename);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* _METISISH_SUPPORT_H */
