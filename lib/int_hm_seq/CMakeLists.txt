set(sources
	src/int-hm-seq.c
)
set(headers
	inc/int_hm_seq.h
)

publish_headers(headers "${CMAKE_BINARY_DIR}/include/int_hm_seq")

include_directories("${CMAKE_BINARY_DIR}/include/int_hm_seq")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_utils")

add_library(int_hm_seq ${sources} ${headers})