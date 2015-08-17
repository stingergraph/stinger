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

##############################################################################

set(_test_client_sources
  test_client/src/main.c
)

add_executable(stinger_test_client ${_test_client_sources})
target_link_libraries(stinger_test_client stinger_core stinger_utils)

##############################################################################

set(_udp_query_sources
  udp_query/src/main.c
)

add_executable(stinger_udp_query ${_udp_query_sources})

##############################################################################

set(_web_server_sources
  web_server/inc/web.h
  web_server/src/web.c
)


add_executable(stinger_web_server ${_web_server_sources})
target_link_libraries(stinger_web_server stinger_utils kv_store mongoose)
target_include_directories(stinger_web_server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/web_server/inc)

##############################################################################

set(_pystinger_sources
  vquery/_stinger.c
  vquery/pystinger.c
  vquery/pystinger.h
)

find_package(PythonInterp)
find_package(PythonLibs)

if(PYTHONINTERP_FOUND AND PYTHONLIBS_FOUND AND NOT CYGWIN)
  include_directories(${PYTHON_INCLUDE_PATH})
  add_library(pystinger ${_pystinger_sources})
  target_link_libraries(pystinger stinger_utils)
  if(APPLE)
    target_link_libraries(pystinger python)
  endif()
else()
  message("Could not find Python.  Skipping vquery extension.")
endif()

if(CYGWIN)
  message("Unglyness, but the vquery python stinger stuff is not supported on Cygwin for now.")
endif()
