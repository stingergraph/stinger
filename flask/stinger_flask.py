import sys
import os
import time
import requests
import json
import threading
import signal
import logging
import argparse
import traceback
from flask import Flask, request, jsonify, Response
from flask.ext.cors import CORS
from flask.ext.restplus import Api, Resource, fields, apidoc
from time import gmtime, strftime


log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)
logging.basicConfig(stream=sys.stderr)

sys.path.append("/home/user/stinger/src/py/")
os.environ['STINGER_LIB_PATH'] = "/home/user/stinger/build/lib/"

import stinger.stinger_net as sn
import stinger.stinger_core as sc

# Value of -1 in either field disables its use as a threshold
TIMEOUT_SECS = 5
BATCH_THRESHOLD = 500

app = Flask(__name__)
api = Api(app, version='1.0', title='STINGER API',
    description='An API to interface with the STINGER graph database',
)
cors = CORS(app)

@api.route('/insert',endpoint='insert')
class Insert(Resource):
    edge = api.model('Edge', {
        'src': fields.String(required=True, description='Source vertex'),
        'dest': fields.String(required=True, description='Destination vertex'),
        'type': fields.String(required=False, description='Edge type'),
        'time': fields.Integer(required=False, description='Timestamp')
    })
    edgesSpec = api.model('Insert', {
        'edges': fields.List(fields.Nested(edge), description='List of edges', required=True),
        'immediate': fields.Boolean(description='Instructs the API to send this batch to STINGER immediately upon receipt', required=False, default=False),
        'strings': fields.Boolean(description='Instructs the API to interpret integer vertex names as strings rather than integer vertex IDs', required=False, default=True)
    })
    @api.expect(edgesSpec)
    @api.doc(responses={
        201: 'Edge Inserted',
        400: 'Bad Request',
        503: 'Unable to reach STINGER'
    })
    def post(self):
        global q

        exec_time = time.time()
        if not request.json:
            r = json.dumps({"error": "Invalid input"})
            return Response(response=r,status=400,mimetype="application/json")

        # grab the lock
        counter_lock.acquire()

        try:
            data = request.json
            send_immediate = False if 'immediate' not in data else data['immediate']
            only_strings = True if 'strings' not in data else data['strings']

            if isinstance(data["edges"], list):
                edges = data["edges"]
                print "Received batch of size", len(edges), 'at', strftime("%Y%m%d%H%M%S", gmtime()),""
                for x in edges:
                    try:
                        if only_strings:
                            source = str(x["src"])
                            destination = str(x["dest"])
                        else:
                            source = x["src"]
                            destination = x["dest"]
                        edge_type = x["type"] if 'type' in x else 0
                        timestamp = int(x["time"]) if 'time' in x else 0
                        s.add_insert(source, destination, edge_type, ts=timestamp, insert_strings=only_strings)
                        print "added edge", source, destination, edge_type, timestamp
                    except Exception as e:
                        print(traceback.format_exc())
                        pass

                # send immediately if the batch is now large
                current_batch_size = s.insertions_count + s.deletions_count + s.vertex_updates_count
                if current_batch_size > BATCH_THRESHOLD and BATCH_THRESHOLD > 0 or send_immediate:
                    s.send_batch()
                    print "Sending  batch of size", current_batch_size, 'at', strftime("%Y%m%d%H%M%S", gmtime()),""

        except:
            print(traceback.format_exc())
            r = json.dumps({"error": "Unable to parse object"})
            return Response(response=r,status=400,mimetype="application/json")

        # end critical section
        finally:
            counter_lock.release()
        exec_time = time.time() - exec_time
        r = json.dumps({"status": "success", "time": exec_time, "current_batch_size": current_batch_size})
        return Response(response=r,status=201,mimetype="application/json")

@api.route('/vertex',endpoint='vertex')
class VertexUpdate(Resource):
    vertexUpdate = api.model('VertexUpdate', {
        'vertex': fields.String(required=True, description='Vertex Name'),
        'vtype': fields.String(required=True, description='Vertex Type Name (None = 0)'),
        'set_weight': fields.String(required=False, description='Edge type'),
        'incr_weight': fields.Integer(required=False, description='Timestamp')
    })
    vertexUpdateSpec = api.model('Update', {
        'vertexUpdates': fields.List(fields.Nested(vertexUpdate), description='List of Vertex Updates', required=True),
        'immediate': fields.Boolean(description='Instructs the API to send this batch to STINGER immediately upon receipt', required=False, default=False)
    })
    @api.expect(vertexUpdateSpec)    
    @api.doc(responses={
        201: 'Vertices Updated',
        400: 'Bad Request',
        503: 'Unable to reach STINGER'
    })
    def post(self):
        global q

        exec_time = time.time()
        if not request.json:
            r = json.dumps({"error": "Invalid input"})
            return Response(response=r,status=400,mimetype="application/json")

        # grab the lock
        counter_lock.acquire()

        try:
            data = request.json
            send_immediate = False
            if 'immediate' in data:
                if data['immediate'] == True:
                    send_immediate = True
            if isinstance(data["vertexUpdates"], list):
                vertexUpdates = data["vertexUpdates"]
                print "Received batch of size", len(vertexUpdates), 'at', strftime("%Y%m%d%H%M%S", gmtime()),""
                for x in vertexUpdates:
                    try:
                        vtx = str(x["vertex"])
                        vtype = str(x["vtype"])
                        set_weight = x["set_weight"] if 'set_weight' in x else 0
                        incr_weight = int(x["incr_weight"]) if 'incr_weight' in x else 0
                        s.add_vertex_update(vtx, vtype, set_weight, incr_weight)
                    except Exception as e:
                        print(traceback.format_exc())
                        pass

                # send immediately if the batch is now large
                current_batch_size = s.insertions_count + s.deletions_count + s.vertex_updates_count
                if current_batch_size > BATCH_THRESHOLD and BATCH_THRESHOLD > 0 or send_immediate:
                    print "Sending  batch of size", current_batch_size, 'at', strftime("%Y%m%d%H%M%S", gmtime()),""
                    s.send_batch()

        except:
            print(traceback.format_exc())
            r = json.dumps({"error": "Unable to parse object"})
            return Response(response=r,status=400,mimetype="application/json")

        # end critical section
        finally:
            counter_lock.release()
        exec_time = time.time() - exec_time
        r = json.dumps({"status": "success", "time": exec_time, "current_batch_size": current_batch_size})
        return Response(response=r,status=201,mimetype="application/json")


