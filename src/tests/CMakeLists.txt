set(_names_test_sources
  names_test/stinger_names_test.cpp
  names_test/stinger_names_test.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/names_test)
add_executable(stinger_names_test ${_names_test_sources})
target_link_libraries(stinger_names_test stinger_core gtest)

#================================

set(_stinger_core_test_sources
        stinger_core_test/stinger_core_test.cpp
        stinger_core_test/stinger_core_test.h
        )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/stinger_core_test)
add_executable(stinger_core_test ${_stinger_core_test_sources})
target_link_libraries(stinger_core_test stinger_core gtest)

#================================

set(_stinger_batch_test_sources
        stinger_batch_test/stinger_batch_test.cpp
        stinger_batch_test/stinger_batch_test.h
        )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/stinger_batch_test)
#HACK
include_directories(${CMAKE_SOURCE_DIR}/lib/stinger_net/inc)
add_executable(stinger_batch_test ${_stinger_batch_test_sources})
target_link_libraries(stinger_batch_test stinger_core stinger_net gtest)

#================================

set(_stinger_physmap_test_sources
  stinger_physmap_test/stinger_physmap_test.cpp
  stinger_physmap_test/stinger_physmap_test.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/stinger_physmap_test)
add_executable(stinger_physmap_test ${_stinger_physmap_test_sources})
target_link_libraries(stinger_physmap_test stinger_core gtest)

#================================

set(_stinger_traversal_test_sources
  stinger_traversal_test/stinger_traversal_test.cpp
  stinger_traversal_test/non_read_only_traversal.cpp
  stinger_traversal_test/read_only_traversal.cpp
  stinger_traversal_test/stinger_traversal_test.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/stinger_traversal_test)
add_executable(stinger_traversal_test ${_stinger_traversal_test_sources})
target_link_libraries(stinger_traversal_test stinger_core gtest)

#================================

set(_adamic_adar_test_sources
  adamic_adar_test/adamic_adar_test.cpp
  adamic_adar_test/adamic_adar_test.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/adamic_adar_test)
add_executable(stinger_adamic_adar_test ${_adamic_adar_test_sources})
target_link_libraries(stinger_adamic_adar_test stinger_utils stinger_alg stinger_core gtest)

#================================

set(_pagerank_test_sources
  pagerank_test/pagerank_test.cpp
  pagerank_test/pagerank_test.h
  pagerank_test/directed_princeton.cpp
  pagerank_test/undirected_princeton.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/pagerank_test)
add_executable(stinger_pagerank_test ${_pagerank_test_sources})
target_link_libraries(stinger_pagerank_test stinger_utils stinger_alg stinger_core gtest)

#================================

set(_betweenness_test_sources
  betweenness_test/betweenness_test.cpp
  betweenness_test/betweenness_test.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/betweenness_test)
add_executable(stinger_betweenness_test ${_betweenness_test_sources})
target_link_libraries(stinger_betweenness_test stinger_utils stinger_alg stinger_core gtest)

#================================

set(_streaming_connected_components_test_sources
  streaming_connected_components_test/scc_test.cpp
  streaming_connected_components_test/scc_test.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/streaming_connected_components_test)
add_executable(stinger_streaming_connected_components_test ${_streaming_connected_components_test_sources})
target_link_libraries(stinger_streaming_connected_components_test stinger_utils stinger_alg stinger_core gtest)

#================================

set(_shortest_paths_sources
        shortest_paths/shortest_paths_test.cpp
        shortest_paths/shortest_paths_test.h
        )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/shortest_paths)
add_executable(stinger_shortest_paths ${_shortest_paths_sources})
target_link_libraries(stinger_shortest_paths stinger_utils stinger_alg stinger_core gtest)

#================================

set(_diameter_test_sources
        diameter_test/diameter_test.cpp
        diameter_test/diameter_test.h
        )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/diameter_test)
add_executable(stinger_diameter_test ${_diameter_test_sources})
target_link_libraries(stinger_diameter_test stinger_utils stinger_alg stinger_core gtest)

#================================

set(_independent_sets_test_sources
        independent_sets_test/independent_sets_test.cpp
        independent_sets_test/independent_sets_test.h
        )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/independent_sets_test)
add_executable(stinger_independent_sets_test ${_independent_sets_test_sources})
target_link_libraries(stinger_independent_sets_test stinger_utils stinger_alg stinger_core gtest)

#================================

set(_hits_test_sources
       hits_test/hits_test.cpp
        hits_test/hits_test.h
        )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/hits_test)
add_executable(stinger_hits_test ${_hits_test_sources})
target_link_libraries(stinger_hits_test stinger_utils stinger_alg stinger_core gtest)
