#if !defined(GRAPH_EL_HEADER_)
#define GRAPH_EL_HEADER_

struct el {
  int64_t nv, ne;
  int64_t nv_orig, ne_orig;
  double weight_sum;
  intvtx_t * restrict d;
  intvtx_t * restrict el;
};

struct csr {
  int64_t nv, ne;
  intvtx_t * restrict xoff;
  intvtx_t * colind;
  intvtx_t * weight;

  intvtx_t * mem;
  size_t memsz;
};

#define CCAT(A,B) A##B
#define CONCAT(A,B) CCAT(A,B)

#if defined(__GNUC__)
#define VAR_MAY_BE_UNUSED __attribute__((unused))
#else
#define VAR_MAY_BE_UNUSED
#endif

#define CDECL(G_) const intvtx_t * restrict VAR_MAY_BE_UNUSED CONCAT(edgedata__,G_) = (G_).el; const intvtx_t * restrict VAR_MAY_BE_UNUSED CONCAT(vtxdata__,G_) = (G_).d;
#define DECL(G_) intvtx_t * restrict VAR_MAY_BE_UNUSED CONCAT(edgedata__,G_) = (G_).el; intvtx_t * restrict VAR_MAY_BE_UNUSED CONCAT(vtxdata__,G_) = (G_).d;
#define PDECL(G_) intvtx_t * restrict VAR_MAY_BE_UNUSED CONCAT(edgedata__,G_) = (G_)->el; intvtx_t * restrict VAR_MAY_BE_UNUSED CONCAT(vtxdata__,G_) = (G_)->d;

#define CDECLCSR(G_) const intvtx_t * restrict VAR_MAY_BE_UNUSED CONCAT(xoff__,G_) = (G_).xoff; const intvtx_t * restrict VAR_MAY_BE_UNUSED CONCAT(colind__,G_) = (G_).colind; const intvtx_t * restrict VAR_MAY_BE_UNUSED CONCAT(weight__,G_) = (G_).weight;
#define DECLCSR(G_) intvtx_t * restrict VAR_MAY_BE_UNUSED CONCAT(xoff__,G_) = (G_).xoff; intvtx_t * restrict VAR_MAY_BE_UNUSED CONCAT(colind__,G_) = (G_).colind; intvtx_t * restrict VAR_MAY_BE_UNUSED CONCAT(weight__,G_) = (G_).weight;

#define D(G_,K_) (CONCAT(vtxdata__,G_)[(K_)])
#define I(G_,K_) (CONCAT(edgedata__,G_)[0+3*(K_)])
#define J(G_,K_) (CONCAT(edgedata__,G_)[1+3*(K_)])
#define W(G_,K_) (CONCAT(edgedata__,G_)[2+3*(K_)])

#define XOFF(G_,I_) (CONCAT(xoff__,G_)[(I_)])
#define COLJ(G_,K_) (CONCAT(colind__,G_)[(K_)])
#define COLW(G_,K_) (CONCAT(weight__,G_)[(K_)])

#define Dold(G_,K_) ((G_).d[(K_)])
#define Iold(G_,K_) ((G_).el[0+3*(K_)])
#define Jold(G_,K_) ((G_).el[1+3*(K_)])
#define Wold(G_,K_) ((G_).el[2+3*(K_)])

#define Iel(EL_,K_) ((EL_)[0+3*(K_)])
#define Jel(EL_,K_) ((EL_)[1+3*(K_)])
#define Wel(EL_,K_) ((EL_)[2+3*(K_)])

struct el alloc_graph (int64_t, int64_t);
int realloc_graph (struct el *, int64_t, int64_t);
int shrinkwrap_graph (struct el *);
struct el copy_graph (struct el);
struct el free_graph (struct el);
int el_snarf_graph (const char *, struct el *);
void dump_el (const char *, const struct el, const size_t, int64_t * restrict);

void el_eval_vtxvol_byrow (const struct el g, const int64_t *, const int64_t *, int64_t *);
void el_eval_vtxvol (const struct el g, int64_t *);

struct csr el_to_csr (struct el);
struct csr alloc_csr (intvtx_t, intvtx_t);
struct csr free_csr (struct csr);


inline static void
canonical_order_edge (intvtx_t * restrict ip, intvtx_t * restrict jp)
{
  intvtx_t i = *ip, j = *jp;
#if !defined(ORDERED_TRIL)
  if ((i ^ j) & 0x01) {
    if (i < j) {
      *ip = i;
      *jp = j;
    } else {
      *ip = j;
      *jp = i;
    }
  } else {
    if (i > j) {
      *ip = i;
      *jp = j;
    } else {
      *ip = j;
      *jp = i;
    }
  }
#else
  if (i < j) {
    *ip = j;
    *jp = i;
  }
#endif
}

#endif /* GRAPH_EL_HEADER_ */