@api.route('/jsonrpc')
class StingerProxy(Resource):
    jsonSpec = api.model('rpc', {
        'jsonrpc': fields.String(description='JSON-RPC Version', required=True, default="2.0"),
        'method': fields.String(description='Method for RPC server to execute', required=True),
        'params': fields.Raw(description='JSON object describing parameters for the requested method', required=False),
        'id': fields.Arbitrary(description='User-specified ID of the request', required=True, default=False)
    })
    @api.expect(jsonSpec)
    def post(self):
        return stingerRPC(request.json)

@api.route('/algorithm/<string:algorithm>')
class Algorithms(Resource):
    @api.doc(responses={
        200: 'Success',
        #400: 'Invalid algorithm',
        503: 'Unable to reach STINGER'
    })
    def get(self,algorithm):
        return stingerRPC({"jsonrpc": "2.0", "method": "get_data_description", "params": {"name": algorithm}, "id": 1})

@api.route('/algorithms')
class Algorithm(Resource):
    @api.doc(responses={
        200: 'Success',
        503: 'Unable to reach STINGER'
    })
    def get(self):
        return stingerRPC({"jsonrpc": "2.0", "method": "get_algorithms", "id": 1})

@api.route('/stat/<string:stat>', methods=['GET','POST'])
class Stat(Resource):
    @api.doc(responses={
        200: 'Success',
        503: 'Unable to reach STINGER'
    })
    def get(self,stat):
        stat_data = "bc" if stat == "betweenness_centrality" else stat
        payload = {"jsonrpc": "2.0", "method": "get_data_array_sorted_range", "params": {"name": stat, "strings": True, "data": stat_data, "offset": 0, "count": 30, "order":"DESC"}, "id": 1}
        return stingerRPC(payload)

@api.route('/health', methods=['GET'])
class Health(Resource):
    @api.doc(responses={
        200: 'Success',
        503: 'Unable to reach STINGER'
    })
    def get(self):
        return stingerRPC({"jsonrpc": "2.0", "method": "get_server_health", "id": 1})


def stingerRPC(payload):
    try:
        urlstr = 'http://{}:{}/jsonrpc'.format(STINGER_HOST,STINGER_RPC_PORT)
        r = requests.post(urlstr, data=json.dumps(payload))
    except:
        print(traceback.format_exc())
        return Response(response=json.dumps({"error": "JSON-RPC down"}),status=503,mimetype="application/json")
    return Response(response=r.text,status=200,mimetype="application/json")


#
# Initialize the connection to STINGER server
#
def connect(undirected=False,strings=True):
    global s
    s = sn.StingerStream(STINGER_HOST, 10102, strings, undirected)
    if s.sock_handle == -1:
        raise Exception("Failed to connect to STINGER")
    directedness = 'UNdirected' if undirected else 'directed'
    print "Edges will be inserted as",directedness

    global counter_lock
    counter_lock = threading.Lock()

#
# attempt to send a batch every TIMEOUT_SECS seconds
#
def sendBatch():
    current_batch_size = s.insertions_count + s.deletions_count + s.vertex_updates_count
    if current_batch_size > 0:
        counter_lock.acquire()
        try:
            s.send_batch()
            print "Sending  batch of size", current_batch_size, 'at', strftime("%Y%m%d%H%M%S", gmtime()),""
        finally:
            counter_lock.release()

    if TIMEOUT_SECS > -1:
        global t
        t = threading.Timer(TIMEOUT_SECS, sendBatch)
        t.start()

def signal_handler(signal, frame):
    try:
        t.cancel()
    except:
        pass
    sys.exit(0)

def setupSTINGERConnection():
    if not 's' in globals():
        try:
            connect(args.undirected)
            print 'STINGER connection successful'
        except Exception as e:
            print str(e)
            print 'STINGER connection unsuccessful'
    if not 't' in globals():
        try:
            sendBatch()
            print 'STINGER timer setup successful'
        except Exception as e:
            print str(e)
            print 'STINGER timer setup unsuccessful'

#
# main
#
if __name__ == '__main__':
    global STINGER_HOST
    global STINGER_RPC_PORT
    signal.signal(signal.SIGINT, signal_handler)
    parser = argparse.ArgumentParser(description="STINGER Flask Relay Server")
    parser.add_argument('--undirected', action="store_true")
    parser.add_argument('--flask_host', default="0.0.0.0")
    parser.add_argument('--flask_port', default=5000, type=int)
    parser.add_argument('--stinger_host', default="localhost")
    parser.add_argument('--stinger_rpc_port', default=8088, type=int)
    args = parser.parse_args()
    STINGER_HOST = args.stinger_host
    STINGER_RPC_PORT = args.stinger_rpc_port
    setupSTINGERConnection()
    app.run(debug=False,host=args.flask_host,port=args.flask_port)
