set(sources
	src/compat.h
	src/spmspv.c
	src/spmspv.h
	src/spmspv_ompcas.c
	src/spmspv_ompcas.h
	src/spmspv_ompcas_batch.c
	src/spmspv_ompcas_batch.h
	src/spmspv_ompsimple.c
	src/spmspv_ompsimple.h
	src/spmspv_test.c
)

include_directories(${CMAKE_CURRENT_SRC_DIR})

add_executable(stinger_spmspv_test ${sources})
target_link_libraries(stinger_spmspv_test stinger_net)