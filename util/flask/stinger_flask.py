import argparse
import commands
import json
import logging
import os
import requests
import signal
import subprocess
import sys
import threading
import time
import traceback
from flask import Flask, request, jsonify, Response, make_response
from flask.ext.cors import CORS
from flask.ext.restplus import Api, Resource, fields, apidoc
from time import gmtime, strftime

# Set logs to go to stderr
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)
logging.basicConfig(stream=sys.stderr)

# Import stinger libraries, setup Flask config parameters
baseDir = os.path.dirname(os.path.realpath(__file__)) + '/../../'
config = {
    'STINGER_BASE':baseDir,
    'STINGER_HOST':'localhost',
    'STINGER_LOGDIR':'/var/log/stinger/',
    'STINGER_RPC_PORT': 8088,
    'LIB_PATH':baseDir + 'build/lib/',
    'PY_PATH':baseDir + 'src/py/',
    'SESSION_NAME':'ManagedSTINGER',
    'STINGERCTL_PATH': baseDir + 'util/stingerctl',
    'UNDIRECTED': False,
    'FLASK_PORT': 82,
    'FLASK_HOST': '0.0.0.0'
}

sys.path.append(config['PY_PATH'])
os.environ['STINGER_LIB_PATH'] = config['LIB_PATH']

import stinger.stinger_net as sn
import stinger.stinger_core as sc

# Value of -1 in either field disables its use as a threshold
TIMEOUT_SECS = 5
BATCH_THRESHOLD = 500

application = Flask(__name__)
api = Api(application, version='1.1', title='STINGER API',
    description='An API to interface with the STINGER graph database',
)
cors = CORS(application)

