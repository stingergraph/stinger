set(_names_test_sources
  names_test/stinger_names_test.c
  names_test/stinger_names_test.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/names_test)
add_executable(stinger_names_test ${_names_test_sources})
target_link_libraries(stinger_names_test stinger_utils)

#================================

set(_adamic_adar_test_sources
  adamic_adar_test/adamic_adar_test.c
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/adamic_adar_test)
add_executable(stinger_adamic_adar_test ${_adamic_adar_test_sources})
target_link_libraries(stinger_adamic_adar_test stinger_utils stinger_alg stinger_core)

#================================

set(_pagerank_test_sources
  pagerank_test/pagerank_test.c
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/pagerank_test)
add_executable(stinger_pagerank_test ${_pagerank_test_sources})
target_link_libraries(stinger_pagerank_test stinger_utils stinger_alg stinger_core)
