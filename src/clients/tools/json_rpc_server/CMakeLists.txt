set(headers
	inc/alg_data_array.h
	inc/json_rpc.h
	inc/json_rpc_server.h
	inc/mon_handling.h
	inc/rpc_state.h
	inc/session_handling.h
)

set(sources
	src/adamic_adar.cpp
	src/alg_data_array.cpp
	src/array_to_json_monolithic.cpp
	src/array_to_json_reduction.cpp
	src/bfs_edges.cpp
	src/breadth_first_search.cpp
        src/community_on_demand.cpp
	src/egonet.cpp
	src/get_algorithms.cpp
	src/get_connected_component.cpp
	src/get_data_array.cpp
	src/get_data_array_range.cpp
	src/get_data_array_reduction.cpp
	src/get_data_array_set.cpp
	src/get_data_array_sorted_range.cpp
	src/get_data_description.cpp
	src/get_graph_stats.cpp
	src/get_server_health.cpp
	src/get_server_info.cpp
	src/json_rpc.cpp
	src/json_rpc.cpp
	src/json_rpc_server.cpp
	src/mon_handling.cpp
	src/pagerank_subgraph.cpp
	src/register_request.cpp
	src/rpc_state.cpp
	src/session_handling.cpp
	src/stinger_rpc_functions.cpp
		src/exact_diameter.cpp src/single_source_shortest_path.cpp)

add_executable(stinger_json_rpc_server ${headers} ${sources})
target_link_libraries(stinger_json_rpc_server stinger_net stinger_alg mongoose)
target_include_directories(stinger_json_rpc_server PUBLIC ${CMAKE_BINARY_DIR})
target_include_directories(stinger_json_rpc_server PUBLIC ${CMAKE_BINARY_DIR}/stinger_net)
target_include_directories(stinger_json_rpc_server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
