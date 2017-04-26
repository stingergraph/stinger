set(sources
	src/int-ht-seq.c
)
set(headers
	inc/int_ht_seq.h
)

publish_headers(headers "${CMAKE_BINARY_DIR}/include/int_ht_seq")

include_directories("${CMAKE_BINARY_DIR}/include/int_ht_seq")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_utils")

add_library(int_ht_seq ${sources} ${headers})