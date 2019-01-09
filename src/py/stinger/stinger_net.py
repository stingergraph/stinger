from ctypes import *
import os
from stinger_core import Stinger

if(os.getenv('STINGER_LIB_PATH')):
  libstinger_net = cdll.LoadLibrary(os.getenv('STINGER_LIB_PATH') + '/libstinger_net.so')
else:
  libstinger_net = cdll.LoadLibrary('libstinger_net.so')

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
  _fields_ = [("etype", c_int64),
	      ("etype_str", c_char_p),
	      ("source", c_int64),
	      ("source_str", c_char_p),
	      ("destination", c_int64),
	      ("destination_str", c_char_p),
	      ("weight", c_int64),
	      ("time", c_int64),
	      ("result", c_int64),
	      ("meta_index", c_int64)]

class StingerVertexUpdate(Structure):
  _fields_ = [("vertex", c_int64),
	      ("vertex_str", c_char_p),
	      ("type", c_int64),
	      ("type_str", c_char_p),
	      ("set_weight", c_int64),
	      ("incr_weight", c_int64),
	      ("meta_index", c_int64)]

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
	      ("num_vertex_updates", c_int64),
	      ("vertex_updates", POINTER(StingerVertexUpdate)),
	      ("num_metadata", c_int64),
	      ("metadata", POINTER(c_char_p)),
	      ("metadata_lengths", POINTER(c_int64)),
	      ("batch_storage", c_void_p),
	      ("batch_type", c_int)]

  def get_alg_data(self, name, desc):
    return StingerDataArray(self.alg_data, desc, name, Stinger(s=self.stinger))

  def stinger():
    return Stinger(s=self.stinger)

