import requests
import argparse
import numpy
import json
import time
from time import gmtime, strftime

# test_algorithms has two modes of operation:
#   1. Create reference output of algorithms
#   2. Validate current algorithm results against reference output

parser = argparse.ArgumentParser(description="Tests the output of algorithms of one STINGER against another")
group = parser.add_mutually_exclusive_group()
group.add_argument('-c', '--create',        help="Create reference algorithm output", action="store_true", default=False)
group.add_argument('-t', '--test',          help="Test reference ", action="store_true", default=False)
parser.add_argument('-s', '--skip_insert',  help="Skip inserting edges, just get and store the results of all algorithms", action="store_true", default=False)
parser.add_argument('-o', '--output_file',  help="Output file to store reference run", default='results.pickle')
parser.add_argument('-i', '--input_file',   help="Input file that holds the reference run", default='results.pickle')
parser.add_argument('-f','--flask_url',     help="URL to STINGER Flask API", default="http://localhost:5000/")
parser.add_argument('-sh','--stinger_host', help="STINGER host", default="localhost")
parser.add_argument('-sp','--stinger_port', help="STINGER RPC Server port", default=8088, type=int)
args = parser.parse_args()

def stingerRPC(payload):
    urlstr = 'http://{}:{}/jsonrpc'.format(args.stinger_host,args.stinger_port)
    r = requests.post(urlstr, data=json.dumps(payload))
    return r.json()

def get_algorithms():
    payload = {"jsonrpc": "2.0", "method": "get_algorithms", "id": 1}
    r = stingerRPC(payload)
    assert 'result' in r
    algs = r['result']['algorithms']
    algs.remove('stinger')
    return algs

def get_results():
    algorithms = get_algorithms()
    alg_data = {}
    alg_data['betweenness_centrality'] = ['bc']
    alg_data['clustering_coeff'] = ['coeff','ntriangles']
    alg_data['kcore'] = ['kcore']
    alg_data['pagerank'] = ['pagerank']
    alg_data['simple_communities'] = ['community_label']
    alg_data['static_components'] = ['component_label']
    alg_data['weakly_connected_components'] = ['component_label','component_size']

    results = {}
    for alg in algorithms:
        print alg
        req_data = alg_data[alg]
        for req in req_data:
            if alg in ['betweenness_centrality','pagerank','kcore']:
                payload = {
                    "jsonrpc": "2.0",
                    "method": "get_data_array_sorted_range",
                    "params": {
                        "name": alg,
                        "strings": True,
                        "data": req,
                        "offset": 0,
                        "count": 500,
                        "order":"DESC"
                    }, "id": 0}
            else:
                payload = {
                    "id": 0,
                    "jsonrpc": "2.0",
                    "method": "get_data_array_set",
                    "params": {
                        "name": alg,
                        "data": req,
                        "strings":True,
                        "set": range(0,34),
                        "order":"DESC"
                    }
                }

            rpc = stingerRPC(payload)
            data = rpc['result'][req]
            alg_store = alg if len(req_data) == 1 else alg + '_' + req
            results[alg_store] = dict(zip(data['vertex_str'],data['value']))

    print json.dumps(results)


def write_results(results,output_file):
    pass

