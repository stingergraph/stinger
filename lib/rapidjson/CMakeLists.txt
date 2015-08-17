
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

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/rapidjson")
file(COPY ${headersInternal} DESTINATION "${CMAKE_BINARY_DIR}/include/rapidjson/internal")

include_directories("${CMAKE_BINARY_DIR}/include/rapidjson")