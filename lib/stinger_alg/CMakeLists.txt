set(sources
	src/adamic_adar.c
	src/pagerank.c
	src/rmat.c
	src/random.c
)
set(headers
	inc/adamic_adar.h
	inc/pagerank.h
	inc/rmat.h
	inc/random.h
)

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/stinger_alg")

include_directories("${CMAKE_BINARY_DIR}/include/stinger_alg")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_core")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_net")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_utils")

set_source_files_properties(${config} PROPERTIES GENERATED TRUE)
add_library(stinger_alg SHARED ${sources} ${headers} ${config})
target_link_libraries(stinger_alg stinger_core stinger_net stinger_utils compat)
