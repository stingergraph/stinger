#if !defined(BFS_H_)
#define BFS_H_

void bfs_stinger (const struct stinger *G, const int64_t nv,
		  const int64_t ne, const int64_t startV, int64_t * marks,
		  const int64_t numSteps, int64_t * Q, int64_t * QHead,
		  int64_t * neighbors);

int64_t st_conn_stinger (const struct stinger *G, const int64_t nv,
			 const int64_t ne, const int64_t * sources,
			 const int64_t num, const int64_t numSteps);

int64_t st_conn_stinger_source (const struct stinger *G, const int64_t nv,
				const int64_t ne, const int64_t from,
				const int64_t * sources, const int64_t num,
				const int64_t numSteps);

void bfs_csr (const int64_t nv, const int64_t ne, const int64_t * off,
	      const int64_t * ind, const int64_t startV, int64_t * marks,
	      const int64_t numSteps, int64_t * Q, int64_t * QHead);

int64_t st_conn_csr (const int64_t nv, const int64_t ne,
		     const int64_t * off, const int64_t * ind,
		     const int64_t * sources, const int64_t num,
		     const int64_t numSteps);

#endif /* BFS_H_ */
