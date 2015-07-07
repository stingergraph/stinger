set(sources
	src/alg_handling.cpp
	src/batch.cpp
	src/batch_server.cpp
	src/server.cpp
	src/udp_name_server.cpp
)

set(headers
	inc/server.h
)

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/stinger_net)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
add_executable(stinger_server ${sources} ${headers})
target_link_libraries(stinger_server stinger_utils stinger_net)