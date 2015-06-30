import sys
import os
import time
import requests
import json
import threading
import signal
from flask import Flask, request, jsonify
import logging
import argparse
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

sys.path.append("/path/to/stinger/src/py/")
os.environ['STINGER_LIB_PATH'] = "/path/to/stinger/build/lib/"

import stinger.stinger_net as sn
import stinger.stinger_core as sc

TIMEOUT_SECS = 5
BATCH_THRESHOLD = 1000


app = Flask(__name__)

@app.route('/insert', methods=['POST'])
def insert():
  exec_time = time.time()
  if not request.json:
    requests.abort(400)

  # grab the lock
  counter_lock.acquire()

  try:
    data = request.json
    if isinstance(data["edges"], list):
      edges = data["edges"]
      for x in edges:
	try:
	  source = x["src"]
	  destination = x["dest"]
	  edge_type = x["type"]
	  timestamp = x["time"]
	  s.add_insert(source, destination, edge_type, ts=timestamp)
	except:
	  pass

    # send immediately if the batch is now large
    current_batch_size = s.insertions_count + s.deletions_count
    if current_batch_size > BATCH_THRESHOLD:
      s.send_batch()

  except:
    return jsonify({"error": "invalid input"})

  # end critical section
  finally:
    counter_lock.release()
  exec_time = time.time() - exec_time
  return jsonify({"status": "success", "time": exec_time}), 201

@app.route('/jsonrpc', methods=['POST'])
def stinger_proxy():
  payload = request.json
  try:
    r = requests.post('http://localhost:8088/jsonrpc', data=json.dumps(payload))
  except:
    return jsonify({"error": "JSON-RPC down"})
  return r.text

#
# Initialize the connection to STINGER server
#
def connect(directed=False):
  global s
  s = sn.StingerStream('localhost', 10102, directed)

  global counter_lock
  counter_lock = threading.Lock()

#
# attempt to send a batch every 60 seconds
#
def sendBatch():
  if s.insertions_count > 0 or s.deletions_count > 0:
    counter_lock.acquire()
    try:
      s.send_batch()
    finally:
      counter_lock.release()
  global t
  t = threading.Timer(TIMEOUT_SECS, sendBatch)
  t.start()

def signal_handler(signal, frame):
  try:
    t.cancel()
  except:
    pass
  sys.exit(0)

#
# main
#
if __name__ == '__main__':
  signal.signal(signal.SIGINT, signal_handler)
  parser = argparse.ArgumentParser(description="STINGER Flask Relay Server")
  parser.add_argument('--directed', action="store_true")
  args = parser.parse_args()
  connect(args.directed)
  sendBatch()
  app.run()