class StingerStream():
  def __init__(self, host, port, strings=True, undirected=False):
    self.connect(host, port)
    self.host = host
    self.port = port
    self.insertions_size = 5000
    self.insertions = (StingerEdgeUpdate * self.insertions_size)()
    self.insertions_refs = []
    self.insertions_count = 0
    self.deletions_size = 5000
    self.deletions = (StingerEdgeUpdate * self.deletions_size)()
    self.deletions_refs = []
    self.deletions_count = 0
    self.vertex_updates_size = 5000
    self.vertex_updates = (StingerVertexUpdate * self.deletions_size)()
    self.vertex_updates_refs = []
    self.vertex_updates_count = 0
    self.only_strings = strings
    self.undirected = undirected

  def connect(self, host, port):
    self.sock_handle = libstinger_net['stream_connect'](c_char_p(host), c_int(port))
    return self.sock_handle != -1

  def is_connected(self):
    return libstinger_net['stream_is_connected'](c_int(self.sock_handle))

  def add_insert(self, vfrom, vto, etype=0, weight=0, ts=0, insert_strings=None):
    self.only_strings = insert_strings if insert_strings is not None else self.only_strings

    if(self.insertions_count >= self.insertions_size):
      self.insertions_size *= 2
      insertions_tmp = (StingerEdgeUpdate * self.insertions_size)()
      self.insertions_refs.append(self.insertions)
      memmove(addressof(insertions_tmp), addressof(self.insertions), sizeof(StingerEdgeUpdate * (self.insertions_size/2)))
      self.insertions = insertions_tmp

    if(self.only_strings):
      self.insertions[self.insertions_count].source_str = c_char_p(vfrom)
      self.insertions[self.insertions_count].destination_str = c_char_p(vto)
    else:
      self.insertions[self.insertions_count].source_str = 0
      self.insertions[self.insertions_count].destination_str = 0
      self.insertions[self.insertions_count].source = c_int64(vfrom)
      self.insertions[self.insertions_count].destination= c_int64(vto)

    if isinstance(etype, basestring):
      self.insertions[self.insertions_count].etype_str = c_char_p(etype)
    else:
      self.insertions[self.insertions_count].etype = c_int64(etype)

    self.insertions[self.insertions_count].weight = c_int64(weight)
    self.insertions[self.insertions_count].time = c_int64(ts)

    self.insertions_count += 1

  def add_delete(self, vfrom, vto, etype=0):
    if(self.deletions_count >= self.deletions_size):
      self.deletions_size *= 2
      deletions_tmp = (StingerEdgeUpdate * self.deletions_size)()
      self.deletions_refs.append(self.deletions)
      memmove(addressof(deletions_tmp), addressof(self.deletions), sizeof(StingerEdgeUpdate * (self.deletions_size/2)))
      self.deletions = deletions_tmp

    if(self.only_strings):
      self.deletions[self.deletions_count].source_str = c_char_p(vfrom)
      self.deletions[self.deletions_count].destination_str = c_char_p(vto)
    else:
      self.deletions[self.deletions_count].source_str = 0
      self.deletions[self.deletions_count].destination_str = 0
      self.deletions[self.deletions_count].source = c_int64(vfrom)
      self.deletions[self.deletions_count].destination= c_int64(vto)

    if isinstance(etype, basestring):
      self.deletions[self.deletions_count].etype_str = c_char_p(etype)
    else:
      self.deletions[self.deletions_count].etype = c_int64(etype)

    self.deletions_count += 1

  def add_vertex_update(self, vtx, vtype, weight=0, incr_weight=0):
    if(self.vertex_updates_count >= self.vertex_updates_size):
      self.vertex_updates_size *= 2
      vertex_updates_tmp = (StingerVertexUpdate * self.vertex_updates_size)()
      self.vertex_updates_refs.append(self.vertex_updates)
      memmove(addressof(vertex_updates_tmp), addressof(self.vertex_updates), sizeof(StingerVertexUpdate * (self.vertex_updates_size/2)))
      self.vertex_updates = vertex_updates_tmp

    if(self.only_strings):
      self.vertex_updates[self.vertex_updates_count].vertex_str = c_char_p(vtx)
    else:
      self.vertex_updates[self.vertex_updates_count].vertex = c_int64(vtx)
      self.vertex_updates[self.vertex_updates_count].vertex_str = 0

    if isinstance(vtype, basestring):
      self.vertex_updates[self.vertex_updates_count].type_str = c_char_p(vtype)
    else:
      self.vertex_updates[self.vertex_updates_count].type = c_int64(vtype)

    self.vertex_updates[self.vertex_updates_count].weight = c_int64(weight)
    self.vertex_updates[self.vertex_updates_count].incr_weight = c_int64(incr_weight)

    self.vertex_updates_count += 1

  def send_batch(self):
    success = libstinger_net['stream_send_batch'](self.sock_handle, c_int(self.only_strings),
	     self.insertions, self.insertions_count,
       self.deletions, self.deletions_count,
       self.vertex_updates, self.vertex_updates_count,
       self.undirected)
    if success:
        self.insertions_count = 0
        self.deletions_count = 0
        self.vertex_updates_count = 0
        self.insertions_refs = []
        self.deletions_refs = []
        self.vertex_updates_refs = []
    else:
        print('Lost connection to STINGER core server, attempting to reconnect...')
        if self.connect(self.host, self.port):
            print('Reattempting batch send...')
            self.send_batch()
            print('Batch sent successfully')
        else:
            raise RuntimeError()





class StingerAlg():
  def __init__(self, params):
    register = libstinger_net['stinger_register_alg_impl']
    register.argtypes = [StingerAlgParams]
    register.restype = POINTER(StingerRegisteredAlg)
    self.alg_ptr = register(params)
    self.alg = self.alg_ptr[0]

  def begin_init(self):
    libstinger_net['stinger_alg_begin_init'](self.alg_ptr)

  def end_init(self):
    libstinger_net['stinger_alg_end_init'](self.alg_ptr)

  def begin_pre(self):
    libstinger_net['stinger_alg_begin_pre'](self.alg_ptr)

  def end_pre(self):
    libstinger_net['stinger_alg_end_pre'](self.alg_ptr)

  def begin_post(self):
    libstinger_net['stinger_alg_begin_post'](self.alg_ptr)

  def end_post(self):
    libstinger_net['stinger_alg_end_post'](self.alg_ptr)

  def stinger(self):
    return Stinger(s=self.alg.stinger)

