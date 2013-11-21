from ctypes import *
from stinger_core import Stinger

lib = cdll.LoadLibrary('libstinger_net.so')

class StingerAlgParams(Structure):
  _fields_ = [("name", c_char_p),
	      ("host", c_char_p),
	      ("port", c_int),
	      ("is_remote", c_int),
	      ("map_private", c_int),
	      ("data_per_vertex", c_int64),
	      ("data_description", c_char_p),
	      ("dependencies", c_void_p),
	      ("num_dependencies", c_int64)]

  def set_dependencies(self, deps):
    self.dependencies = (len(deps) * c_char_p)()
    for i in xrange(len(deps)):
      self.dependencies[i] = c_char_p(deps[i])

class StingerEdgeUpdate(Structure):
  _fields_ = [("type", c_int64),
	      ("type_str", c_char_p),
	      ("source", c_int64),
	      ("source_str", c_char_p),
	      ("destination", c_int64),
	      ("destination_str", c_char_p),
	      ("weight", c_int64),
	      ("time", c_int64)]

class StingerRegisteredAlg(Structure):
  _fields_ = [("enabled", c_int),
	      ("map_private", c_int),
	      ("sock", c_int),
	      ("stinger", c_void_p),
	      ("stinger_loc", 256 * c_char),
	      ("alg_name", 256 * c_char),
	      ("alg_num", c_int64),
	      ("alg_data_loc", 256 * c_char),
	      ("alg_data", c_void_p),
	      ("alg_data_per_vertex", c_int64), 
	      ("dep_count", c_int64),
	      ("dep_name", POINTER(c_char_p)),
	      ("dep_location", POINTER(c_char_p)),
	      ("dep_data", POINTER(c_void_p)),
	      ("dep_description", POINTER(c_char_p)),
	      ("dep_data_per_vertex", POINTER(c_int64)),
	      ("batch", c_int64),
	      ("num_insertions", c_int64),
	      ("insertions", POINTER(StingerEdgeUpdate)),
	      ("num_deletions", c_int64),
	      ("deletions", POINTER(StingerEdgeUpdate)),
	      ("batch_storage", c_void_p),
	      ("batch_type", c_int)]

class StingerAlg():
  def __init__(self, params):
    register = lib['stinger_register_alg_impl']
    register.argtypes = [StingerAlgParams]
    register.restype = POINTER(StingerRegisteredAlg)
    self.alg = register(params)

  def begin_init(self):
    lib['stinger_alg_begin_init'](self.alg)

  def end_init(self):
    lib['stinger_alg_end_init'](self.alg)

  def begin_pre(self):
    lib['stinger_alg_begin_pre'](self.alg)

  def end_pre(self):
    lib['stinger_alg_end_pre'](self.alg)

  def begin_post(self):
    lib['stinger_alg_begin_post'](self.alg)

  def end_port(self):
    lib['stinger_alg_end_post'](self.alg)

  def stinger(self):
    return Stinger(s=self.alg.stinger)
