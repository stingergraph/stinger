#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_shared.h"
#include "stinger_utils/stinger_sockets.h"
#include "pystinger.h"

static char module_docstring[] =
"This module provides an interface to a STINGER graph server.";
static char connect_docstring[] =
"Connect to the STINGER graph server.";
static char vquery_docstring[] =
"Takes a vertex ID and returns a list of edges and metadata.";
static char stats_docstring[] =
"Print some useful stats about the graph to which you are connected.";

static PyObject *stinger_connect(PyObject *self, PyObject *args);
static PyObject *stinger_vquery(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *stinger_stats(PyObject *self);

static PyMethodDef module_methods[] = {
  {"connect", stinger_connect, METH_VARARGS, connect_docstring},
  {"vquery", stinger_vquery, METH_VARARGS | METH_KEYWORDS, vquery_docstring},
  {"stats", stinger_stats, METH_NOARGS, stats_docstring},
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initpystinger(void)
{
  PyObject *m = Py_InitModule3("pystinger", module_methods, module_docstring);
  if (m == NULL)
    return;
}

struct stinger * S = NULL;

static PyObject *stinger_connect(PyObject *self, PyObject *args)
{
  /* needs server, port */
  char * hostname = "localhost";
  int port = 10102;
  char name[1024];
  size_t sz;

  /* Parse the input tuple */
  if (!PyArg_ParseTuple(args, "si", &hostname, &port))
    return NULL;

  /* Call the external C function to compute the chi-squared. */
  printf("hostname: %s\n", hostname);
  printf("port: %d\n", port);
  get_shared_map_info (hostname, port, &name, 1024, &sz);
  printf("map_name: %s\n", name);
  printf("size: %ld\n", sz);

  /* Map the STINGER */
  S = stinger_shared_map (name, sz);

  if (S == NULL) {
    PyErr_SetString(PyExc_RuntimeError,
	"Failed to map the STINGER.");
    return NULL;
  }

  /* Build the output tuple */
  PyObject *ret = Py_BuildValue("i", 0);
  return ret;
}

static PyObject *stinger_vquery(PyObject *self, PyObject *args, PyObject *keywds)
{
  char * vertex_name = "\0";
  int64_t timerecentmin = -1;
  int64_t timerecentmax = -1;
  int64_t timefirstmin = -1;
  int64_t timefirstmax = -1;
  int64_t weightmax = -1;
  int64_t weightmin = -1;
  char * vtype = NULL;
  char * etype = NULL;

  int64_t vtx = 0;

  static char *kwlist[] = {"vtx","timerecentmin","timerecentmax","timefirstmin","timefirstmax","weightmax","weightmin","vtype","etype",NULL};

  /* Parse the input tuple */
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "l|llllllss", kwlist, &vtx, &timerecentmin, &timerecentmax, &timefirstmin, &timefirstmax, &weightmax, &weightmin, &vtype, &etype))
    return NULL;

  if (S == NULL) {
    PyErr_SetString(PyExc_RuntimeError,
	"Failed to map the STINGER.");
    return NULL;
  }

  global_config conf;
  parse_config(S, timerecentmin, timerecentmax, timefirstmin, timefirstmax, weightmax, weightmin, vtype, etype, &conf);

  //int64_t vtx = stinger_mapping_lookup(S, vertex_name, strlen(vertex_name));

  PyObject *dict = NULL;
  PyListObject *list;

  list = (PyListObject *) Py_BuildValue("[]");

  if (vtx >= 0) {
    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, vtx) {
      if(matches(S, &conf, STINGER_EDGE_TYPE, STINGER_EDGE_DEST, STINGER_EDGE_WEIGHT, STINGER_EDGE_TIME_FIRST, STINGER_EDGE_TIME_RECENT)) {
	char * id;
	int64_t id_len;
	stinger_mapping_physid_direct(S, STINGER_EDGE_DEST, &id, &id_len);
	dict = Py_BuildValue ("{s:l,s:s,s:l,s:s,s:s,s:l,s:l,s:l,s:s,s:s}",
			      "SourceID", vtx,
			      "SourceString", vertex_name,
			      "DestinationID", STINGER_EDGE_DEST,
			      "DestinationString", id,
			      "EdgeType", stinger_etype_names_lookup_name (S, STINGER_EDGE_TYPE),
			      "EdgeWeight", STINGER_EDGE_WEIGHT,
			      "EdgeTimeFirst", STINGER_EDGE_TIME_FIRST,
			      "EdgeTimeRecent", STINGER_EDGE_TIME_RECENT,
			      "SourceType", stinger_vtype_names_lookup_name (S, stinger_vtype_get(S, vtx)),
			      "DestinationType", stinger_vtype_names_lookup_name (S, stinger_vtype_get(S, STINGER_EDGE_DEST))
			      );
	PyList_Append(list, dict);
      }
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }


  /* Build the output tuple */
  return (PyObject *) list;
}

static PyObject *stinger_stats(PyObject *self)
{
  if (S == NULL) {
    PyErr_SetString(PyExc_RuntimeError,
	"Error: must connect to graph first.");
    return NULL;
  }
    
  uint64_t nv = stinger_num_active_vertices (S);
  uint64_t max_nv = stinger_max_active_vertex (S);
  int64_t ne = stinger_total_edges (S);
  uint32_t check = stinger_consistency_check (S, max_nv+1);

  printf("nv: %ld\n", nv);
  printf("max_nv: %ld\n", max_nv);
  printf("ne: %ld\n", ne);
  printf("check: %d\n", check);

  /* Build the output tuple */
  PyObject *ret = Py_BuildValue("i", 0);
  return ret;
}
