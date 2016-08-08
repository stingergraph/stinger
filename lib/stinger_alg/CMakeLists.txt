set(sources
  src/adamic_adar.c
  src/betweenness.c
  src/clustering.c
  src/kcore.c
  src/pagerank.c
  src/random.c
  src/rmat.c
  src/static_components.c
  src/weakly_connected_components.c
  src/streaming_connected_components.c
)
set(headers
  inc/adamic_adar.h
  inc/betweenness.h
  inc/clustering.h
  inc/kcore.h
  inc/pagerank.h
  inc/random.h
  inc/rmat.h
  inc/static_components.h
  inc/weakly_connected_components.h
  inc/streaming_connected_components.h
)

publish_headers(headers "${CMAKE_BINARY_DIR}/include/stinger_alg")

include_directories("${CMAKE_BINARY_DIR}/include/stinger_alg")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_core")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_net")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_utils")

set_source_files_properties(${config} PROPERTIES GENERATED TRUE)
add_library(stinger_alg SHARED ${sources} ${headers} ${config})
target_link_libraries(stinger_alg stinger_core stinger_net stinger_utils compat)