class StingerDataArray():
  def __init__(self, data_ptr, data_desc, field_name, s):
    data_desc = data_desc.split()

    if not isinstance(data_ptr, int):
      data_ptr = data_ptr.value

    print data_desc
    field_index = data_desc[1:].index(field_name)

    self.field_name = field_name
    self.data_type = data_desc[0][field_index]
    self.nv = libstinger_net['stinger_alg_max_vertices'](s.raw())
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

  def get_data_ptr(self):
    return self.data

  def weight_double(self, weight):
    libstinger_net['stinger_alg_weight_double'](self.s.raw(), self.data, c_double(weight))


class StingerAlgState():
  def __init__(self, alg, stinger):
    self.alg = alg
    self.s = stinger

  def get_name(self):
    dd = libstinger_net['stinger_alg_state_get_name']
    dd.restype = c_char_p
    return str(dd(self.alg))

  def get_data_description(self):
    dd = libstinger_net['stinger_alg_state_get_data_description']
    dd.restype = c_char_p
    return str(dd(self.alg))

  def get_data_ptr(self):
    dp = libstinger_net['stinger_alg_state_get_data_ptr']
    dp.restype = c_void_p
    return c_void_p(dp(self.alg))

  def get_data_array(self, name):
    return StingerDataArray(self.get_data_ptr(), self.get_data_description(), name, self.s)

  def get_data_per_vertex(self):
    return libstinger_net['stinger_alg_state_data_per_vertex'](self.alg)

  def get_level(self):
    return libstinger_net['stinger_alg_state_level'](self.alg)

  def number_of_dependencies(self):
    return libstinger_net['stinger_alg_state_number_dependencies'](self.alg)

  def get_dependency(self, i):
    dep = libstinger_net['stinger_alg_state_depencency']
    dep.restype = c_char_p
    return dep(self.alg, c_int64(i))


class StingerMon():
  def __init__(self, name, host='localhost', port=10103):
    libstinger_net['mon_connect'](c_int(port), c_char_p(host), c_char_p(name))
    get_mon = libstinger_net['get_stinger_mon']
    get_mon.restype = c_void_p
    self.mon = c_void_p(get_mon())

  def num_algs(self):
    return libstinger_net['stinger_mon_num_algs'](self.mon)

  def get_alg_state(self, name_or_int):
    if isinstance(name_or_int, basestring):
      get_alg = libstinger_net['stinger_mon_get_alg_state_by_name']
      get_alg.restype = c_void_p
      return StingerAlgState(c_void_p(get_alg(self.mon, c_char_p(name_or_int))), self.stinger())
    else:
      get_alg = libstinger_net['stinger_mon_get_alg_state']
      get_alg.restype = c_void_p
      return StingerAlgState(c_void_p(get_alg(self.mon, c_int64(name_or_int))), self.stinger())

  def has_alg(self, name):
    return libstinger_net['stinger_mon_has_alg'](self.mon, c_char_p(name))

  def get_read_lock(self):
    libstinger_net['stinger_mon_get_read_lock'](self.mon)

  def release_read_lock(self):
    libstinger_net['stinger_mon_release_read_lock'](self.mon)

  def stinger(self):
    get_stinger = libstinger_net['stinger_mon_get_stinger']
    get_stinger.restype = c_void_p
    return Stinger(s=c_void_p(get_stinger(self.mon)))

  def wait_for_sync(self):
    libstinger_net['stinger_mon_wait_for_sync'](self.mon)
