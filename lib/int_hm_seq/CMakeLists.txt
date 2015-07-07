set(sources
	src/int-hm-seq.c
)
set(headers
	inc/int_hm_seq.h
)

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/int_hm_seq")

include_directories("${CMAKE_BINARY_DIR}/include/int_hm_seq")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_utils")

add_library(int_hm_seq SHARED ${sources} ${headers})