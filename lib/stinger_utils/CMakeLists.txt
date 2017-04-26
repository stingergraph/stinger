set(sources
	src/csv.c
	src/dimacs_support.c
	src/graph_construct.c
	src/histogram.c
	src/json_graph.cpp
	src/json_support.c
	src/metisish_support.c
	src/stinger_extract.c
	src/stinger_la.c
	src/stinger_test.c
	src/stinger_utils.c
	src/timer.c
	src/xml_support.c
)
set(headers
	inc/csv.h
	inc/dimacs_support.h
	inc/graph_construct.h
	inc/histogram.h
	inc/json_support.h
	inc/metisish_support.h
	inc/stinger_la.h
	inc/stinger_test.h
	inc/stinger_utils.h
	inc/timer.h
	inc/xml_support.h
)

publish_headers(headers "${CMAKE_BINARY_DIR}/include/stinger_utils")

include_directories("${CMAKE_BINARY_DIR}/include/stinger_utils")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_core")

# Need to declare an explicit dependency on rapidjson headers, since there's no associated target
set_source_files_properties("${CMAKE_BINARY_DIR}/include/rapidjson/document.h" GENERATED)
add_library(stinger_utils ${sources} ${headers} "${CMAKE_BINARY_DIR}/include/rapidjson/document.h")
add_dependencies(stinger_utils rapidjson)
target_link_libraries(stinger_utils stinger_core fmemopen int_hm_seq string compat)