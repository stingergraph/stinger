STINGER
=======
[![Build Status (master)](https://travis-ci.org/stingergraph/stinger.png?branch=master)](https://travis-ci.org/stingergraph/stinger)
[![Build Status (dev)](https://travis-ci.org/stingergraph/stinger.png?branch=dev)](https://travis-ci.org/stginergraph/stinger)

Learn more at [stingergraph.com](http://stingergraph.com).

What is STINGER?
=============

STINGER is a package designed to support streaming graph analytics by using in-memory parallel computation to accelerate the computation.  STINGER is composed of the core data structure and the STINGER server, algorithms, and an RPC server that can be used to run queries and serve visualizations.  The directory structure of the components is as follows:

    doc/ - Documentation
    doxygen/ - Doxygen generated documentation
    external/ - External dependencies packaged with STINGER
    flask/ - Python Flask Relay Server for interacting with the JSON RPC server and STINGER server
    html/ - Basic web pages that communicate with the JSON RPC server
    lib/ - The STINGER library and dependencies
        stinger_alg/ - Algorithm kernels that work on the STINGER data structure
        stinger_core/ - The Core STINGER data structure
        stinger_net/ - Libraries for communicating over unix sockets and/or TCP/IP using Protobufs
        stinger_utils/ - Auxiliary functions over the data structure
    src/ - STINGER ecosystem binaries
        server/ - The STINGER server
        clients/ - Clients that connect to the STINGER server
            algorithms/ - Streaming Algorithm binaries
            streams/ - Binaries for streaming in new data
            tools/ - Auxiliary tools
        py/stinger - Python bindings to STINGER
        standalone/ - Standalone binaries that use the STINGER core data structure
        templates/json - Common templates for stream ingest
        tests/ - Tests for the STINGER data structure and algorithms
    SOURCEME.sh - File to be executed from out-of-source build directory to link the html/ web pages with the STINGER server

Building
========

STINGER is built using [CMake](http://www.cmake.org).  From the root of STINGER, first create a build directory:

    mkdir build && cd build
    . ../SOURCEME.sh

Then call CMake from that build directory to automatically configure the build and to create a Makefile:

    ccmake .. 

Change `Release` to `Debug` or `RelWithDebInfo` for a debugging build during development.  Finally, call make to build all libraries and executable targets (or call make and the name of an executable or library to build):

    make -j8

Note: the -j flag is a multi-threaded build.  Typically you should match the argument to the number of cores on your system.  

All binary targets will be built and placed in build/bin.  They are named according to the folder from which they were 
built (so src/bin/server produces build/bin/stinger\_server, src/bin/clients/tools/json\_rpc\_server produces 
build/bin/stinger\_json\_rpc\_server, etc.).  If you ran SOURCEME.sh from the build directory as instructed above, the build/bin
directory is appended to your path.

Executable Targets
==================

As indicated by the directory structure, there are three primary types of executable targets: clients, server, and standalone, 
and subtypes in the case of clients.

The STINGER server maintains a STINGER graph in memory and can maintain multiple connections with client streams, algorithms, and monitors.

Client streams can send edges consisting of source, destination, weight, time, and type where some fields are optional 
and others can optionally be text strings.

Client algorithms will receive these batches of updates in a 
synchronous manner, as well as shared-memory read-only access to the complete graph.  The server provides the capability
for client algorithms to request a shared memory space to store results and communicate with other algorithms.  
Client algorithms declare dependencies when they connect and receive the mapped data in the returned structure.
The server guarantees that all of an algorithm's dependencies will finish processing before that algorithm is executed.
Clients algorithms are required to provide a description string that indicates what data is stored and the type of the data.  

Client tools are intended to be read-only, but are notified of all running algorithms and shared data.  An example
of this kind of client is the JSON-RPC server (src/bin/clients/tools/json\_rpc\_server).  This server provides access
to shared algorithm data via JSON-RPC calls over HTTP.  Additionally, some primitive operations are provided to support
selecting the top vertices as scored by a particular algorithm or obtaining the shortest paths between two vertices, for example.

Standalone executables are generally self-contained and use the STINGER
libraries for the graph structure and supporting functions.  Most of the existing standalone executables demonstrate
a single streaming or static algorithm on a synthetic R-MAT graph and edge stream.

Running
=======

Using the Server
----------------
To run an example using the server and five terminals:

    term1:build$ env STINGER_MAX_MEMSIZE=1G ./bin/stinger_server
    term2:build$ ./bin/stinger_json_rpc_server
    term3:build$ ./bin/stinger_static_components
    term4:build$ ./bin/stinger_pagerank
    term5:build$ ./bin/stinger_rmat_edge_generator -n 100000 -x 10000

This will start a stream of R-MAT edges over 100,000 vertices in batches of 10,000 edges.  A connected component labeling
and PageRank scoring will be maintained.  The JSON-RPC server will host interactive web pages at 
http://localhost:8088/full.html are powered by the live streaming analysis.  The total memory usage of the dynamic graph is limited to 1 GiB.

Server Configuration
--------------------

### CMake Parameters

The STINGER is configured via several methods when using the STINGER Server.  There are several CMake Parameters that can be set when configuring the build via CMake.  These are listed below.

- ``STINGER_DEFAULT_VERTICES`` -> Specifies the number of vertices
- ``STINGER_DEFAULT_NEB_FACTOR`` -> Specifies the Number of Edge Blocks allocated for each vertex per edge type
- ``STINGER_DEFAULT_NUMETYPES`` -> Specifies the Number of Edge Types
- ``STINGER_DEFAULT_NUMVTYPES`` -> Specifies the Number of Vertex Types
- ``STINGER_EDGEBLOCKSIZE`` -> Specifies the minimum number of edges of each type that can be attached to a vertex.  This should be a multiple of 2
- ``STINGER_NAME_STR_MAX`` -> Specifies the maximum length of strings when specifying edge type names and vertex type names
- ``STINGER_NAME_USE_SQLITE`` -> (Alpha - Still under development) Replaces the static names structure with a dynamic structure that uses SQLITE
- ``STINGER_USE_TCP`` -> Uses TCP instead of Unix Sockets to connect to the STINGER server

** Note: STINGER, by default, uses one edge type name and vertex type name for the "None" type.  This can be disabled using the STINGER server config file described in a later section.

### Example

Suppose there is a graph you want to store with a power-law distribution of edges, and the average degree of each vertex is 3.  There are 3 named edge types and 4 name vertex types.  You wish to store up to 2 million vertices. A good configuration would be

    STINGER_DEFAULT_VERTICES = 1 << 21
    STINGER_EDGEBLOCKSIZE = 4            # On average one edge block is sufficient for most vertices
    STINGER_DEFAULT_NEB_FACTOR = 2       # Depending on the skewness 3 may also be necessary.  This will create 2 * nv edge blocks for each edge types
    STINGER_DEFAULT_NUMETYPES = 4        # There are 3 named types and the default "None" type
    STINGER_DEFAULT_NUMVTYPES = 5        # There are 4 named types and the default "None" type

### Environment Variables

By default, STINGER will attempt to allocate 1/2 of the available memory on the system.  This can be overwritten using an environment variable, ``STINGER_MAX_MEMSIZE``.

Example:
  > ``STINGER_MAX_MEMSIZE=4g ./bin/stinger_server``

If the STINGER parameters provided would create a STINGER that is larger than STINGER_MAX_MEMSIZE, then the STINGER is shrunk to fit the specified memory size.  This is done by repeatedly reducing the number of vertices by 25% until the STINGER fits in the specified memory size.

### Config File

The STINGER server accepts a config file format in the libconfig style.  A config filename can be passed as an argument to the ``stinger_server`` with the ``-C`` flag.  An example config file is provided in stinger.cfg in the root directory of the STINGER repository.  All of the cmake configurations regarding the size of the STINGER can be overridden via the config file __except for the ``STINGER_EDGEBLOCKSIZE``__.  The config file values will override any CMake values.  If the ``STINGER_MAX_MEMSIZE`` environment variable is set, it will be considered an upper limit on the memory size.

The following values are allowed in stinger.cfg:

- ``num_vertices`` -> A __long__ integer representing the number of vertices.  __Must have the L at the end of the integer__
- ``edges_per_type`` -> A __long__ integer representing the number of edges for each vertex type.  __Must have the L at the end of the integer__
- ``num_edge_types`` -> Specifies number of edge types.
- ``num_vertex_types`` -> Specifies number of vertex types.
- ``max_memsize`` -> Specifies maximum memory for STINGER.  Cannot be bigger than the ``STINGER_MAX_MEMSIZE`` environment variable.  Uses the same format as the environment variable (e.g. "4G", "1T", "512M")
- ``edge_type_names`` -> A list of edge type names to map at STINGER startup.  If this list is specified the "None" type will not be mapped.
- ``vertex_type_names`` -> A list of vertex type names to map at STINGER startup.  If this list is specified the "None" type will not be mapped.
- ``map_none_etype`` -> If set to false, the "None" edge type will not be mapped at startup.  Likewise, if true, the "None" edge type will be mapped at startup.
- ``map_none_vtype`` -> If set to false, the "None" vertex type will not be mapped at startup.  Likewise, if true, the "None" vertex type will be mapped at startup.
- ``no_resize`` -> If set to true, the the STINGER will not try to resize and fit in the specified memory size.  Instead it will throw an error and exit.


Example: Parsing Twitter
------------------------

Given a stream of Tweets in Twitter's default format (a stream of JSON objects, one per line), it is fairly easy to pipe
the user mentions / retweets graph into STINGER using the json\_stream.  The json\_stream is a templated JSON stream parser
designed to consume one object per line like the Twitter stream and to produce edges from this stream based on a template.

The templates can use the following variables (where one of the two source and one of the two destination variables 
must be used):

    $source_str         - The source vertex name.
    $source             - The source of the edge as a number (must be able to parse as an integer 
                          less than the maximum vertex ID in the STINGER server).
    $source_type        - A string representing the type of the source vertex.
    $source_weight      - A number to be added to the weight of the source vertex (vertex weights
                          start at zero).
    $destination_str    - The destination vertex name
    $destination        - The destination of the edge as a number (must be able to parse as an 
                          integer less than the maximum vertex ID in the STINGER server).
    $destination_type   - A string representing the type of the destination vertex
    $destination_weight - A number to be added to the weight of the destination vertex (vertex 
                          weights start at zero).
    $type_str           - The edge type as a string
    $weight             - The weight of the edge (must be able to parse as an integer).
    $time               - The time of the edge (must be able to parse as an integer).
    $time_ttr           - Must be a string of either the form "Mon Sep 24 03:35:21 +0000 2012" or 
                          "Sun, 28 Oct 2012 17:32:08 +0000".  These will be converted internally
                          into integers of the form YYYYMMDDHHMMSS.  Note that this does not currently support
                          setting a constant value.

For example, the simplest template for Twitter mentions and retweets would be (we'll call this template.json):

    {
      "user": {
        "screen_name": "$source_str1"
      },
      "entities": {
        "user_mentions": [
          {
            "screen_name": "$destination_str1"
          }
        ]
      },
      "this_doesnt_matter": "$source_type=user",
      "same_here": "$destination_type=user",
      "and_here": "$type=mention"
    }

To parse a Twitter stream into STINGER using this template:

    cat twitter_sample.json | ./bin/stinger_json_stream template.json

You can replace the 'cat twitter\_sample.json' command with one of the curl commands from the Twitter developer
API page to directly inject a live Twitter stream (obviously you should go to dev.twitter.com to get your 
own OAuth data):

    curl --request 'POST' 'https://stream.twitter.com/1.1/statuses/sample.json' --header 
    'Authorization: OAuth oauth_consumer_key="KEYKEYKEY", oauth_nonce="NONCENONCENONCE", 
    oauth_signature="SIGSIGSIG", oauth_signature_method="HMAC-SHA1", oauth_timestamp="ts", 
    oauth_token="TOKENTOKENTOKEN", oauth_version="1.0"' --verbose | ./bin/json_stream template.json

Example: Parsing CSV Files / Streams
---------------------------

The csv\_stream parser follows a similar templated format to the json parser, so parsing edges out of a file might look like:

    id,email_a,config_a,email_b,config_b,unix_time,length
    na,$source_str1,na,$destination_str1,na,$time1,$weight1, $source_type1=email, $destination_type1=email

This file would create edges between email addresses using the length field as the weight and the Unix timestamp field as the time.  To use this template, pipe the file or stream into the parser and pass the template as a parameter like so:

    cat emails.csv | ./bin/stinger_csv_stream template.csv

Please be aware that the CSV parser and the underlying code to parse CSV files does not currently trim whitespace, and does not treat quoted strings of any kind as quoted.

Using a Standalone Client
-------------------------
To create a toy R-MAT graph (256K vertices and 2M undirected edges) and run the insert-remove benchmark:

    term1:build$ stinger_rmat_graph_generator -s 18 -e 8 -n 100000
    term1:build$ stinger_insert_remove_benchmark -n 1 -b 100000 g.18.8.bin a.18.8.100000.bin
    
    
[![githalytics.com alpha](https://cruel-carlota.pagodabox.com/a7d82e7aa670122314238336dbbd7c89 "githalytics.com")](http://githalytics.com/robmccoll/stinger)

Handling Common Errors
======================

Runtime Issues
--------------

STINGER allocates and manages its own memory. When STINGER starts, it allocates one large block of memory (enough to hold its maximum size), and then manages its own memory allocation from that pool.  The server version of STINGER does this in shared memory so that multiple processes can see the graph.  Unfortunately, the error handling for memory allocations is not particularly user-friendly at the moment.  Changing the way that this works is on the issues list (see https://github.com/robmccoll/stinger/issues/8).

- "Bus error" when running the server: The size of STINGER that the server is trying to allocate is too large for your memory.  First try to increase the size of your /dev/shm (See below).  If this does not work reduce the size of your STINGER and recompile.
- "XXX: eb pool exhausted" or "STINGER has run out of internal storage space" when running the server, standalone executables, or anything else using stinger\_core: you have run out of internal edge storage.  Increase the size of STINGER and recompile.

See the section on STINGER server configuration to adjust the sizing of STINGER

Resizing /dev/shm
--------------

/dev/shm allows for memory-mapped files in the shared memory space.  By default the size of this filesystem is set to be 1/2 the total memory in the system.  STINGER by default attempts to allocate 3/4 of the total memory (unless the STINGER_MAX_MEMSIZE environment variable is used).  This will commonly cause a 'Bus Error' the first time STINGER server is run on the system.  There are two solutions to this problem

1. Use STINGER_MAX_MEMSIZE and provide a value no greater than 1/2 the total system memory.
2. Resize the /dev/shm filesystem to match the total system memory size.

To achieve #2 you should edit /etc/fstab to include the following line

### On Ubuntu

    tmpfs /run/shm tmpfs defaults,noexec,nosuid,size=32G 0 0

Replace 32G with the size of your system's main memory

### On most other systems

    tmpfs /dev/shm tmpfs defaults,noexec,nosuid,size=32G 0 0

Replace 32G with the size of your system's main memory 

Once you have added this line either restart the machine or run the following command.

  sudo mount -o remount /run/shm  


Build Problems
--------------

First check the STINGER GitHub page to verify that the build is passing (icon immediately under the title).  If it is not passing, the issue resides within the current version itself.  Please checkout a previous revision - the build should be fixed shortly as failing builds sent notifications to the authors.  

Build problems after pulling updates are frequently the result of changes to the Protocol Buffer formats used in STINGER.  These are currently unavoidable as an unfortunate side effect of how we distribute PB and tie it into CMake.  To fix, remove your build directory and build STINGER from scratch.

Additionally, this version of the STINGER tool suite is tested almost exclusively on Linux machines running later version of Ubuntu and Fedora.  While we  would like to have multi-platform compatibility with Mac (via "real" GCC) and Windows (via GCC on cygwin), these are lower priority for our team - unless a project sponsor requires it :-)  

