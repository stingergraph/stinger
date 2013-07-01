STINGER
=======

Learn more at [stingergraph.com | http://stingergraph.com]

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

STINGER is built using [CMake | http://www.cmake.org].  From the root of STINGER, first create a build directory:
    mkdir build && cd build

Then call CMake from that build directory to automatically configure the build and to create a Makefile:
    cmake ..

Finally, call make to build all libraries and executable targets (or call make and the name of an executable or library to build):
    make
