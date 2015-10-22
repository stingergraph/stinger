import requests
import argparse
import random
import json
import time
from time import gmtime, strftime

TEST_URL = 'http://localhost:82/insert'

parser = argparse.ArgumentParser(description="Test script for STINGER Flask Relay Server. Generates and inserts a batch of batch_size edges every batch_rate seconds.")
parser.add_argument('-n','--batch_size', help="Number of edges to send per batch", default=1000, type=int)
parser.add_argument('-r','--batch_rate', help="Seconds between sending a batch", default=4, type=float)
parser.add_argument('-v','--num_vertices', help="Number of vertices to choose from", default=1000, type=int)
parser.add_argument('-e','--edge_types', help="Number of different edge types", default=2, type=int)
args = parser.parse_args()

while True:
    edges = []
    src = [int(args.num_vertices*random.random()) for i in xrange(args.batch_size)]
    dst = [int(args.num_vertices*random.random()) for i in xrange(args.batch_size)]
    typ = [int(args.edge_types*random.random()) for i in xrange(args.batch_size)]
    exec_time = time.time()
    for i in range(0,args.batch_size):
        e = {'src':str(src[i]),'dest':str(dst[i]),'type':str(typ[i]),'time':int(time.strftime("%Y%m%d%H%M%S", gmtime()))}
        edges.append(e)

    payload = {'edges':edges}
    headers = {'Content-type': 'application/json', 'Accept': 'text/plain'}
    try:
        r = requests.post(TEST_URL, data=json.dumps(payload), headers=headers)
        print r.text
    except:
        print "Unable to reach", TEST_URL, "Sleeping for 5 seconds"
        time.sleep(5)
    while time.time() < exec_time + args.batch_rate:
        time.sleep(0.05)
