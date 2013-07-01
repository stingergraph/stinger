#if !defined(XMT_LUC_)
#define XMT_LUC_
void xmt_luc_io_init (void);
void xmt_luc_snapin (const char*, void*, size_t);
void xmt_luc_snapout (const char*, void*, size_t);
void xmt_luc_stat (const char*, size_t*);
#endif
