set(sources
	src/pagerank_updating.c
)

include_directories(${CMAKE_CURRENT_SRC_DIR})

add_executable(stinger_pagerank_updating ${sources})
target_link_libraries(stinger_pagerank_updating stinger_net stinger_alg)
