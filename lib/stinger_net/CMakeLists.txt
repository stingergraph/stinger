set(sources
	src/mon_handling.cpp
	src/send_rcv.cpp
	src/stinger_alg.cpp
	src/stinger_alg_state.cpp
	src/stinger_alg_state_c.cpp
	src/stinger_mon.cpp
	src/stinger_mon_c.cpp
	src/stinger_mon_state.cpp
	src/stinger_server_state.cpp
	src/stinger_stream.cpp
	src/stinger_local_state_c.cpp
)

set(proto
	proto/stinger-alg.proto
	proto/stinger-batch.proto
	proto/stinger-connect.proto
	proto/stinger-monitor.proto
	)

set(headers
	inc/mon_handling.h
	inc/send_rcv.h
	inc/stinger_alg.h
	inc/stinger_alg_state.h
	inc/stinger_alg_state_c.h
	inc/stinger_mon.h
	inc/stinger_mon_c.h
	inc/stinger_mon_state.h
	inc/stinger_server_state.h
	inc/stinger_stream.h
	inc/stinger_stream_state.h
	inc/stinger_local_state_c.h
)

set(_protobuf_proto_path "--proto_path=${CMAKE_CURRENT_LIST_DIR}")
set(_protobuf_include_path "-I=${CMAKE_SOURCE_DIR}/lib")
set(_protobuf_output_path "--cpp_out=${CMAKE_BINARY_DIR}")

#message("Protobuf protopath: ${_protobuf_proto_path}")
#message("Protobuf protopath: ${_protobuf_include_path}")
#message("Protobuf protopath: ${_protobuf_output_path}")

foreach(file ${proto})
	get_filename_component(_file_name ${file} NAME_WE)
	get_filename_component(_abs_file_name ${file} ABSOLUTE)
	
	add_custom_command(
		OUTPUT "${CMAKE_BINARY_DIR}/stinger_net/proto/${_file_name}.pb.cc" "${CMAKE_BINARY_DIR}/stinger_net/proto/${_file_name}.pb.h"
		COMMAND ./protoc ${_protobuf_include_path} ${_protobuf_proto_path} ${_protobuf_output_path} ${_abs_file_name}
		DEPENDS protobuf ${_abs_file_name}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
	)

	set(_generated_files ${_generated_files} "${CMAKE_BINARY_DIR}/stinger_net/proto/${_file_name}.pb.cc" "${CMAKE_BINARY_DIR}/stinger_net/proto/${_file_name}.pb.h")
endforeach()

publish_headers(headers "${CMAKE_BINARY_DIR}/include/stinger_net")

include_directories("${CMAKE_BINARY_DIR}/")
include_directories("${CMAKE_BINARY_DIR}/stinger_net")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_net")

set_source_files_properties(${_generated_files} PROPERTIES GENERATED TRUE)
#message("Files: ${_generated_files}")
add_library(stinger_net SHARED ${sources} ${headers} ${_generated_files})
target_link_libraries(stinger_net _protobuf_library _protobuf_lite_library _protobuf_library stinger_core stinger_utils)
