include_directories(${CMAKE_BINARY_DIR})

set(_csv_stream_sources
	csv_stream/src/main.cpp
	csv_stream/inc/explore_csv.h
)

add_executable(stinger_csv_stream ${_csv_stream_sources})
target_link_libraries(stinger_csv_stream stinger_net)
target_include_directories(stinger_csv_stream PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/csv_stream/inc)

##############################################################################

set(_human_edge_generator_sources
  human_edge_generator/inc/human_edge_generator.h
  human_edge_generator/src/time_functions.cpp
  human_edge_generator/src/main.cpp
)

add_executable(stinger_human_edge_generator ${_human_edge_generator_sources})
target_include_directories(stinger_human_edge_generator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/csv_stream/inc)
target_include_directories(stinger_human_edge_generator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/human_edge_generator/inc)
target_link_libraries(stinger_human_edge_generator stinger_net)

##############################################################################

set(_json_stream_sources
  json_stream/inc/explore_json.h
  json_stream/src/main.cpp
)

add_executable(stinger_json_stream ${_json_stream_sources})
target_link_libraries(stinger_json_stream stinger_net)
target_include_directories(stinger_json_stream PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/json_stream/inc)

##############################################################################

set(_mongodb_stream_sources
	mongodb_stream/src/main.cpp
	mongodb_stream/inc/mongodb_stream.h
)

add_executable(stinger_mongodb_stream ${_mongodb_stream_sources})
target_link_libraries(stinger_mongodb_stream stinger_net mongo_c_driver)
target_include_directories(stinger_mongodb_stream PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/mongodb_stream/inc)

##############################################################################

set(_netflow_stream_sources
	netflow_stream/inc/netflow_stream.h
	netflow_stream/src/main.cpp
)

add_executable(stinger_netflow_stream ${_netflow_stream_sources})
target_include_directories(stinger_netflow_stream PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/netflow_stream/inc)
target_link_libraries(stinger_netflow_stream stinger_net) 

##############################################################################

set(_random_edge_generator_sources
	random_edge_generator/inc/build_name.h
	random_edge_generator/inc/random_edge_generator.h
	random_edge_generator/src/build_name.cpp
	random_edge_generator/src/main.cpp
)

add_executable(stinger_random_edge_generator ${_random_edge_generator_sources})
target_include_directories(stinger_random_edge_generator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/random_edge_generator/inc)
target_link_libraries(stinger_random_edge_generator stinger_net)

##############################################################################

set(_rmat_edge_generator_sources
	rmat_edge_generator/inc/build_name.h
	rmat_edge_generator/inc/rmat_edge_generator.h
	rmat_edge_generator/src/build_name.cpp
	rmat_edge_generator/src/main.cpp
)

add_executable(stinger_rmat_edge_generator ${_rmat_edge_generator_sources})
target_include_directories(stinger_rmat_edge_generator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/rmat_edge_generator/inc)
target_link_libraries(stinger_rmat_edge_generator stinger_net stinger_alg)

##############################################################################

