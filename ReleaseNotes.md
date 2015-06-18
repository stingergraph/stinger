Release Notes
=============

Version
-------

These notes are for STINGER version 06.15.

Notes
-----

- New sparse matrix-sparse vector client
- New streaming PageRank implementation
- Add Metis file support to the STINGER server
- Improvements to the random edge generator stream
- Changes to random edge generator stream string generation
- Clean up /dev/shmem at STINGER server exit
- Bug fixes in src/lib/stinger_net/src/stinger_mon.cpp
- Switch to pthreads instead of fork() in STINGER server
- Catch signals and clean up in the STINGER server
- Add JSON-RPC method for Adamic-Adar score
- JSON-RPC server reports execution time for every query
- Add JSON-RPC method to report STINGER health statistics
- ADD JSON-RPC method to report all registered methods
- New test routines for STINGER names
- Bug fix in stinger_set_initial_edges()
- Bug fix in stinger_names_create_type()
- Consistency check when saving STINGER to disk or recovering from disk
- STINGER names structure can now resize when fulla
- Bug fixes in JSON-RPC server
- Bug fix in breadth-first search
- Bug fix in PageRank
- Add JSON-RPC method for egonet
- Monitor process can reconnect without timeout
- New monitor process that dumps graph edges to disk
- Switch to UNIX domain sockets for algorithm/monitor communication (TCP is supported through ccmake)
- New performance counters available in JSON-RPC server
- STINGER batch server drops new batches when the queue reaches 100
- New NetFlow v5 stream
- Bug fix in src/bin/clients/tools/json_rpc_server/src/rpc_state.cpp
- Bug fix in src/bin/clients/tools/json_rpc_server/src/mon_handling.cpp
- New weakly connected components algorithm
- New connected components based on edge types
- All executable names are prepended with "stinger_"
- Fixed threading and signals for Mac OS X
- Python Flask interface for STINGER
- Bug fix in stinger_physmap_id_get()
- Cache block alignment of STINGER edge blocks
- Bug fix in stinger_incr_edge()
- Bug fix in sampling betweenness centrality algorithm
- Add JSON-RPC method for labeled breadth-first search
- Add JSON-RPC method for seed set expansion

