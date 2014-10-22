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

# get_server_info

## Input

    {
      "jsonrpc": "2.0",
      "method": "get_server_info",
      "id": 13
    }

## Output

    {
      "jsonrpc": "2.0",
      "result": {
	"pid": 2776392484925547500
      },
      "id": 13,
      "millis": 0.21
    }

# get_graph_stats

## Input

    {
	"jsonrpc": "2.0",
	"method": "get_graph_stats",
	"params" : {
	    "get_types": Boolean	/* OPTIONAL */
	},
	"id": 7
    }

## Output

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


# get_algorithms

## Input

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

# get_data_description

## Input

    {
	"jsonrpc": "2.0",
	"method": "get_data_description",
	"params": {
	    "name": "pagerank"
	},
	"id": 2
    }

## Output

    {
	"jsonrpc": "2.0",
	"result": {
	    "alg_data": [
		"pagerank"
	    ]
	},
	"id": 2,
	"millis": 0.71
    }

# get_data_array

## Input

    {
	"jsonrpc": "2.0",
	"method": "get_data_array",
	"params": {
	    "name": "algorithm_name",
	    "data": "data_array_name",
	    "strings": Boolean,		/* Optional */
	    "stride": Integer,		/* Optional */
	    "samples": Integer,		/* Optional */
	    "log": Boolean		/* Optional */
	},
	"id": 3
    }

## Output

    {
	"jsonrpc": "2.0",
	"result": {
	    "time": 100,
	    "pagerank": {
		"offset": 0,
		"count": 178,
		"vertex_id": []
		"vertex_str": []
		"value": []
	    }
	},
	"id": 5,
	"millis": 0.3
    }


# get_data_array_range

# Input

    {
	"jsonrpc": "2.0",
	"method": "get_data_array_range",
	"params": {
	    "name": "algorithm_name",
	    "data": "data_array_name",
	    "offset": Integer,
	    "count": Integer,
	    "strings": Boolean,		/* Optional */
	    "stride": Integer,		/* Optional */
	    "samples": Integer,		/* Optional */
	    "log": Boolean		/* Optional */
	},
	"id": 4
    }

# Output

    {
	"jsonrpc": "2.0",
	"result": {
	    "time": 100,
	    "pagerank": {
		"offset": 0,
		"count": 178,
		"vertex_id": []
		"vertex_str": []
		"value": []
	    }
	},
	"id": 5,
	"millis": 0.3
    }


# get_data_array_sorted_range

## Input

    {
	"jsonrpc": "2.0",
	"method": "get_data_array_sorted_range",
	"params": {
	    "name": "algorithm_name",
	    "data": "data_array_name",
	    "offset": Integer,
	    "count": Integer,
	    "order": "ASC/DESC",	/* OPTIONAL - Default: DESC */
	    "strings": Boolean,		/* OPTIONAL */
	    "stride": Integer,		/* OPTIONAL */
	    "samples": Integer,		/* OPTIONAL */
	    "log": Boolean		/* OPTIONAL */
	},
	"id": 5
    }

## Output

    {
	"jsonrpc": "2.0",
	"result": {
	    "time": 100,
	    "pagerank": {
		"offset": 0,
		"count": 178,
		"order": "DESC",
		"vertex_id": []
		"vertex_str": []
		"value": []
	    }
	},
	"id": 5,
	"millis": 0.3
    }


# get_data_array_set

## Input

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
	    "strings": Boolean		/* OPTIONAL */
	},
	"id": 6
    }

## Output

    {
	"jsonrpc": "2.0",
	"result": {
	    "time": 100,
	    "pagerank": {
		"vertex_id": []
		"vertex_str": []
		"value": []
	    }
	},
	"id": 5,
	"millis": 0.3
    }

# get_data_array_reduction

## Input

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

## Output

    {
	"jsonrpc": "2.0",
	"result": {
	    "time": 100,
	    "pagerank": [
		{
		    "field": "pagerank",
		    "value": 1.999858697906
		}
	    ]
	},
	"id": 12,
	"millis": 16.71
    }

# breadth_first_search

## Input

    {
	"jsonrpc": "2.0",
	"method": "breadth_first_search",
	"params": {
	    "source": 27,
	    "target": 151,
	    "strings": Boolean,		    /* OPTIONAL */
	    "get_types": Boolean,	    /* OPTIONAL */
	    "get_etypes": Boolean,	    /* OPTIONAL */
	    "get_vtypes": Boolean	    /* OPTIONAL */
	},
	"id": 8
    }

## Output

    {
	"jsonrpc": "2.0",
	"result": {
	    "subgraph": []
	},
	"id": 11,
	"millis": 5.34
    }


# register

## Input

    {
	"jsonrpc": "2.0",
	"method": "register",
	"params": {
	    "type": "",
	    "strings": Boolean		    /* OPTIONAL */
	},
	"id": 9
    }

## Output


# request

## Input

    {
	"jsonrpc": "2.0",
	"method": "request",
	"params": {
	    "session_id": Integer,
	    "strings": Boolean		    /* OPTIONAL */
	},
	"id": 10
    }

## Output


# adamic_adar_index

## Input

    {
	"jsonrpc": "2.0",
	"method": "adamic_adar_index",
	"params": {
	    "source": 1,
	    "strings": Boolean,		    /* OPTIONAL */
	    "include_neighbors": Boolean    /* OPTIONAL */
	},
	"id": 14
    }

## Output

    {
	"jsonrpc": "2.0",
	"result": {
	    "source": 1,
	    "source_str": "864",
	    "vertex_id": [
		0
	    ],
	    "vertex_str": [
		"987"
	    ],
	    "value": [
		0
	    ]
	},
	"id": 14,
	"millis": 13.85
    }

# label_breadth_first_search

# label_mod_expand

