set(sources
	src/mongoose.c
)
set(headers


	inc/mongoose.h
)

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/mongoose")

include_directories("${CMAKE_BINARY_DIR}/include/mongoose")

add_library(mongoose SHARED ${sources} ${headers})