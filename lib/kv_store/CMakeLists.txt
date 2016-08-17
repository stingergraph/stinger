set(sources
	src/kv_store.c
)
set(headers
	inc/kv_store.h
)

publish_headers(headers "${CMAKE_BINARY_DIR}/include/kv_store")

include_directories("${CMAKE_BINARY_DIR}/include/kv_store")

add_library(kv_store SHARED ${sources} ${headers})

target_link_libraries(kv_store string)