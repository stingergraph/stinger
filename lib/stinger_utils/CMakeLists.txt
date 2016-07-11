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

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/stinger_utils")

include_directories("${CMAKE_BINARY_DIR}/include/stinger_utils")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_core")

add_library(stinger_utils SHARED ${sources} ${headers})

target_link_libraries(stinger_utils stinger_core fmemopen int_hm_seq string compat)