@api.route('/insert',endpoint='insert')
class Insert(Resource):
    edge = api.model('Edge', {
        'src': fields.String(required=True, description='Source vertex'),
        'dest': fields.String(required=True, description='Destination vertex'),
        'type': fields.String(required=False, description='Edge type'),
        'weight': fields.Integer(required=False, description='Edge weight'),
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
        setupSTINGERConnection()
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
                        edge_weight = int(x["weight"]) if 'weight' in x else 0
                        timestamp = int(x["time"]) if 'time' in x else 0
                        s.add_insert(source, destination, edge_type,
                            weight = edge_weight, ts=timestamp,
                            insert_strings=only_strings)
                        # print "added edge", source, destination, edge_type, timestamp
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
class Algorithm(Resource):
    @api.doc(responses={
        200: 'Success',
        #400: 'Invalid algorithm',
        503: 'Unable to reach STINGER'
    })
    def get(self,algorithm):
        return stingerRPC({"jsonrpc": "2.0", "method": "get_data_description", "params": {"name": algorithm}, "id": 1})

@api.route('/algorithms')
class Algorithms(Resource):
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
        payload = {"jsonrpc": "2.0", "method": "get_data_array_sorted_range", "params": {"name": stat, "strings": True, "data": stat_data, "offset": 0, "count": 500, "order":"DESC"}, "id": 1}
        return stingerRPC(payload)

@api.route('/health', methods=['GET'])
@application.route('/server_status', methods=['GET'])
class Health(Resource):
    @api.doc(responses={
        200: 'Success',
        503: 'Unable to reach STINGER'
    })
    def get(self):
        return stingerRPC({"jsonrpc": "2.0", "method": "get_server_health", "id": 1})


@api.route('/connections', methods=['GET'])
class Connections(Resource):
    @api.doc(responses={
        200: 'Success',
        500: 'Cannot read connections.json'
    })
    def get(self):
        response = make_response(get_connections())
        response.headers['Access-Control-Allow-Origin'] = '*'
        response.headers["content-type"] = "text/plain"
        return response


@api.route('/log', methods=['GET'])
class Log(Resource):
    @api.doc(responses={
        200: 'Success',
        500: 'Cannot read log'
    })
    def get(self):
        response = make_response(get_log())
        response.headers['Access-Control-Allow-Origin'] = '*'
        response.headers["content-type"] = "text/plain"
        return response


@api.route('/status')
class Status(Resource):
    @api.doc(responses={
        200: 'Success',
        500: 'Cannot read process list'
    })
    def get(self):
        response = {}
        processes = ['stinger_server','stinger_json_rpc_server','stinger_pagerank','stinger_betweenness','stinger_kcore']
        for p in processes:
            running = commands.getoutput('ps -A w')
            if p in starting:
                response[p] = "STARTING"
            elif p in running:
                response[p] = "RUNNING"
            else:
                response[p] = "DOWN"
        return response

@api.route('/toggle/<string:program>')
class Toggle(Resource):
    @api.doc(responses={
        200: 'Success',
        500: 'Cannot stop/start requested program'
    })
    def get(self,program):
        success = 0
        status = ''
        processes = commands.getoutput('ps -A')
        if program in processes:
            if(program == 'stinger_server'):
                success =  stingerctl(['stop'])
            if success == 0:
                status = 'NOT RUNNING'
        else:
            if(program == 'stinger_server'):
                success =  stingerctl(['start'])
            else:
                alg = program[len('stinger_'):]
                success = stingerctl(['addalg',str(alg)])
            if success == 0:
                status = 'RUNNING'

        return {'status':status,'success':success == 0}


starting = list()

def reverse_readline(filename, buf_size=8192):
    """a generator that returns the lines of a file in reverse order"""
    with open(filename) as fh:
        segment = None
        offset = 0
        fh.seek(0, os.SEEK_END)
        total_size = remaining_size = fh.tell()
        while remaining_size > 0:
            offset = min(total_size, offset + buf_size)
            fh.seek(-offset, os.SEEK_END)
            buffer = fh.read(min(remaining_size, buf_size))
            remaining_size -= buf_size
            lines = buffer.split('\n')
            # the first line of the buffer is probably not a complete line so
            # we'll save it and applicationend it to the last line of the next buffer
            # we read
            if segment is not None:
                # if the previous chunk starts right from the beginning of line
                # do not concact the segment to the last line of new chunk
                # instead, yield the segment first
                if buffer[-1] is not '\n':
                    lines[-1] += segment
                else:
                    yield segment
            segment = lines[0]
            for index in range(len(lines) - 1, 0, -1):
                yield lines[index]
        yield segment

def get_time_from_log_line(line):
    return

def ingest_history():
    output = []
    # Only get the last 10 minutes
    time_window = 10

def get_log():
    output = ""
    return output
    num_lines = 200
    fileName = "syslog"
    nextLog = 1
    try:
        while 1==1:
            if os.path.exists("/var/log/" + fileName) == False:
                break;
            print "Trying " + "/var/log/" + fileName
            for line in reverse_readline("/var/log/" + fileName):
                if re.search("stinger", line):
                    output += line + '\n'
                    num_lines -= 1
                if num_lines == 0:
                    break
            if num_lines ==0:
                break;
            else:
                if nextLog == 1:
                    fileName = "syslog.1"
                else:
                    fileName = "syslog." + str(nextLog) + ".gz"
                nextLog += 1

        return output

    except:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        traceback.print_exception(exc_type, exc_value, exc_traceback, limit=2, file=sys.stderr)
        traceback.print_tb(exc_traceback, limit=4, file=sys.stderr)

def get_connections():
    thisdir = os.path.dirname(os.path.realpath(__file__))
    with open(thisdir + '/connections.json') as data_file:
        data = data_file.read()
    return data


def stingerctl(command):
    cmd = [config['STINGERCTL_PATH']]
    cmd.extend(command)
    print ' '.join(cmd)
    return subprocess.call( cmd )

def stingerRPC(payload):
    try:
        urlstr = 'http://{0}:{1}/jsonrpc'.format(config['STINGER_HOST'],config['STINGER_RPC_PORT'])
        r = requests.post(urlstr, data=json.dumps(payload))
    except:
        print(traceback.format_exc())
        return Response(response=json.dumps({"error": "JSON-RPC down"}),status=503,mimetype="application/json")
    return Response(response=r.text,status=200,mimetype="application/json")


#
# Initialize the connection to STINGER server
#
def connect(strings=True):
    global s
    global counter_lock
    s = sn.StingerStream(config['STINGER_HOST'], 10102, strings, config['UNDIRECTED'])
    if s.sock_handle == -1:
        raise Exception("Failed to connect to STINGER")
    directedness = 'UNdirected' if config['UNDIRECTED'] else 'directed'
    print "Edges will be inserted as",directedness

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
    if not 's' in globals() or 'counter_lock' not in globals():
        try:
            connect()
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


setupSTINGERConnection()

#
# main
#
if __name__ == '__main__':
    signal.signal(signal.SIGINT, signal_handler)
    parser = argparse.ArgumentParser(description="STINGER Flask Relay Server")
    parser.add_argument('--undirected', action="store_true")
    parser.add_argument('--flask_host', default=config['FLASK_HOST'])
    parser.add_argument('--flask_port', default=config['FLASK_PORT'], type=int)
    parser.add_argument('--stinger_host', default=config['STINGER_HOST'])
    parser.add_argument('--stinger_rpc_port', default=config['STINGER_RPC_PORT'], type=int)
    args = parser.parse_args()
    config['STINGER_HOST'] = args.stinger_host
    config['STINGER_RPC_PORT'] = args.stinger_rpc_port
    config['UNDIRECTED'] = args.undirected
    setupSTINGERConnection()
    application.run(debug=False,host=args.flask_host,port=args.flask_port)
