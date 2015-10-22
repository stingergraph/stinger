set(sources
	src/bsutil.h
	src/community-update.c
	src/community-update.h
	src/community.c
	src/community.h
	src/compat.h
	src/defs.h
	src/graph-el.c
	src/graph-el.h
	src/simple_communities.c
	src/sorts.c
	src/sorts.h
	src/xmt-luc.cc
	src/xmt-luc.h
)

include_directories(${CMAKE_CURRENT_SRC_DIR})

add_executable(stinger_simple_communities ${sources})
target_link_libraries(stinger_simple_communities stinger_net)