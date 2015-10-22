Release Notes
=============

Version
-------

These notes are for STINGER version 15.10

Notes
-----

- STINGER directory structure and CMakeLists.txt have changed to support parallel build
  - lib/ directory houses the STINGER libraries
  - src/ directory houses the supporting binaries
  - external/ directory houses external dependencies
  - Can now build with make -j8 after performing cmake in the build directory
- Prevent remapping of graph on each batch sent over protobuf (fixing a memory leak)
- STINGER management web interface merged in
  - Hosted under the util/management directory
  - Flask API merged with the management console flask API and is now in util/flask
  - Old STINGER web interface now in util/graph_explorer
- Flask API improvements
  - Fix for empty batches
  - Optimization on sending batches
  - Flask is fully aware of directedness and is consistent with STINGER
  - Added a Swagger UI documentation to the Flask API
  - Added requirements.txt for easy dependency installation (Use pip install -r requirements.txt)
- PageRank changes
  - A PageRank that works on a directed graph was added
  - A PageRank that works over a subset of vertices was added. A JSON RPC call was also added to call this method
- stinger_alg
  - Most algorithms are now in the lib/stinger_alg directory
  - Algorithms are now uniformly used between the client applications and the JSON RPC server
- netflow_stream
  - Bug fixes
- Adamic Adar
  - Bug fix to the calculation to now be right for undirected graphs
- Numerous whitespace fixes (removing tabs)
- Refactored array_to_json_monolithic to reduce code redundancy
- Added type filtering to JSON requests on get_data_array_*
- Betweenness Centrality
  - Uses a BFS style solving approach now that is compatible with directed graphs and has a near identical runtime to old version
- Changed the default STINGER to be 3/4 of the specified memory size
- Added a configuration file option to the server that uses libconfig
  - Can be used to specify number of vertices, edge types, memory size, etc.
  - Can pre-map edge type strings and vertex type strings
  - stinger_new_full and stinger_shared_new_full API changed to support a robust config parameter
- Traversal macros had several bugs that were squashed
- Updated Readme.md to reference config file changes
- Added Testing infrastructure
  - Using googletest for tests
  - Added rule to CMake for `make check` that runs all unit tests
  - Updated travis.yml to automatically run make check on any new PRs
- travis.yml
  - Fixed travis to run in a docker container allowing for faster CI tests

Version
-------

These notes are for STINGER version 06.15.

Notes
-----

- All executable names now prepended with "stinger_"
- Switch to pthreads instead of fork() in STINGER server
- Switch to UNIX domain sockets for algorithm/monitor communication (TCP is supported through ccmake)
- STINGER batch server drops new batches when the queue reaches 100
- Monitor process can reconnect without timeout
- New sparse matrix-sparse vector client
- New streaming PageRank implementation
- Add Metis file support to the STINGER server
- New Python Flask interface for STINGER
- Improvements to the random edge generator stream
- Changes to random edge generator stream string generation
- Catch signals and clean up in the STINGER server
- New test routines for STINGER names
- STINGER names structure can now resize when full
- Consistency check when saving STINGER to disk or recovering from disk
- New monitor process that dumps graph edges to disk
- New performance counters available in JSON-RPC server
- New NetFlow v5 stream
- New weakly connected components algorithm
- New connected components algorithm based on edge types
- JSON-RPC server reports execution time for every query
- Add JSON-RPC method for Adamic-Adar score
- Add JSON-RPC method to report STINGER health statistics
- ADD JSON-RPC method to report all registered methods
- Add JSON-RPC method for labeled breadth-first search
- Add JSON-RPC method for seed set expansion
- Add JSON-RPC method for egonet
- Cache block alignment of STINGER edge blocks
- Clean up /dev/shmem at STINGER server exit
- Fixed threading and signals for Mac OS X
- Bug fixes in src/lib/stinger_net/src/stinger_mon.cpp
- Bug fix in stinger_set_initial_edges()
- Bug fix in stinger_names_create_type()
- Bug fix in stinger_physmap_id_get()
- Bug fixes in JSON-RPC server
- Bug fix in JSON-RPC breadth-first search
- Bug fix in PageRank algorithm
- Bug fix in src/bin/clients/tools/json_rpc_server/src/rpc_state.cpp
- Bug fix in src/bin/clients/tools/json_rpc_server/src/mon_handling.cpp
- Bug fix in stinger_incr_edge()
- Bug fix in sampling betweenness centrality algorithm

