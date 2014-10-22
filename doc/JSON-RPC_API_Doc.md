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

```
    {
      "jsonrpc": "2.0",
      "method": "get_server_info",
      "id": Integer
    }
```

### Output

* pid: The process ID of the STINGER server process.

```
    {
      "jsonrpc": "2.0",
      "result": {
        "pid": Integer 
      },
      "id": Integer,
      "millis": Float
    }
```


## get_graph_stats

This method returns summary statistics about the current state of the graph.

### Input

* get_types: If True, will return strings associated with edge and vertex types.

```
    {
      "jsonrpc": "2.0",
      "method": "get_graph_stats",
      "params" : {
        "get_types": Boolean                   /* OPTIONAL */
      },
      "id": Integer
    }
```

### Output

* vertices: Number of vertices in the graph
* edges: Number of directed edges in the graph
* time: Maximum timestamp in STINGER
* edge_types: Number of edge types in use
* vertex_types: Number of vertex types in use

```
    {
      "jsonrpc": "2.0",
      "result": {
        "vertices": Integer,
        "edges": Integer,
        "time": Integer,
        "edge_types": Integer,
        "vertex_types": Integer
      },
      "id": Integer,
      "millis": Float
    }
```

* edge_types: Array of edge type strings
* vertex_types: Array of vertex type strings

```
    {
      "jsonrpc": "2.0",
      "result": {
        "vertices": Integer,
        "edges": Integer,
        "time": Integer,
        "edge_types": [ String ],
        "vertex_types": [ String ]
      },
      "id": Integer,
      "millis": Float
    }
```

## get_algorithms

Returns a list of algorithms currently running and connected to the STINGER server.

### Input

This method takes no parameters.

```
    {
      "jsonrpc": "2.0",
      "method": "get_algorithms",
      "id": Integer
    }
```

## Output

* algorithms: Array of algorithm string identifiers

```
    {
      "jsonrpc": "2.0",
      "result": {
        "algorithms": [ String ]
      },
      "id": Integer,
      "millis": Float
    }
```


## get_data_descriptiona

This method retrieves the data fields published by an algorithm.

### Input

* name: String identifier of an algorithm

```
    {
      "jsonrpc": "2.0",
      "method": "get_data_description",
      "params": {
        "name": String
      },
      "id": Integer
    }
```

### Output

* alg_data: Array of data field string identifiers

```
    {
      "jsonrpc": "2.0",
      "result": {
        "alg_data": [ String ]
      },
      "id": Integer,
      "millis": Float
    }
```


## get_data_array

Returns the entire data array for one algorithm field.

### Input

* name: Algorithm string identifier
* data: Data field string identifier
* strings: If True, return vertex identifier strings
* stride: Return every other _stride_ values
* samples: Return _samples_ number of results selected at even intervals across the data array
* log: Make the sampling spacing logarithmic

```
    {
      "jsonrpc": "2.0",
      "method": "get_data_array",
      "params": {
        "name": String,
        "data": String,
        "strings": Boolean,                    /* Optional */
        "stride": Integer,                     /* Optional */
        "samples": Integer,                    /* Optional */
        "log": Boolean                         /* Optional */
      },
      "id": 3
    }
```

### Output

* time: Maximum timestamp in STINGER

Result will be stored in an object named according to the data field name in the input.

* offset: Offset into the sorted array
* count: Number of results to return
* vertex_id: Array of vertex IDs
* vertex_str: Array of vertex string identifiers
* value: Array of data results

```
    {
      "jsonrpc": "2.0",
      "result": {
        "time": Integer,
        "pagerank": {
          "offset": Integer,
          "count": Integer,
          "vertex_id": [ Integer ]
          "vertex_str": [ String ]
          "value": [ Number ]
        }
      },
      "id": Integer,
      "millis": Integer
    }
```


## get_data_array_range

Returns a range of data values based on an offset and count.

### Input

* name: Algorithm string identifier
* data: Data field string identifier
* offset: Offset into the sorted array
* count: Number of results to return
* strings: If True, return vertex identifier strings
* stride: Return every other _stride_ values
* samples: Return _samples_ number of results selected at even intervals across the data array
* log: Make the sampling spacing logarithmic

```
    {
      "jsonrpc": "2.0",
      "method": "get_data_array_range",
      "params": {
        "name": String,
        "data": String,
        "offset": Integer,
        "count": Integer,
        "strings": Boolean,                    /* Optional */
        "stride": Integer,                     /* Optional */
        "samples": Integer,                    /* Optional */
        "log": Boolean                         /* Optional */
      },
      "id": Integer
    }
```

### Output

* time: Maximum timestamp in STINGER

Result will be stored in an object named according to the data field name in the input.

* offset: Offset into the sorted array
* count: Number of results to return
* vertex_id: Array of vertex IDs
* vertex_str: Array of vertex string identifiers
* value: Array of data results

