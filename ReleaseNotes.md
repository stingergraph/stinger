Release Notes
=============

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

