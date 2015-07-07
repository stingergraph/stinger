set(sources
	src/bcon.c
	src/bson.c
	src/encoding.c
	src/env.c
	src/gridfs.c
	src/md5.c
	src/mongo.c
	src/numbers.c
)
set(headers
	inc/bcon.h
	inc/bson.h
	inc/encoding.h
	inc/env.h
	inc/gridfs.h
	inc/md5.h
	inc/mongo.h
)

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/mongo_c_driver")

include_directories("${CMAKE_BINARY_DIR}/include/mongo_c_driver")

add_library(mongo_c_driver SHARED ${sources} ${headers})

target_link_libraries(mongo_c_driver)