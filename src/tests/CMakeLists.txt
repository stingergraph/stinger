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
