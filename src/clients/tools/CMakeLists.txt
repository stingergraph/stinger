set(_alg_to_mongo_sources
  alg_to_mongo/inc/alg_to_mongo.h
  alg_to_mongo/src/alg_handling.cpp
  alg_to_mongo/src/main.cpp
  alg_to_mongo/src/time_functions.cpp
)


add_executable(stinger_alg_to_mongo ${_alg_to_mongo_sources})
target_link_libraries(stinger_alg_to_mongo stinger_net mongo_c_driver)
target_include_directories(stinger_alg_to_mongo PUBLIC ${CMAKE_BINARY_DIR})
target_include_directories(stinger_alg_to_mongo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/alg_to_mongo/inc)
target_include_directories(stinger_alg_to_mongo PUBLIC ${CMAKE_BINARY_DIR}/include/stinger_net)
target_include_directories(stinger_alg_to_mongo PUBLIC ${CMAKE_BINARY_DIR}/stinger_net/proto)

##############################################################################

set(_dump_graph_to_disk_sources
  dump_graph_to_disk/src/main.cpp
)

add_executable(stinger_dump_graph_to_disk ${_dump_graph_to_disk_sources})
target_link_libraries(stinger_dump_graph_to_disk stinger_net)
target_include_directories(stinger_dump_graph_to_disk PUBLIC ${CMAKE_BINARY_DIR})

##############################################################################

add_subdirectory(json_rpc_server)

##############################################################################

set(_scrape_alg_data_sources
  scrape_alg_data/src/scrape_mmap.c
)

add_executable(stinger_scape_alg_data ${_scrape_alg_data_sources})

##############################################################################

set(_scrape_vertex_data_sources
  scrape_vertex_data/src/scrape_vertex_data.c
)

add_executable(stinger_scrape_vertex_data ${_scrape_vertex_data_sources})
target_link_libraries(stinger_scrape_vertex_data stinger_core)

##############################################################################

set(_sql_client_sources
  sql_client/inc/sql_parser.h
  sql_client/src/execution.cpp
  sql_client/src/parser.cpp
  sql_client/src/sql_parser.cpp
  sql_client/src/utils.cpp
)


add_executable(stinger_sql_client ${_sql_client_sources})
target_link_libraries(stinger_sql_client stinger_core stinger_net)
target_include_directories(stinger_sql_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/sql_client/inc)
target_include_directories(stinger_sql_client PUBLIC ${CMAKE_BINARY_DIR})
