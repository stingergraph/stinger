STINGER
=======
[![Build Status](https://travis-ci.org/robmccoll/stinger.png?branch=master)](https://travis-ci.org/robmccoll/stinger)

Learn more at [stingergraph.com](http://stingergraph.com).

Directory Structure
===================

    .
    ├── CMakeLists.txt
    ├── README.md
    ├── SOURCEME.sh
    └── src
	├── bin
	│   ├── clients
	│   │   ├── algorithms
	│   │   ├── streams
	│   │   └── tools
	│   ├── server
	│   └── standalone
	│       ├── breadth_first_search
	│       ├── community_reagglomeration
	│       ├── connected_components
	│       ├── insert_remove_benchmark
	│       ├── protobuf_test
	│       ├── streaming_clustering_coefficients
	│       └── streaming_connected_components
	└── lib
	    ├── fmemopen
	    ├── int_hm_seq
	    ├── int_ht_seq
	    ├── intvec
	    ├── kv_store
	    ├── protobuf
	    ├── pugixml
	    ├── stinger_core
	    ├── stinger_utils
	    ├── string
	    └── vtx_set

Building
========

STINGER is built using [CMake](http://www.cmake.org).  From the root of STINGER, first create a build directory:

    mkdir build && cd build
    . ../SOURCME.sh

Then call CMake from that build directory to automatically configure the build and to create a Makefile:

    cmake ..

Finally, call make to build all libraries and executable targets (or call make and the name of an executable or library to build):

    make

All binary targets will be built and placed in build/bin.  They are named according to the folder from which they were 
built (so src/bin/server produces build/bin/server, src/bin/clients/tools/json\_rpc\_server produces 
build/bin/json\_rpc\_server, etc.).  If you ran SOURCEME.sh from the build directory as instructed above, the build/bin
directory is appended to your path.

All library targets are built as both static and shared libraries by default and are placed in build/lib as .so and .a files
(or .dylib on Mac).  Headers for these libraries are copied into build/include/library\_name.  The build/include directory
is in the include path of all targets.

Executable Targets
==================

As indicated by the directory structure, there are three primary types of targets (client, server, standalone) 
and subtypes in the case of clients.

Standalone executables are generally self-contained and use the STINGER
libraries for the graph structure and supporting functions.  Most of the existing standalone executables demonstrate
a single streaming or static algorithm on a synthetic R-MAT graph and edge stream.

The STINGER server maintains a STINGER graph in memory and can maintain multiple connections with clients.

Client streams can send edges consisting of source, destination, weight, time, and type where some fields are optional 
and others can optionally be text strings.

Client algorithms will receive these batches of updates in a somewhat 
synchronous manner as well as shared-memory read only access to the complete graph.  The server provides the capability
for client algorithms to request a shared memory space to store results and communicate with other algorithms.  
Client algorithms declare dependencies when they connect and receive the mapped data in the returned structure.
The server guarantees that all of an algorithm's dependencies will finish processing before that algorithm is executed.
Clients algorithms are required to provide a description string that indicates what data is stored and the type of the data.  

Client tools are intended to be read-only, but are notified of all running algorithms and shared data.  An example
of this kind of client is the JSON RPC server (src/bin/clients/tools/json\_rpc\_server).  This server provides access
to shared algorithm data via JSON RPC calls over HTTP.  Additionally, some primitive operations are provided to support
selecting the top vertices as scored by a particular algorithm or obtaining the shortest paths between two vertices.

Running
=======

Using the Server
----------------
To run an example using the server and five terminals:

    term1:build$ server
    term2:build$ json_rpc_server
    term3:build$ static_components
    term4:build$ pagerank
    term5:build$ rmat_edge_generator -n 100000 -x 10000

This will start a stream of R-MAT edges over 100,000 vertices in batches of 10,000 edges.  A connected component labeling
and PageRank scoring will be maintained.  The JSON RPC server will host interactive web pages at 
http://localhost:8088/data/page\_rank\_top.html, page\_rank\_group.html, and components.html that are powered by
the live streaming analysis.

Using a Standalone Client
-------------------------
To create a toy R-MAT graph (256K vertices and 2M undirected edges) and run the insert-remove benchmark:

    term1:build$ rmat_graph_generator -s 18 -e 8
    term1:build$ insert_remove_benchmark -n 1 -b 100000 g.18.8.bin a.18.8.100000.bin
