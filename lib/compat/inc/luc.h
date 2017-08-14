#ifndef _LUC_H
#define _LUC_H

void luc_io_init (void);
void luc_snapin (const char*, void*, off_t);
void luc_snapout (const char*, void*, off_t);
void luc_stat (const char*, off_t*);

#endif /* _LUC_H */
