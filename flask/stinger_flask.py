import sys, os, time, requests, json, threading, signal, logging, argparse, traceback
from flask import Flask, request, jsonify
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)
logging.basicConfig(stream=sys.stderr)

sys.path.append("/home/tgoodyear/projects/blueprints/src/py/")
os.environ['STINGER_LIB_PATH'] = "/home/tgoodyear/projects/blueprints/build/lib/"

import stinger.stinger_net as sn
import stinger.stinger_core as sc

# Value of -1 in either field disables its use as a threshold
TIMEOUT_SECS = 1
BATCH_THRESHOLD = 500

STINGER_HOST = 'localhost'
STINGER_PORT = '8088'


app = Flask(__name__)

@app.route('/insert', methods=['POST'])
def insert():
  global q
  global cur
  global con

  exec_time = time.time()
  if not request.json:
    abort(400)

  # grab the lock
  counter_lock.acquire()

  try:
    data = request.json
    print len(data['edges']), "Edges received"
    if isinstance(data["edges"], list):
      edges = data["edges"]
      for x in edges:
        try:
          source = int(x["src"])
          destination = int(x["dest"])
          edge_type = x["type"]
          timestamp = int(x["time"])
          s.add_insert(source, destination, edge_type, ts=timestamp)
          # print "added edge", source, destination, edge_type, timestamp
        except Exception as e:
          print(traceback.format_exc())
          pass

    # send immediately if the batch is now large
    current_batch_size = s.insertions_count + s.deletions_count
    if current_batch_size > BATCH_THRESHOLD:
      s.send_batch()
      print "Sending batch of size", current_batch_size

  except:
    return jsonify({"error": "invalid input"})

  # end critical section
  finally:
    counter_lock.release()
  exec_time = time.time() - exec_time
  return jsonify({"status": "success", "time": exec_time, "size": current_batch_size}), 201


@app.route('/jsonrpc', methods=['POST'])
def stinger_proxy():
  return stingerRPC(request.json)

@app.route('/algorithms', methods=['GET'])
def get_algs():
  return stingerRPC({"jsonrpc": "2.0", "method": "get_algorithms", "id": 1})

@app.route('/health', methods=['GET'])
def stinger_health():
  return stingerRPC({"jsonrpc": "2.0", "method": "get_server_health", "id": 1})

def stingerRPC(payload):
  try:
    r = requests.post('http://'+ STINGER_HOST +':'+ STINGER_PORT +'/jsonrpc', data=json.dumps(payload))
  except:
    print(traceback.format_exc())
    return jsonify({"error": "JSON-RPC down"})
  return r.text


#
# Initialize the connection to STINGER server
#
def connect(directed=False):
  global s
  s = sn.StingerStream(STINGER_HOST, 10102, directed)

  global counter_lock
  counter_lock = threading.Lock()

#
# attempt to send a batch every TIMEOUT_SECS seconds
#
def sendBatch():
  if s.insertions_count > 0 or s.deletions_count > 0:
    counter_lock.acquire()
    try:
      s.send_batch()
      print "Sending batch with", s.insertions_count, "insertions and", s.deletions_count, "deletions"
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


if not 's' in globals():
  print 'Connecting to STINGER'
  try:
    connect()
  except e as Exception:
    print str(e)
  print 'Connected to STINGER'
if not 't' in globals():
  print 'STINGER timer setup started'
  try:
    sendBatch()
  except e as Exception:
    print str(e)
  print 'STINGER timer setup finished'
#
# main
#
if __name__ == '__main__':
    signal.signal(signal.SIGINT, signal_handler)
    parser = argparse.ArgumentParser(description="STINGER Flask Relay Server")
    parser.add_argument('--directed', action="store_true")
    args = parser.parse_args()
    #connect(args.directed)
    sendBatch()
    app.run(debug=True)
