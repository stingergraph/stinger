import requests
import argparse
import numpy
import json
import time
from time import gmtime, strftime

TEST_URL = 'http://localhost:5000/insert'

parser = argparse.ArgumentParser(description="Test script for STINGER Flask Relay Server")
parser.add_argument('--batch_size', help="Number of edges to send per batch", default=1000, type=int)
parser.add_argument('--batch_rate', help="Seconds between sending a batch", default=4, type=int)
parser.add_argument('--edge_types', help="Number of different edge types", default=2, type=int)
args = parser.parse_args()

while True:
    edges = []
    src = numpy.random.randint(0,args.batch_size,args.batch_size)
    dst = numpy.random.randint(0,args.batch_size,args.batch_size)
    typ = numpy.random.randint(0,args.edge_types,args.batch_size)
    exec_time = time.time()
    for i in range(0,args.batch_size):
        if dst[i] == src[i]:
            src += 1
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
        time.sleep(1)
