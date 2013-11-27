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

  def get_alg_data(self, name, desc):
    return StingerDataArray(self.alg_data, desc, name, Stinger(s=self.stinger))

  def stinger():
    return Stinger(s=self.stinger)

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

class StingerDataArray():
  def __init__(self, data_ptr, data_desc, field_name, s):
    data_desc = data_desc.split(" ")
    data_ptr = data_ptr.value

    field_index = data_desc[1:].index(field_name)

    self.field_name = field_name
    self.data_type = data_desc[0][field_index]
    self.nv = lib['stinger_mon_get_max_nv']()
    self.s = s

    offset = reduce(
	lambda x,y: x+y,
	[8 if c == 'd' or c == 'l' else 
	 4 if c == 'f' or c == 'i' else 
	 1 
	   for c in data_desc[0][:field_index]], 
	0)

    self.data = data_ptr + (offset * self.nv)

    if self.data_type == 'd':
      self.data = cast(self.data, POINTER(c_double))
    elif self.data_type == 'f':
      self.data = cast(self.data, POINTER(c_float))
    elif self.data_type == 'l':
      self.data = cast(self.data, POINTER(c_int64))
    elif self.data_type == 'i':
      self.data = cast(self.data, POINTER(c_int32))
    else: #self.data_type == 'b'
      self.data = cast(self.data, POINTER(c_int8))

  def __getitem__(self, i):
    if isinstance(i, basestring):
      i = self.s.get_mapping(i)
    return self.data[i] if i >= 0 else 0

  def __setitem__(self, i, k):
    if isinstance(i, basestring):
      i = self.s.get_mapping(i)
    if i >= 0:
      self.data[i] = k
      return k
    else:
      return 0


class StingerAlgState():
  def __init__(self, alg, stinger):
    self.alg = alg
    self.s = stinger

  def get_name(self):
    dd = lib['stinger_alg_state_get_name']
    dd.restype = c_char_p
    return str(dd(self.alg))

  def get_data_description(self):
    dd = lib['stinger_alg_state_get_data_description']
    dd.restype = c_char_p
    return str(dd(self.alg))

  def get_data_ptr(self):
    dp = lib['stinger_alg_state_get_data_ptr']
    dp.restype = c_void_p
    return c_void_p(dp(self.alg))

  def get_data_array(self, name):
    return StingerDataArray(self.get_data_ptr(), self.get_data_description(), name, self.s)

  def get_data_per_vertex(self):
    return lib['stinger_alg_state_data_per_vertex'](self.alg)

  def get_level(self):
    return lib['stinger_alg_state_level'](self.alg)

  def number_of_dependencies(self):
    return lib['stinger_alg_state_number_dependencies'](self.alg)

  def get_dependency(self, i):
    dep = lib['stinger_alg_state_depencency']
    dep.restype = c_char_p
    return dep(self.alg, c_int64(i))


class StingerMon():
  def __init__(self, name, host='localhost', port=10103):
    lib['mon_connect'](c_int(port), c_char_p(host), c_char_p(name))
    get_mon = lib['get_stinger_mon']
    get_mon.restype = c_void_p
    self.mon = c_void_p(get_mon())

  def num_algs(self):
    return lib['stinger_mon_num_algs'](self.mon)

  def get_alg_state(self, name_or_int):
    if isinstance(name_or_int, basestring):
      get_alg = lib['stinger_mon_get_alg_state_by_name']
      get_alg.restype = c_void_p
      return StingerAlgState(c_void_p(get_alg(self.mon, c_char_p(name_or_int))), self.stinger())
    else:
      get_alg = lib['stinger_mon_get_alg_state']
      get_alg.restype = c_void_p
      return StingerAlgState(c_void_p(get_alg(self.mon, c_int64(name_or_int))), self.stinger())

  def has_alg(self, name):
    return lib['stinger_mon_has_alg'](self.mon, c_char_p(name))

  def get_read_lock(self):
    lib['stinger_mon_get_read_lock'](self.mon)

  def release_read_lock(self):
    lib['stinger_mon_release_read_lock'](self.mon)
      
  def stinger(self):
    get_stinger = lib['stinger_mon_get_stinger']
    get_stinger.restype = c_void_p
    return Stinger(s=c_void_p(get_stinger(self.mon)))

  def wait_for_sync(self):
    lib['stinger_mon_wait_for_sync'](self.mon)