```
    {
      "jsonrpc": "2.0",
      "result": {
        "time": Integer,
        "pagerank": {
          "offset": Integer,
          "count": Integer,
          "vertex_id": [ Integer ]
          "vertex_str": [ String ]
          "value": [ Number ]
        }
      },
      "id": Integer,
      "millis": Float
    }
```


## get_data_array_sorted_range

Returns a range of data values based on an offset and count in sorted order.

### Input

* name: Algorithm string identifier
* data: Data field string identifier
* offset: Offset into the sorted array
* count: Number of results to return
* order: Sorting order requested (ASC or DESC)
* strings: If True, return vertex identifier strings
* stride: Return every other _stride_ values
* samples: Return _samples_ number of results selected at even intervals across the data array
* log: Make the sampling spacing logarithmic

```
    {
      "jsonrpc": "2.0",
      "method": "get_data_array_sorted_range",
      "params": {
        "name": String,
        "data": String,
        "offset": Integer,
        "count": Integer,
        "order": "ASC/DESC",                   /* OPTIONAL - Default: DESC */
        "strings": Boolean,                    /* OPTIONAL */
        "stride": Integer,                     /* OPTIONAL */
        "samples": Integer,                    /* OPTIONAL */
        "log": Boolean                         /* OPTIONAL */
      },
      "id": Number
    }
```

### Output

* time: Maximum timestamp in STINGER

Result will be stored in an object named according to the data field name in the input.

* offset: Offset into the sorted array
* count: Number of results to return
* order: Sorting order requested (ASC or DESC)
* vertex_id: Array of vertex IDs
* vertex_str: Array of vertex string identifiers
* value: Array of data results

```
    {
      "jsonrpc": "2.0",
      "result": {
        "time": Integer,
        "pagerank": {
          "offset": Integer,
          "count": Integer,
          "order": String,
          "vertex_id": [ Integer ]
          "vertex_str": [ String ]
          "value": [ Number ]
        }
      },
      "id": Integer,
      "millis": Float
    }
```


## get_data_array_set

Returns data values for specific vertices.

### Input

* name: Algorithm string identifier
* data: Data field string identifier
* set: Array of vertex IDs or string identifiers
* strings: If True, return vertex identifier strings

```
    {
      "jsonrpc": "2.0",
      "method": "get_data_array_set",
      "params": {
        "name": String,
        "data": String,
        "set": [ Integer/String ],
        "strings": Boolean                     /* OPTIONAL */
      },
      "id": Number
    }
```

### Output

* time: Maximum timestamp in STINGER

Result will be stored in an object named according to the data field name in the input.

* vertex_id: Array of vertex IDs
* vertex_str: Array of vertex string identifiers
* value: Array of data results

```
    {
      "jsonrpc": "2.0",
      "result": {
        "time": Number,
        "pagerank": {
          "vertex_id": [ Integer ]
          "vertex_str": [ String ]
          "value": [ Number ]
        }
      },
      "id": Number,
      "millis": Float
    }
```


## get_data_array_reduction

Perform a reduction over the entire data array and return a single value.

NOTE:  Currently only "sum" is valid.

### Input

* name: Algorithm string identifier
* data: Data field string identifier
* op: Reduction operation

```
    {
      "jsonrpc": "2.0",
      "method": "get_data_array_reduction",
      "params": {
        "name": String,
        "data": String,
        "op": String
      },
      "id": Number
    }
```

### Output

* time: Maximum timestamp in STINGER

Result will be stored in an object named according to the algorithm name in the input.

* field: Data field name that was reduced
* value: Reduced value

```
    {
      "jsonrpc": "2.0",
      "result": {
        "time": Integer,
        String: [
          {
            "field": String,
            "value": Number 
          }
        ]
      },
      "id": Integer,
      "millis": Integer
    }
```


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

* subgraph: Array of edges

```
    {
      "jsonrpc": "2.0",
      "result": {
        "subgraph": []
      },
      "id": Integer,
      "millis": Float
    }
```


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

* source: Vertex ID
* source_str: Vertex string identifier
* vertex_id: Array of vertex IDs
* vertex_str: Array of vertex string identifiers
* value: Array of Adamic-Adar scores

```
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
```

## label_breadth_first_search


## label_mod_expand


Session-based Methods
=====================

## register

### Input

```
    {
      "jsonrpc": "2.0",
      "method": "register",
      "params": {
        "type": "",
        "strings": Boolean                     /* OPTIONAL */
      },
      "id": Integer
    }
```


### Output


## request

### Input

```
    {
      "jsonrpc": "2.0",
      "method": "request",
      "params": {
        "session_id": Integer,
        "strings": Boolean                     /* OPTIONAL */
      },
      "id": Integer
    }
```

### Output


