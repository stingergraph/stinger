set(_breadth_first_search_sources
  breadth_first_search/bfs.c
  breadth_first_search/bfs.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/breadth_first_search)
add_executable(stinger_breadth_first_search ${_breadth_first_search_sources})
target_link_libraries(stinger_breadth_first_search stinger_utils)

##############################################################################

set(_community_reagglomeration_sources
  community_reagglomeration/bsutil.h
  community_reagglomeration/community-update.c
  community_reagglomeration/community-update.h
  community_reagglomeration/community.c
  community_reagglomeration/community.h
  community_reagglomeration/compat.h
  community_reagglomeration/graph-el.c
  community_reagglomeration/graph-el.h
  community_reagglomeration/main.c
  community_reagglomeration/sorts.c
  community_reagglomeration/sorts.h
  community_reagglomeration/xmt-luc.cc
  community_reagglomeration/xmt-luc.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/community_reagglomeration)
add_executable(stinger_community_reagglomeration ${_community_reagglomeration_sources})
target_link_libraries(stinger_community_reagglomeration stinger_utils)

##############################################################################

set(_connected_components_sources
  connected_components/connected-comp.c
  connected_components/connected-comp.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/connected_components)
add_executable(stinger_connected_components ${_connected_components_sources})
target_link_libraries(stinger_connected_components stinger_utils)

##############################################################################

set(_depold_community_sources
  depold_community/src/main.cpp
)

add_executable(stinger_depold_community ${_depold_community_sources})
target_link_libraries(stinger_depold_community stinger_utils int_ht_seq)

##############################################################################

set(_insert_remove_benchmark_sources
  insert_remove_benchmark/main.c
)

add_executable(stinger_insert_remove_benchmark ${_insert_remove_benchmark_sources})
target_link_libraries(stinger_insert_remove_benchmark stinger_utils)

##############################################################################

set(_matvev_sources
  matvec/main.c
)

add_executable(stinger_matvec ${_matvev_sources})
target_link_libraries(stinger_matvec stinger_utils)

##############################################################################

set(_protobuf_test_sources
  protobuf_test/main.c
)

add_executable(stinger_protobuf_test ${_protobuf_test_sources})

##############################################################################

set(_rmat_graph_generator_sources
  rmat_graph_generator/rmatter.c
)

add_executable(stinger_rmat_graph_generator ${_rmat_graph_generator_sources})
target_link_libraries(stinger_rmat_graph_generator stinger_utils stinger_alg)
target_include_directories(stinger_rmat_graph_generator PUBLIC ${CMAKE_BINARY_DIR}/include/stinger_alg)

##############################################################################

set(_streaming_clustering_coefficients_sources
  streaming_clustering_coefficients/main.c
)

add_executable(stinger_streaming_clustering_coefficients ${_streaming_clustering_coefficients_sources})
target_link_libraries(stinger_streaming_clustering_coefficients stinger_utils)

##############################################################################

set(_streaming_connected_components_sources
  streaming_connected_components/connected-comp.c
  streaming_connected_components/connected-comp.h
  streaming_connected_components/main.c
)

add_executable(stinger_streaming_connected_components ${_streaming_connected_components_sources})
target_link_libraries(stinger_streaming_connected_components stinger_utils)

##############################################################################

set(_streaming_connected_components_hipc_sources
  streaming_connected_components_hipc/main.c
)

add_executable(stinger_streaming_connected_components_hipc_sources ${_streaming_connected_components_hipc_sources})
target_link_libraries(stinger_streaming_connected_components_hipc_sources stinger_utils)


