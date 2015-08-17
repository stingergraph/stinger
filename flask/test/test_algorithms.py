import argparse
import cPickle as pickle
import colorama as color
import csv
import json
import numpy
import os
import requests
import sys
import time
from time import gmtime, strftime

# test_algorithms has two modes of operation:
#   1. Create reference output of algorithms
#   2. Validate current algorithm results against reference output

run_dir = os.path.dirname(os.path.realpath(__file__))
color.init()

TOLERENCE = 1e-10   # Acceptable difference per dataset from reference values

parser = argparse.ArgumentParser(description="Tests the output of algorithms of one STINGER against another")
group = parser.add_mutually_exclusive_group()
parser.add_argument('-c', '--create',       help="Write results to file", action="store_true", default=False)
parser.add_argument('-t', '--test',         help="Test reference ", action="store_true", default=False)
parser.add_argument('-s', '--skip_insert',  help="Skip inserting edges, just get and store the results of all algorithms", action="store_true", default=False)
parser.add_argument('-o', '--output_file',  help="Output file to store reference run", default=run_dir + '/results.pickle')
parser.add_argument('-i', '--input_file',   help="Input file that stores the reference run", default=run_dir + '/results.pickle')
parser.add_argument('-f','--flask_url',     help="URL to STINGER Flask API", default="http://localhost:5000/")
parser.add_argument('-sh','--stinger_host', help="STINGER host", default="localhost")
parser.add_argument('-sp','--stinger_port', help="STINGER RPC Server port", default=8088, type=int)
parser.add_argument('-g','--graph_file',    help="CSV file with source,dest pairs defining the reference graph to be inserted", default=run_dir + '/reference_graph.csv')
args = parser.parse_args()

def stingerRPC(payload):
    urlstr = 'http://{}:{}/jsonrpc'.format(args.stinger_host,args.stinger_port)
    try:
        r = requests.post(urlstr, data=json.dumps(payload))
    except:
        print color.Fore.RED + "Cannot connect to STINGER RPC" + color.Style.RESET_ALL
        sys.exit(1)
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
    # print algorithms
    # TODO: Get these data elements from RPC Server
    # Define data elements available for each algorithm.
    alg_data = {}
    alg_data['betweenness_centrality'] = ['bc', 'times_found']
    alg_data['clustering_coeff'] = ['coeff','ntriangles']
    alg_data['kcore'] = ['kcore','count']
    alg_data['pagerank'] = ['pagerank']
    alg_data['simple_communities'] = ['community_label']
    alg_data['static_components'] = ['component_label']
    alg_data['weakly_connected_components'] = ['component_label','component_size']

    results = {}
    for alg in algorithms:
        req_data = alg_data[alg]
        for req in req_data:
            # print alg, req
            payload = {
                "id": 0,
                "jsonrpc": "2.0",
                "method": "get_data_array",
                "params": {
                    "name": alg,
                    "data": req,
                    "strings":True
                }
            }

            alg_store = alg if len(req_data) == 1 else alg + '-' + req
            rpc = stingerRPC(payload)
            if 'result' not in rpc:
                print color.Fore.RED+ alg_store+ ": STINGER RPC returned unexpected results"
                print '\tReq: ' + json.dumps(payload)
                print '\tResp:' + json.dumps(rpc) + color.Style.RESET_ALL
                continue
            data = rpc['result'][req]
            results[alg_store] = dict(zip(data['vertex_str'],data['value']))

    print "Received results of",len(results),"data elements from",len(algorithms),"algorithms"
    #print json.dumps(results)
    return results


def write_results(results,output_file):
    pickle.dump(results, open(output_file,'wb'))
    print "Wrote", len(results), "results to", output_file

def create_graph():
    endpoint = args.flask_url + 'insert'
    hd = {'content-type': 'application/json'}
    edges = {'edges': []}
    with open(args.graph_file, 'rb') as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            edges['edges'].append({'src':str(row[0]),'dest':str(row[1])})

    edges['immediate'] = True

    payload = json.dumps(edges)
    resp = requests.post(url=endpoint, headers=hd, data=payload)
    time.sleep(2)

def test_graphs(reference,current):
    reference_keys = reference.keys()
    current_keys = current.keys()

    if set(reference_keys) != set(current_keys):
        print color.Fore.RED + "Different algorithms in reference set than current set. " + \
              color.Style.RESET_ALL + "Reference set contains these elements:"
        print ", ".join(reference_keys)

    diff = {}
    for element in reference:
        if element not in current:
            print color.Fore.RED + "Reference Dataset '"+ str(element) +"' not in current STINGER" + color.Style.RESET_ALL
            continue
        diff[element] = 0
        dataset = reference[element]
        for vertex in dataset:
            if vertex not in current[element]:
                print "Reference Vertex '" + str(vertex) + "' not in current STINGER"
                continue
            diff[element] += abs(dataset[vertex] - current[element][vertex])

    print "Computed difference between datasets"
    for element in sorted(diff):
        col = color.Fore.GREEN if diff[element] < TOLERENCE else color.Fore.RED
        print col + "\t",element,":",diff[element], color.Style.RESET_ALL
    # print reference.keys()
    # print current.keys()

if not args.skip_insert:
    create_graph()

current_results = get_results()
if args.create:
    write_results(current_results,args.output_file)
elif args.test:
    if not os.path.isfile(args.input_file):
        print color.Fore.RED + "Reference file " + args.input_file + " does not exist." + \
            color.Style.RESET_ALL + " Did you mean to create a reference file?"
        sys.exit(1)
    reference_results = pickle.load(open(args.input_file,'rb'))
    test_graphs(reference_results,current_results)
else:
    print "Invalid options. Must specify one of -c or -t"
