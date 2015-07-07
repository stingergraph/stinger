set(sources
	src/compat.h
	src/pagerank.c
	src/pagerank.h
	src/pagerank_updating.c
	src/spmspv.c
	src/spmspv.h
	src/spmspv_ompcas.c
	src/spmspv_ompcas.h
	src/spmspv_ompcas_batch.c
	src/spmspv_ompcas_batch.h
	src/spmspv_ompsimple.c
	src/spmspv_ompsimple.h
)

include_directories(${CMAKE_CURRENT_SRC_DIR})

add_executable(stinger_pagerank_updating ${sources})
target_link_libraries(stinger_pagerank_updating stinger_net)