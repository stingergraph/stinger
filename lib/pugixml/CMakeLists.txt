set(sources
	src/pugixml.cpp
)
set(headers
	inc/foreach.hpp
	inc/pugiconfig.hpp
	inc/pugixml.hpp
)

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/pugixml")

include_directories("${CMAKE_BINARY_DIR}/include/pugixml")

add_library(pugixml SHARED ${sources} ${headers})