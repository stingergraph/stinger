import numpy as np
import logging
import sys
from itertools import izip
from struct import unpack

def init_logging(loglevel="INFO", filename=None, filemode='w'):
  numeric_level = getattr(logging, loglevel.upper(), None)
  if not isinstance(numeric_level, int):
    raise ValueError('Invalid log level: %s' % loglevel)
  if(filename):
    logging.basicConfig(level=numeric_level, filename=filename, filemode=filemode)
  else:
    logging.basicConfig(level=numeric_level)

def load_alg_data(filename):
  """
  read a binary algorithm data result file produced by STINGER

  Reads the file in filename into a dictionary like so:
  {
    name: algorithm_name
    desc: algorithm description string
    nv_max: maximum number of vertices in STINGER instance that produced this file
    nv: actual number of vertices present in STINGER at the time (and in this file)
    fields: field names mapped to nv-length numpy arrays of values of the appropriate type
    {
      field_name1: np.array([0 .. nv]),
      field_name2: ...
    }
  }

  Field types can be 32 and 64-bit integers and floats and bytes.
  """
  try:
    out = dict()
    fp = open(filename)

    name_len = unpack('q', fp.read(8))[0]
    out['name'] = str(fp.read(name_len))

    name_len = unpack('q', fp.read(8))[0]
    out['desc'] = str(fp.read(name_len))

    out['nv_max'] = unpack('q', fp.read(8))[0]
    out['nv'] = unpack('q', fp.read(8))[0]
    nv = out['nv']

    fields = out['desc'].split(' ')
    out['fields'] = dict()

    for t,field in izip(fields[0], fields[1:]):
      if t == 'f':
	out['fields'][field] = np.array(unpack(str(nv) + 'f',fp.read(4 * nv)), np.float32)
      elif t == 'd':
	out['fields'][field] = np.array(unpack(str(nv) + 'd',fp.read(8 * nv)), np.float64)
      elif t == 'i':
	out['fields'][field] = np.array(unpack(str(nv) + 'i',fp.read(4 * nv)), np.int32)
      elif t == 'l':
	out['fields'][field] = np.array(unpack(str(nv) + 'q',fp.read(8 * nv)), np.int64)
      elif t == 'b':
	out['fields'][field] = np.array(unpack(str(nv) + 'b',fp.read(1 * nv)), np.int8)
      else:
	out['fields'][field] = np.array(xrange(nv), np.int8)
	logging.error("Unknown field type: " + t + ". Filling with xrange...")

    logging.info(str('Parsed alg ::' + out['name'] + '::' +  out['desc'] + '::'))

    return out
  except:
    logging.error("Error reading in file " + filename)

def load_vertex_data(filename):
  """
  read a vertex name mapping file produced by STINGER

  Reads the file in filename into a dictionary like so:
  {
    to_name: an array of string names such that the numberical index of a vertex can 
      be used to look up the string name
      [name0, name1, name2, ...] 
    to_inex: a dictionary mapping string names of vertices to their respective indices
      { name0: 0, name1: 1, name2: 2, ... }
  }
  """
  try:
    out = { 'to_name': [], 'to_index': {} }
    fp = open(filename)

    max_len = unpack('q', fp.read(8))[0]
    count = unpack('q', fp.read(8))[0]

    for n in xrange(count):
      str_len = unpack('q', fp.read(8))[0]
      name = str(fp.read(min(str_len, max_len)))
      out['to_name'].append(name)
      out['to_index'][name] = n

    return out
  except:
    logging.error("Error reading in file " + filename)