def create_graph():
    endpoint = args.flask_url + 'insert'
    hd = {'content-type': 'application/json'}
    edges = {'edges': []}

    edges['edges'].append({'src': 1, 'dest': 2})
    edges['edges'].append({'src': 1, 'dest': 3})
    edges['edges'].append({'src': 1, 'dest': 4})
    edges['edges'].append({'src': 1, 'dest': 5})
    edges['edges'].append({'src': 1, 'dest': 6})
    edges['edges'].append({'src': 1, 'dest': 7})
    edges['edges'].append({'src': 1, 'dest': 8})
    edges['edges'].append({'src': 1, 'dest': 9})
    edges['edges'].append({'src': 1, 'dest': 11})
    edges['edges'].append({'src': 1, 'dest': 12})
    edges['edges'].append({'src': 1, 'dest': 13})
    edges['edges'].append({'src': 1, 'dest': 14})
    edges['edges'].append({'src': 1, 'dest': 18})
    edges['edges'].append({'src': 1, 'dest': 20})
    edges['edges'].append({'src': 1, 'dest': 22})
    edges['edges'].append({'src': 1, 'dest': 32})
    edges['edges'].append({'src': 2, 'dest': 1})
    edges['edges'].append({'src': 2, 'dest': 3})
    edges['edges'].append({'src': 2, 'dest': 4})
    edges['edges'].append({'src': 2, 'dest': 8})
    edges['edges'].append({'src': 2, 'dest': 14})
    edges['edges'].append({'src': 2, 'dest': 18})
    edges['edges'].append({'src': 2, 'dest': 20})
    edges['edges'].append({'src': 2, 'dest': 22})
    edges['edges'].append({'src': 2, 'dest': 31})
    edges['edges'].append({'src': 3, 'dest': 1})
    edges['edges'].append({'src': 3, 'dest': 2})
    edges['edges'].append({'src': 3, 'dest': 8})
    edges['edges'].append({'src': 3, 'dest': 28})
    edges['edges'].append({'src': 3, 'dest': 29})
    edges['edges'].append({'src': 3, 'dest': 33})
    edges['edges'].append({'src': 3, 'dest': 9})
    edges['edges'].append({'src': 3, 'dest': 10})
    edges['edges'].append({'src': 3, 'dest': 14})
    edges['edges'].append({'src': 3, 'dest': 4})
    edges['edges'].append({'src': 4, 'dest': 13})
    edges['edges'].append({'src': 4, 'dest': 1})
    edges['edges'].append({'src': 4, 'dest': 2})
    edges['edges'].append({'src': 4, 'dest': 8})
    edges['edges'].append({'src': 4, 'dest': 3})
    edges['edges'].append({'src': 4, 'dest': 14})
    edges['edges'].append({'src': 5, 'dest': 1})
    edges['edges'].append({'src': 5, 'dest': 11})
    edges['edges'].append({'src': 5, 'dest': 7})
    edges['edges'].append({'src': 6, 'dest': 1})
    edges['edges'].append({'src': 6, 'dest': 7})
    edges['edges'].append({'src': 6, 'dest': 11})
    edges['edges'].append({'src': 6, 'dest': 17})
    edges['edges'].append({'src': 7, 'dest': 1})
    edges['edges'].append({'src': 7, 'dest': 5})
    edges['edges'].append({'src': 7, 'dest': 6})
    edges['edges'].append({'src': 7, 'dest': 17})
    edges['edges'].append({'src': 8, 'dest': 1})
    edges['edges'].append({'src': 8, 'dest': 2})
    edges['edges'].append({'src': 8, 'dest': 3})
    edges['edges'].append({'src': 8, 'dest': 4})
    edges['edges'].append({'src': 9, 'dest': 1})
    edges['edges'].append({'src': 9, 'dest': 3})
    edges['edges'].append({'src': 9, 'dest': 33})
    edges['edges'].append({'src': 9, 'dest': 34})
    edges['edges'].append({'src': 9, 'dest': 31})
    edges['edges'].append({'src': 10, 'dest': 3})
    edges['edges'].append({'src': 10, 'dest': 34})
    edges['edges'].append({'src': 11, 'dest': 1})
    edges['edges'].append({'src': 11, 'dest': 5})
    edges['edges'].append({'src': 11, 'dest': 6})
    edges['edges'].append({'src': 12, 'dest': 1})
    edges['edges'].append({'src': 13, 'dest': 1})
    edges['edges'].append({'src': 13, 'dest': 4})
    edges['edges'].append({'src': 14, 'dest': 1})
    edges['edges'].append({'src': 14, 'dest': 4})
    edges['edges'].append({'src': 14, 'dest': 3})
    edges['edges'].append({'src': 14, 'dest': 2})
    edges['edges'].append({'src': 14, 'dest': 34})
    edges['edges'].append({'src': 15, 'dest': 34})
    edges['edges'].append({'src': 15, 'dest': 33})
    edges['edges'].append({'src': 16, 'dest': 34})
    edges['edges'].append({'src': 16, 'dest': 33})
    edges['edges'].append({'src': 17, 'dest': 6})
    edges['edges'].append({'src': 17, 'dest': 7})
    edges['edges'].append({'src': 18, 'dest': 1})
    edges['edges'].append({'src': 18, 'dest': 2})
    edges['edges'].append({'src': 19, 'dest': 34})
    edges['edges'].append({'src': 19, 'dest': 33})
    edges['edges'].append({'src': 20, 'dest': 1})
    edges['edges'].append({'src': 20, 'dest': 2})
    edges['edges'].append({'src': 20, 'dest': 34})
    edges['edges'].append({'src': 21, 'dest': 34})
    edges['edges'].append({'src': 21, 'dest': 33})
    edges['edges'].append({'src': 22, 'dest': 1})
    edges['edges'].append({'src': 22, 'dest': 2})
    edges['edges'].append({'src': 23, 'dest': 34})
    edges['edges'].append({'src': 23, 'dest': 33})
    edges['edges'].append({'src': 24, 'dest': 34})
    edges['edges'].append({'src': 24, 'dest': 33})
    edges['edges'].append({'src': 24, 'dest': 28})
    edges['edges'].append({'src': 24, 'dest': 26})
    edges['edges'].append({'src': 24, 'dest': 30})
    edges['edges'].append({'src': 25, 'dest': 26})
    edges['edges'].append({'src': 25, 'dest': 28})
    edges['edges'].append({'src': 25, 'dest': 32})
    edges['edges'].append({'src': 26, 'dest': 32})
    edges['edges'].append({'src': 26, 'dest': 25})
    edges['edges'].append({'src': 26, 'dest': 24})
    edges['edges'].append({'src': 27, 'dest': 30})
    edges['edges'].append({'src': 27, 'dest': 34})
    edges['edges'].append({'src': 28, 'dest': 34})
    edges['edges'].append({'src': 28, 'dest': 25})
    edges['edges'].append({'src': 28, 'dest': 24})
    edges['edges'].append({'src': 28, 'dest': 3})
    edges['edges'].append({'src': 29, 'dest': 3})
    edges['edges'].append({'src': 29, 'dest': 32})
    edges['edges'].append({'src': 29, 'dest': 34})
    edges['edges'].append({'src': 30, 'dest': 33})
    edges['edges'].append({'src': 30, 'dest': 34})
    edges['edges'].append({'src': 30, 'dest': 24})
    edges['edges'].append({'src': 30, 'dest': 27})
    edges['edges'].append({'src': 31, 'dest': 34})
    edges['edges'].append({'src': 31, 'dest': 33})
    edges['edges'].append({'src': 31, 'dest': 9})
    edges['edges'].append({'src': 31, 'dest': 2})
    edges['edges'].append({'src': 32, 'dest': 1})
    edges['edges'].append({'src': 32, 'dest': 29})
    edges['edges'].append({'src': 32, 'dest': 34})
    edges['edges'].append({'src': 32, 'dest': 33})
    edges['edges'].append({'src': 32, 'dest': 25})
    edges['edges'].append({'src': 32, 'dest': 26})
    edges['edges'].append({'src': 33, 'dest': 9})
    edges['edges'].append({'src': 33, 'dest': 3})
    edges['edges'].append({'src': 33, 'dest': 32})
    edges['edges'].append({'src': 33, 'dest': 24})
    edges['edges'].append({'src': 33, 'dest': 19})
    edges['edges'].append({'src': 33, 'dest': 21})
    edges['edges'].append({'src': 33, 'dest': 30})
    edges['edges'].append({'src': 33, 'dest': 16})
    edges['edges'].append({'src': 33, 'dest': 15})
    edges['edges'].append({'src': 33, 'dest': 34})
    edges['edges'].append({'src': 33, 'dest': 23})
    edges['edges'].append({'src': 33, 'dest': 31})
    edges['edges'].append({'src': 34, 'dest': 14})
    edges['edges'].append({'src': 34, 'dest': 9})
    edges['edges'].append({'src': 34, 'dest': 29})
    edges['edges'].append({'src': 34, 'dest': 32})
    edges['edges'].append({'src': 34, 'dest': 33})
    edges['edges'].append({'src': 34, 'dest': 28})
    edges['edges'].append({'src': 34, 'dest': 24})
    edges['edges'].append({'src': 34, 'dest': 19})
    edges['edges'].append({'src': 34, 'dest': 21})
    edges['edges'].append({'src': 34, 'dest': 30})
    edges['edges'].append({'src': 34, 'dest': 16})
    edges['edges'].append({'src': 34, 'dest': 27})
    edges['edges'].append({'src': 34, 'dest': 15})
    edges['edges'].append({'src': 34, 'dest': 23})
    edges['edges'].append({'src': 34, 'dest': 10})
    edges['edges'].append({'src': 34, 'dest': 20})
    edges['edges'].append({'src': 34, 'dest': 31})
    edges['immediate'] = True

    payload = json.dumps(edges)
    resp = requests.post(url=endpoint, headers=hd, data=payload)


if args.create:
    if not args.skip_insert:
        create_graph()
    results = get_results()
    write_results(results,args.output_file)
elif args.test:
    test_graphs()
else:
    print "Invalid options. Must specify one of -c or -t"
