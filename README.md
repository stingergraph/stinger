STINGER
=======

Here begins the reboot of the [STINGER dynamic graph structure](http://www.stingergraph.com/).  
There's a long way to go before this will be useful.  For now, grab the original pure C version of STINGER and hack away.  
Check back here later.

Directory Structure
===================

    .
    ├── obj
    │   ├── bin		    Resulting binary executables
    │   └── lib		    Resulting binary libraries
    ├── inc_lib             Headers for libraries
    │   ├── stinger_core    Minimal STINGER structure (self contained)
    │   └── stinger_util    Convenience funcions for STINGER (dependent on core)
    ├── src_bin		    Executable binary targets - allowed to depend on libraries
    │   ├── standalone	    Stand-alone examples 
    │   ├── stinger_algs    STINGER clients - algorithms to analyze a streaming graph
    │   ├── stinger_server  STINGER server - supports streams, algorithms, and tools
    │   ├── stinger_streams STINGER clients - parse source formats into edge streams
    │   └── stinger_tools   STINGER clients - allow manual exploration / manipulation of the graph
    └── src_lib		    Binary library targets (both dynamic and static)
	├── stinger_core
	└── stinger_util

