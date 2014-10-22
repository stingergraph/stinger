Overview
========

The JSON-RPC server provides a JSON-RPC 2.0 endpoint for client applications to 
request algorithm and graph data from STINGER.  See the [JSON-RPC Specification](http://www.jsonrpc.org/specification) 
for more details.

NOTE: This JSON-RPC server only processes parameters by name, not by position.

CAVEAT: This JSON-RPC server does not currently implement notifications.

CAVEAT: This JSON-RPC server does not currently implement batches.


Method Summary
==============

## get_server_info

This method returns basic information about the STINGER server.

### Input

This method takes no parameters.

    {
      "jsonrpc": "2.0",
      "method": "get_server_info",
      "id": 13
    }

### Output

pid: The process ID of the STINGER server process.

    {
      "jsonrpc": "2.0",
      "result": {
        "pid": 2776392484925547500
      },
      "id": 13,
      "millis": 0.21
    }


## get_graph_stats

This method returns summary statistics about the current state of the graph.

### Input

get_types: If True, will return strings associated with edge and vertex types.

    {
      "jsonrpc": "2.0",
      "method": "get_graph_stats",
      "params" : {
        "get_types": Boolean                   /* OPTIONAL */
      },
      "id": 7
    }

### Output

    {
      "jsonrpc": "2.0",
      "result": {
        "vertices": 178,
        "edges": 200,
        "time": 100,
        "edge_types": 1,
        "vertex_types": 1
      },
      "id": 7,
      "millis": 0.38
    }

    {
      "jsonrpc": "2.0",
      "result": {
        "vertices": 178,
        "edges": 200,
        "time": 100,
        "edge_types": [
          "None"
        ],
        "vertex_types": [
          "None"
        ]
      },
      "id": 7,
      "millis": 1.01
    }


## get_algorithms

Returns a list of algorithms currently running and connected to the STINGER server.

### Input

This method takes no parameters.

    {
      "jsonrpc": "2.0",
      "method": "get_algorithms",
      "id": 1
    }

## Output

    {
      "jsonrpc": "2.0",
      "result": {
        "algorithms": [
          "pagerank",
          "rate_monitor",
          "stinger"
        ]
      },
      "id": 1,
      "millis": 0.24
    }

## get_data_descriptiona

This method retrieves the data fields published by an algorithm.

### Input

name: String identifier of an algorithm

    {
      "jsonrpc": "2.0",
      "method": "get_data_description",
      "params": {
        "name": "pagerank"
      },
      "id": 2
    }

### Output

    {
      "jsonrpc": "2.0",
      "result": {
        "alg_data": [ String ]
      },
      "id": 2,
      "millis": 0.71
    }

## get_data_array

Returns the entire data array for one algorithm field.

### Input

name: String identifier of an algorithm
data: String identifier of a data field 

    {
      "jsonrpc": "2.0",
      "method": "get_data_array",
      "params": {
        "name": "algorithm_name",
        "data": "data_array_name",
        "strings": Boolean,                    /* Optional */
        "stride": Integer,                     /* Optional */
        "samples": Integer,                    /* Optional */
        "log": Boolean                         /* Optional */
      },
      "id": 3
    }

### Output

    {
      "jsonrpc": "2.0",
      "result": {
        "time": 100,
        "pagerank": {
          "offset": 0,
          "count": 178,
          "vertex_id": [ Integer ]
          "vertex_str": [ String ]
          "value": [ Number ]
        }
      },
      "id": 5,
      "millis": 0.3
    }


## get_data_array_range

Returns a range of data values based on an offset and count.

### Input

    {
      "jsonrpc": "2.0",
      "method": "get_data_array_range",
      "params": {
        "name": "algorithm_name",
        "data": "data_array_name",
        "offset": Integer,
        "count": Integer,
        "strings": Boolean,                    /* Optional */
        "stride": Integer,                     /* Optional */
        "samples": Integer,                    /* Optional */
        "log": Boolean                         /* Optional */
      },
      "id": 4
    }

### Output

    {
      "jsonrpc": "2.0",
      "result": {
        "time": 100,
        "pagerank": {
          "offset": 0,
          "count": 178,
          "vertex_id": [ Integer ]
          "vertex_str": [ String ]
          "value": [ Number ]
        }
      },
      "id": 5,
      "millis": 0.3
    }


## get_data_array_sorted_range

Returns a range of data values based on an offset and count in sorted order.

### Input

    {
      "jsonrpc": "2.0",
      "method": "get_data_array_sorted_range",
      "params": {
        "name": "algorithm_name",
        "data": "data_array_name",
        "offset": Integer,
        "count": Integer,
        "order": "ASC/DESC",                   /* OPTIONAL - Default: DESC */
        "strings": Boolean,                    /* OPTIONAL */
        "stride": Integer,                     /* OPTIONAL */
        "samples": Integer,                    /* OPTIONAL */
        "log": Boolean                         /* OPTIONAL */
      },
      "id": 5
    }

### Output

    {
      "jsonrpc": "2.0",
      "result": {
        "time": 100,
        "pagerank": {
          "offset": 0,
          "count": 178,
          "order": "DESC",
          "vertex_id": [ Integer ]
          "vertex_str": [ String ]
          "value": [ Number ]
        }
      },
      "id": 5,
      "millis": 0.3
    }


## get_data_array_set

Returns data values for specific vertices.

### Input

    {
      "jsonrpc": "2.0",
      "method": "get_data_array_set",
      "params": {
        "name": "algorithm_name",
        "data": "data_array_name",
        "set": [
          1,
          2,
          3,
          4,
          5
        ],
        "strings": Boolean                     /* OPTIONAL */
      },
      "id": 6
    }

### Output

    {
      "jsonrpc": "2.0",
      "result": {
        "time": 100,
        "pagerank": {
          "vertex_id": [ Integer ]
          "vertex_str": [ String ]
          "value": [ Number ]
        }
      },
      "id": 5,
      "millis": 0.3
    }

## get_data_array_reduction

Perform a reduction over the entire data array and return a single value.

### Input

    {
      "jsonrpc": "2.0",
      "method": "get_data_array_reduction",
      "params": {
        "name": "pagerank",
        "data": "pagerank",
        "op": "sum"
      },
      "id": 12
    }

### Output

    {
      "jsonrpc": "2.0",
      "result": {
        "time": 100,
        "pagerank": [
          {
            "field": String,
            "value": Number 
          }
        ]
      },
      "id": 12,
      "millis": 16.71
    }


On-demand Analytics
===================

## breadth_first_search

Perform a breadth-first search from _source_ to _target_ and return edges along the shortest paths.

### Input

* source: Vertex ID or String
* target: Vertex ID or String
* strings: If True, return vertex identifier strings
* get_types: If True, return string identifiers for edge and vertex types
* get_etypes: If True, return string identifiers for edge types
* get_vtypes: If True, return string identifiers for vertex types

    {
      "jsonrpc": "2.0",
      "method": "breadth_first_search",
      "params": {
        "source": Integer/String,
        "target": Integer/String,
        "strings": Boolean,                    /* OPTIONAL */
        "get_types": Boolean,                  /* OPTIONAL */
        "get_etypes": Boolean,                 /* OPTIONAL */
        "get_vtypes": Boolean                  /* OPTIONAL */
      },
      "id": Integer
    }

### Output

    {
      "jsonrpc": "2.0",
      "result": {
        "subgraph": []
      },
      "id": Integer,
      "millis": Float
    }


## adamic_adar_index

Calculates the Adamic-Adar score for all vertices not adjacent to _source_.

### Input

* source: Vertex ID or String
* strings: If True, return vertex identifier strings
* include_neighbors: Also calculate Adamic-Adar index for neighbors of _source_

```
    {
      "jsonrpc": "2.0",
      "method": "adamic_adar_index",
      "params": {
        "source": Integer/String,
        "strings": Boolean,                    /* OPTIONAL */
        "include_neighbors": Boolean           /* OPTIONAL */
      },
      "id": Integer
    }
```

### Output

    {
      "jsonrpc": "2.0",
      "result": {
        "source": Integer,
        "source_str": String,
        "vertex_id": [ Integer ],
        "vertex_str": [ String ],
        "value": [ Double ]
      },
      "id": Integer,
      "millis": Float
    }


## label_breadth_first_search


## label_mod_expand


Session-based Methods
=====================

## register

### Input

    {
      "jsonrpc": "2.0",
      "method": "register",
      "params": {
        "type": "",
        "strings": Boolean                     /* OPTIONAL */
      },
      "id": 9
    }

### Output


## request

### Input

    {
      "jsonrpc": "2.0",
      "method": "request",
      "params": {
        "session_id": Integer,
        "strings": Boolean                     /* OPTIONAL */
      },
      "id": 10
    }

### Output


