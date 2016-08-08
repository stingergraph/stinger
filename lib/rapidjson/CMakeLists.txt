
set(headersInternal
	include/internal/pow10.h
	include/internal/stack.h
	include/internal/strfunc.h
)
set(headers
	include/doc_to_val.h
	include/document.h
	include/filestream.h
	include/prettywriter.h
	include/rapidjson.h
	include/reader.h
	include/stringbuffer.h
	include/writer.h
)

publish_headers(headers "${CMAKE_BINARY_DIR}/include/rapidjson")
publish_headers(headersInternal "${CMAKE_BINARY_DIR}/include/rapidjson/internal")
add_custom_target("rapidjson" DEPENDS ${headers} ${headersInternal})
include_directories("${CMAKE_BINARY_DIR}/include/rapidjson")