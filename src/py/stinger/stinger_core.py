from ctypes import *

libstinger_core = cdll.LoadLibrary('libstinger_core.so')

class Stinger:
  def __init__(self, s=None, filename=None):
    if(filename != None):
      self.s = c_void_p(0)
      nv = c_int64(0)
      libstinger_core['stinger_open_from_file'](c_char_p(filename), c_void_p(addressof(self.s)), 
	  c_void_p(addressof(nv)))
      self.free = True
    elif(None == s):
      stinger_new = libstinger_core['stinger_new']
      stinger_new.restype = c_void_p
      self.free = True
      self.s = c_void_p(stinger_new())
    else:
      self.free = False
      self.s = s

  def __del__(self):
    if(self.free):
      stinger_free_all = libstinger_core['stinger_free_all']
      stinger_free_all.restype = c_void_p
      self.s = stinger_free_all(self.s)

  def save_to_file(self, filename):
    libstinger_core['stinger_save_to_file'](self.s, 1+self.max_active_vtx(), c_char_p(filename))

  def max_active_vtx(self):
    return libstinger_core['stinger_max_active_vertex'](self.s)

  def create_mapping(self, name):
    vtx_out = c_int64(0)
    libstinger_core['stinger_mapping_create'](self.s, c_char_p(name), len(name), c_void_p(addressof(vtx_out)))
    return vtx_out.value

  def get_mapping(self, name):
    return libstinger_core['stinger_mapping_lookup'](self.s, c_char_p(name), len(name))

  def get_name(self, vtx):
    name = c_char_p(0)
    length = c_int64(0)
    libstinger_core['stinger_mapping_physid_direct'](self.s, c_int64(vtx), c_void_p(addressof(name)), c_void_p(addressof(length)))
    rtn = str(name.value[:length.value])
    return rtn
  
  def mapping_nv(self):
    return libstinger_core['stinger_mapping_nv'](self.s)

  def create_vtype(self, name):
    vtx_out = c_int64(0)
    libstinger_core['stinger_vtype_names_create_type'](self.s, c_char_p(name), c_void_p(addressof(vtx_out)))
    return vtx_out.value

  def get_vtype(self, name):
    return libstinger_core['stinger_vtype_names_lookup_type'](self.s, c_char_p(name))

  def get_vtype_name(self, vtype):
    lookup_name = libstinger_core['stinger_vtype_names_lookup_name']
    lookup_name.restype = c_char_p
    return lookup_name(self.s, c_int64(vtype))

  def num_vtypes(self):
    return libstinger_core['stinger_vtype_names_count'](self.s)

  def create_etype(self, name):
    vtx_out = c_int64(0)
    libstinger_core['stinger_etype_names_create_type'](self.s, c_char_p(name), c_void_p(addressof(vtx_out)))
    return vtx_out.value

  def get_etype(self, name):
    return libstinger_core['stinger_etype_names_lookup_type'](self.s, c_char_p(name))

  def get_etype_name(self, etype):
    lookup_name = libstinger_core['stinger_etype_names_lookup_name']
    lookup_name.restype = c_char_p
    return lookup_name(self.s, c_int64(etype))

  def num_etypes(self):
    return libstinger_core['stinger_etype_names_count'](self.s)

  def insert_edge(self, vfrom, vto, etype=0, weight=1, ts=1):
    if isinstance(vfrom, basestring):
      vfrom = self.create_mapping(vfrom)
    if isinstance(vto, basestring):
      vto = self.create_mapping(vto)
    if isinstance(etype, basestring):
      etype = self.create_etype(etype)
    libstinger_core['stinger_insert_edge'](self.s, c_int64(etype), c_int64(vfrom), 
	c_int64(vto), c_int64(weight), c_int64(ts))

  def insert_edge_pair(self, vfrom, vto, etype=0, weight=1, ts=1):
    if isinstance(vfrom, basestring):
      vfrom = self.create_mapping(vfrom)
    if isinstance(vto, basestring):
      vto = self.create_mapping(vto)
    if isinstance(etype, basestring):
      etype = self.create_etype(etype)
    libstinger_core['stinger_insert_edge_pair'](self.s, c_int64(etype), c_int64(vfrom), 
	c_int64(vto), c_int64(weight), c_int64(ts))

  def increment_edge(self, vfrom, vto, etype=0, weight=1, ts=1):
    if isinstance(vfrom, basestring):
      vfrom = self.create_mapping(vfrom)
    if isinstance(vto, basestring):
      vto = self.create_mapping(vto)
    if isinstance(etype, basestring):
      etype = self.create_etype(etype)
    libstinger_core['stinger_incr_edge'](self.s, c_int64(etype), c_int64(vfrom), 
	c_int64(vto), c_int64(weight), c_int64(ts))

  def increment_edge_pair(self, vfrom, vto, etype=0, weight=1, ts=1):
    if isinstance(vfrom, basestring):
      vfrom = self.create_mapping(vfrom)
    if isinstance(vto, basestring):
      vto = self.create_mapping(vto)
    if isinstance(etype, basestring):
      etype = self.create_etype(etype)
    libstinger_core['stinger_incr_edge_pair'](self.s, c_int64(etype), c_int64(vfrom), 
	c_int64(vto), c_int64(weight), c_int64(ts))

  def remove_edge(self, vfrom, vto, etype=0):
    if isinstance(vfrom, basestring):
      vfrom = self.get_mapping(vfrom)
    if isinstance(vto, basestring):
      vto = self.get_mapping(vto)
    if isinstance(etype, basestring):
      etype = self.create_etype(etype)
    if (vfrom > 0) and (vto > 0):
      libstinger_core['stinger_remove_edge'](self.s, c_int64(etype), c_int64(vfrom), 
	  c_int64(vto))

  def remove_edge_pair(self, vfrom, vto, etype=0):
    if isinstance(vfrom, basestring):
      vfrom = self.get_mapping(vfrom)
    if isinstance(vto, basestring):
      vto = self.get_mapping(vto)
    if isinstance(etype, basestring):
      etype = self.create_etype(etype)
    if (vfrom > 0) and (vto > 0):
      libstinger_core['stinger_remove_edge_pair'](self.s, c_int64(etype), c_int64(vfrom), 
	  c_int64(vto))

  def indegree(self, vtx):
    if isinstance(vtx, basestring):
      vtx = self.get_mapping(vtx)
    return libstinger_core['stinger_indegree_get'](self.s, c_int64(vtx))

  def outdegree(self, vtx):
    if isinstance(vtx, basestring):
      vtx = self.get_mapping(vtx)
    return libstinger_core['stinger_outdegree_get'](self.s, c_int64(vtx))

  def get_type(self, vtx):
    if isinstance(vtx, basestring):
      vtx = self.get_mapping(vtx)
    return libstinger_core['stinger_vtype_get'](self.s, c_int64(vtx))

  def set_vtype(self, vtx, vtype):
    if isinstance(vtx, basestring):
      vtx = self.get_mapping(vtx)
    return libstinger_core['stinger_vtype_set'](self.s, c_int64(vtx), c_int64(vtype))

  def get_vweight(self, vtx):
    if isinstance(vtx, basestring):
      vtx = self.get_mapping(vtx)
    return libstinger_core['stinger_vweight_get'](self.s, c_int64(vtx))

  def set_vweight(self, vtx, vweight):
    if isinstance(vtx, basestring):
      vtx = self.get_mapping(vtx)
    return libstinger_core['stinger_vweight_set'](self.s, c_int64(vtx), c_int64(vweight))

  def increment_vweight(self, vtx, vweight):
    if isinstance(vtx, basestring):
      vtx = self.get_mapping(vtx)
    return libstinger_core['stinger_vweight_increment'](self.s, c_int64(vtx), c_int64(vweight))

  def edges_of(self, vtx):
    if isinstance(vtx, basestring):
      vtx_name = vtx
      vtx = self.get_mapping(vtx)

      deg = self.outdegree(vtx)

      outlen = (c_int64 * 1)()
      arr_type = c_int64 * deg

      source = [vtx_name] * deg
      neighbor = arr_type()
      weight = arr_type()
      timefirst = arr_type()
      timerecent = arr_type()
      etype = arr_type()

      libstinger_core['stinger_gather_successors'](self.s, c_int64(vtx),
	  outlen, neighbor, weight, timefirst, timerecent, etype, c_int64(deg))

      neighbor = [self.get_name(v) for v in neighbor]

      max_etypes = self.num_etypes()
      etype = [self.get_etype_name(t) if t < max_etypes else t for t in etype]

      return zip(etype, source, neighbor, weight, timefirst, timerecent)
    else:
      deg = self.outdegree(vtx)

      outlen = (c_int64 * 1)()
      arr_type = c_int64 * deg

      source = (vtx) * deg
      neighbor = arr_type()
      weight = arr_type()
      timefirst = arr_type()
      timerecent = arr_type()
      etype = arr_type()

      libstinger_core['stinger_gather_successors'](self.s, c_int64(vtx),
	  outlen, neighbor, weight, timefirst, timerecent, etype, c_int64(deg))

      return zip(etype, source, neighbor, weight, timefirst, timerecent)
