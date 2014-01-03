#ifndef  EXPLORE_CSV_H
#define  EXPLORE_CSV_H

#include "stinger_net/proto/stinger-batch.pb.h"

#define LOG_AT_W
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;

/* 
 * EdgeCollectionSet
 * - have apply() and learn()
 * - has EdgeCollection(s)
 *   - have apply()
 *   - use linked lists of ExploreCSVGenerics
 *     - have operator(), print(), and copy()
 */

typedef enum {
  VALUE_TYPE,
  VALUE_TYPE_STR,
  VALUE_SOURCE,
  VALUE_SOURCE_STR,
  VALUE_SOURCE_TYPE,
  VALUE_SOURCE_WEIGHT,
  VALUE_SOURCE_WEIGHT_INCR,
  VALUE_DESTINATION,
  VALUE_DESTINATION_STR,
  VALUE_DESTINATION_TYPE,
  VALUE_DESTINATION_WEIGHT,
  VALUE_DESTINATION_WEIGHT_INCR,
  VALUE_WEIGHT,
  VALUE_TIME
} value_type_t;

typedef enum {
  PATH_DEFAULT,
  PATH_EXACT,
  PATH_ORDERED
} path_type_t;

struct EdgeCollection;

struct ExploreCSVGeneric {
  ExploreCSVGeneric * child;
  ExploreCSVGeneric() : child(NULL) {}
  ~ExploreCSVGeneric() { if (child) delete child; }

  virtual bool operator()(EdgeCollection & edges, char ** fields, int64_t * lengths, int64_t count, char * field) { LOG_E("Error, this is a generic object"); }
  virtual void print() { LOG_E("Error, this is a generic object"); }
  virtual ExploreCSVGeneric * copy(path_type_t path) { LOG_E("Error, this is a generic object"); }
};

struct EdgeCollection {
  std::vector<ExploreCSVGeneric *> start;
  path_type_t path;

  EdgeCollection() : path(PATH_DEFAULT) {
    has_const_type = false;
    has_const_type_str = false;
    has_const_source = false;
    has_const_source_str = false;
    has_const_source_type = false;
    has_const_source_weight = false;
    has_const_source_weight_incr = false;
    has_const_destination = false;
    has_const_destination_str = false;
    has_const_destination_type = false;
    has_const_destination_weight = false;
    has_const_destination_weight_incr = false;
    has_const_weight = false;
    has_const_time = false;
  }

  ~EdgeCollection() { 
    for(int64_t s = 0; s < start.size(); s++) {
      if(start[s])
	delete start[s];
    }
  }

  int64_t const_type;
  std::string const_type_str;
  int64_t const_source;
  std::string const_source_str;
  std::string const_source_type;
  int64_t const_source_weight;
  int64_t const_destination;
  std::string const_destination_str;
  std::string const_destination_type;
  int64_t const_destination_weight;
  int64_t const_weight;
  int64_t const_time;

  bool has_const_type;
  bool has_const_type_str;
  bool has_const_source;
  bool has_const_source_str;
  bool has_const_source_type;
  bool has_const_source_weight;
  bool has_const_source_weight_incr;
  bool has_const_destination;
  bool has_const_destination_str;
  bool has_const_destination_type;
  bool has_const_destination_weight;
  bool has_const_destination_weight_incr;
  bool has_const_weight;
  bool has_const_time;

  std::vector<int64_t> type;
  std::vector<std::string> type_str;
  std::vector<int64_t> source;
  std::vector<std::string> source_str;
  std::vector<std::string> source_type;
  std::vector<int64_t> source_weight;
  std::vector<int64_t> source_weight_incr;
  std::vector<int64_t> destination;
  std::vector<std::string> destination_str;
  std::vector<std::string> destination_type;
  std::vector<int64_t> destination_weight;
  std::vector<int64_t> destination_weight_incr;
  std::vector<int64_t> weight;
  std::vector<int64_t> time;

  EdgeInsertion *
  get_insertion(StingerBatch & batch, int64_t meta_index) {
    LOG_D("called")
    EdgeInsertion * in = batch.add_insertions();
    in->set_meta_index(meta_index);
    if(has_const_type) {
      in->set_type(const_type);
    }
    if(has_const_type_str) {
      in->set_type_str(const_type_str);
    }
    if(has_const_source) {
      in->set_source(const_source);
    }
    if(has_const_source_str) {
      in->set_source_str(const_source_str);
    }
    if(has_const_destination) {
      in->set_destination(const_destination);
    }
    if(has_const_destination_str) {
      in->set_destination_str(const_destination_str);
    }
    if(has_const_weight) {
      in->set_weight(const_weight);
    }
    if(has_const_time) {
      in->set_time(const_time);
    }
    return in;
  }

  template<bool is_source>
  VertexUpdate *
  get_vertex_update(StingerBatch & batch) {
    LOG_D("called")
    VertexUpdate * up = batch.add_vertex_updates();
    if(is_source) {
      if(has_const_source) {
	up->set_vertex(const_source);
      }
      if(has_const_source_str) {
	up->set_vertex_str(const_source_str);
      }
    if(has_const_source_type) {
	up->set_type_str(const_source_type);
    }
    if(has_const_source_weight) {
	up->set_set_weight(const_source_weight);
      }
      if(has_const_source_weight_incr) {
	up->set_incr_weight(const_source_weight);
    }
    } else {
    if(has_const_destination) {
	up->set_vertex(const_destination);
    }
    if(has_const_destination_str) {
	up->set_vertex_str(const_destination_str);
    }
    if(has_const_destination_type) {
	up->set_type_str(const_destination_type);
    }
    if(has_const_destination_weight) {
	up->set_set_weight(const_destination_weight);
    }
      if(has_const_destination_weight_incr) {
	up->set_incr_weight(const_destination_weight);
    }
    }
    return up;
  }

  void print() {
    for(int64_t s = 0; s < start.size(); s++) {
      if(start[s])
	start[s]->print();
    }
  }

  template<bool use_last, bool source_is_int, bool dest_is_int>
  inline void
  handle_vtypes_vweights(StingerBatch & batch, int64_t meta_index, int64_t source, const std::string & source_str, int64_t dest, const std::string & dest_str, 
	int64_t & src_type, int64_t & src_weight, int64_t & src_weight_incr, int64_t & dest_type, int64_t & dest_weight, int64_t & dest_weight_incr) {

    bool do_source = has_const_source_type || has_const_source_weight || has_const_source_weight_incr ||
		     source_type.size() || source_weight.size() || source_weight_incr.size();

    bool do_destination = has_const_destination_type || has_const_destination_weight || has_const_destination_weight_incr ||
		     destination_type.size() || destination_weight.size() || destination_weight_incr.size();


    if(do_source) {
      VertexUpdate * up = get_vertex_update<true>(batch);
      up->set_meta_index(meta_index);

    if(src_type < source_type.size()) {
	up->set_type_str(source_type[src_type++]);
    } else if(use_last && source_type.size()) {
	up->set_type_str(source_type[source_type.size()-1]);
      src_type++;
    }

    if(src_weight < source_weight.size()) {
	up->set_set_weight(source_weight[src_weight++]);
    } else if(use_last && source_weight.size()) {
	up->set_set_weight(source_weight[source_weight.size()-1]);
      src_weight++;
    }

      if(src_weight_incr < source_weight_incr.size()) {
	up->set_incr_weight(source_weight_incr[src_weight_incr++]);
      } else if(use_last && source_weight_incr.size()) {
	up->set_incr_weight(source_weight_incr[source_weight_incr.size()-1]);
	src_weight_incr++;
      }

      if(!has_const_source && !has_const_destination_str) {
	if(source_is_int) {
	  up->set_vertex(source);
	} else {
	  up->set_vertex_str(source_str);
	}
      }
    }

    if(do_destination) {
      VertexUpdate * up = get_vertex_update<false>(batch);
      up->set_meta_index(meta_index);

    if(dest_type < destination_type.size()) {
	up->set_type_str(destination_type[dest_type++]);
    } else if(use_last && destination_type.size()) {
	up->set_type_str(destination_type[destination_type.size()-1]);
      dest_type++;
    }

    if(dest_weight < destination_weight.size()) {
	up->set_set_weight(destination_weight[dest_weight++]);
    } else if(use_last && destination_weight.size()) {
	up->set_set_weight(destination_weight[destination_weight.size()-1]);
      dest_weight++;
    }

      if(dest_weight_incr < destination_weight_incr.size()) {
	up->set_incr_weight(destination_weight_incr[dest_weight_incr++]);
      } else if(use_last && destination_weight_incr.size()) {
	up->set_incr_weight(destination_weight_incr[destination_weight_incr.size()-1]);
	dest_weight_incr++;
  }

      if(!has_const_destination && !has_const_destination_str) {
	if(dest_is_int) {
	  up->set_vertex(dest);
	} else {
	  up->set_vertex_str(dest_str);
	}
      }
    }

  }

  int64_t
  apply(StingerBatch & batch, char ** fields, int64_t * lengths, int64_t count, int64_t & timestamp, int64_t meta_index) {
    type.clear();
    type_str.clear();
    source.clear();
    source_str.clear();
    source_type.clear();
    source_weight.clear();
    source_weight_incr.clear();
    destination.clear();
    destination_str.clear();
    destination_type.clear();
    destination_weight.clear();
    destination_weight_incr.clear();
    weight.clear();
    time.clear();

    /* TODO figure out what to do about missing fields */
    for(int64_t i = 0; i < start.size(); i++) {
      if(!(*start[i])(*this, fields, lengths, count, NULL)) {
	LOG_E_A("Index %ld skipped", i);
	return 0;
      }
    }

    int64_t src_type = 0;
    int64_t src_weight = 0;
    int64_t src_weight_incr = 0;
    int64_t dest_type = 0;
    int64_t dest_weight = 0;
    int64_t dest_weight_incr = 0;

    switch(path) {
      default:
      case PATH_EXACT:
      case PATH_DEFAULT: {
	for(int64_t t = 0; t < type.size(); t++) {
	  for(int64_t s = 0; s < source.size(); s++) {
	    for(int64_t d = 0; d < destination.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = get_insertion(batch, meta_index);
		      in->set_type(type[t]); in->set_source(source[s]); in->set_destination(destination[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		      handle_vtypes_vweights<true, true, true>(batch, meta_index, source[s], NULL, destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		    }
		  } else {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(type[t]); in->set_source(source[s]); in->set_destination(destination[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		    handle_vtypes_vweights<true, true, true>(batch, meta_index, source[s], NULL, destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		}
	      } else {
		if(time.size()) {
		  for(int64_t m = 0; m < time.size(); m++) {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(type[t]); in->set_source(source[s]); in->set_destination(destination[d]);
		    in->set_weight(1); in->set_time(time[m]);
		    handle_vtypes_vweights<true, true, true>(batch, meta_index, source[s], NULL, destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		} else {
		  EdgeInsertion * in = get_insertion(batch, meta_index);
		  in->set_type(type[t]); in->set_source(source[s]); in->set_destination(destination[d]);
		  in->set_weight(1); in->set_time(timestamp++);
		  handle_vtypes_vweights<true, true, true>(batch, meta_index, source[s], NULL, destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		}
	      }
	    }
	    for(int64_t d = 0; d < destination_str.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = get_insertion(batch, meta_index);
		      in->set_type(type[t]); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		      handle_vtypes_vweights<true, true, false>(batch, meta_index, source[s], NULL, 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		    }
		  } else {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(type[t]); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		    handle_vtypes_vweights<true, true, false>(batch, meta_index, source[s], NULL, 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		}
	      } else {
		if(time.size()) {
		  for(int64_t m = 0; m < time.size(); m++) {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(type[t]); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(1); in->set_time(time[m]);
		    handle_vtypes_vweights<true, true, false>(batch, meta_index, source[s], NULL, 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		} else {
		  EdgeInsertion * in = get_insertion(batch, meta_index);
		  in->set_type(type[t]); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		  in->set_weight(1); in->set_time(timestamp++);
		  handle_vtypes_vweights<true, true, false>(batch, meta_index, source[s], NULL, 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		}
	      }
	    }
	  }
	  for(int64_t s = 0; s < source_str.size(); s++) {
	    for(int64_t d = 0; d < destination.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = get_insertion(batch, meta_index);
		      in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		      handle_vtypes_vweights<true, false, true>(batch, meta_index, 0, source_str[s], destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		    }
		  } else {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		    handle_vtypes_vweights<true, false, true>(batch, meta_index, 0, source_str[s], destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		}
	      } else {
		if(time.size()) {
		  for(int64_t m = 0; m < time.size(); m++) {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		    in->set_weight(1); in->set_time(time[m]);
		    handle_vtypes_vweights<true, false, true>(batch, meta_index, 0, source_str[s], destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		} else {
		  EdgeInsertion * in = get_insertion(batch, meta_index);
		  in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		  in->set_weight(1); in->set_time(timestamp++);
		  handle_vtypes_vweights<true, false, true>(batch, meta_index, 0, source_str[s], destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		}
	      }
	    }
	    for(int64_t d = 0; d < destination_str.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = get_insertion(batch, meta_index);
		      in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		      handle_vtypes_vweights<true, false, false>(batch, meta_index, 0, source_str[s], 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		    }
		  } else {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		    handle_vtypes_vweights<true, false, false>(batch, meta_index, 0, source_str[s], 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		}
	      } else {
		if(time.size()) {
		  for(int64_t m = 0; m < time.size(); m++) {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(1); in->set_time(time[m]);
		    handle_vtypes_vweights<true, false, false>(batch, meta_index, 0, source_str[s], 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		} else {
		  EdgeInsertion * in = get_insertion(batch, meta_index);
		  in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		  in->set_weight(1); in->set_time(timestamp++);
		  handle_vtypes_vweights<true, false, false>(batch, meta_index, 0, source_str[s], 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		}
	      }
	    }
	  }
	}
	for(int64_t t = 0; t < type_str.size(); t++) {
	  for(int64_t s = 0; s < source.size(); s++) {
	    for(int64_t d = 0; d < destination.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = get_insertion(batch, meta_index);
		      in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		      handle_vtypes_vweights<true, true, true>(batch, meta_index, source[s], NULL, destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		    }
		  } else {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		    handle_vtypes_vweights<true, true, true>(batch, meta_index, source[s], NULL, destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		}
	      } else {
		if(time.size()) {
		  for(int64_t m = 0; m < time.size(); m++) {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		    in->set_weight(1); in->set_time(time[m]);
		    handle_vtypes_vweights<true, true, true>(batch, meta_index, source[s], NULL, destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		} else {
		  EdgeInsertion * in = get_insertion(batch, meta_index);
		  in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		  in->set_weight(1); in->set_time(timestamp++);
		  handle_vtypes_vweights<true, true, true>(batch, meta_index, source[s], NULL, destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		}
	      }
	    }
	    for(int64_t d = 0; d < destination_str.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = get_insertion(batch, meta_index);
		      in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		      handle_vtypes_vweights<true, true, false>(batch, meta_index, source[s], NULL, 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		    }
		  } else {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		    handle_vtypes_vweights<true, true, false>(batch, meta_index, source[s], NULL, 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		}
	      } else {
		if(time.size()) {
		  for(int64_t m = 0; m < time.size(); m++) {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(1); in->set_time(time[m]);
		    handle_vtypes_vweights<true, true, false>(batch, meta_index, source[s], NULL, 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		} else {
		  EdgeInsertion * in = get_insertion(batch, meta_index);
		  in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		  in->set_weight(1); in->set_time(timestamp++);
		  handle_vtypes_vweights<true, true, false>(batch, meta_index, source[s], NULL, 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		}
	      }
	    }
	  }
	  for(int64_t s = 0; s < source_str.size(); s++) {
	    for(int64_t d = 0; d < destination.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = get_insertion(batch, meta_index);
		      in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		      handle_vtypes_vweights<true, false, true>(batch, meta_index, 0, source_str[s], destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		    }
		  } else {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		    handle_vtypes_vweights<true, false, true>(batch, meta_index, 0, source_str[s], destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		}
	      } else {
		if(time.size()) {
		  for(int64_t m = 0; m < time.size(); m++) {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		    in->set_weight(1); in->set_time(time[m]);
		    handle_vtypes_vweights<true, false, true>(batch, meta_index, 0, source_str[s], destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		} else {
		  EdgeInsertion * in = get_insertion(batch, meta_index);
		  in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		  in->set_weight(1); in->set_time(timestamp++);
		  handle_vtypes_vweights<true, false, true>(batch, meta_index, 0, source_str[s], destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		}
	      }
	    }
	    for(int64_t d = 0; d < destination_str.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = get_insertion(batch, meta_index);
		      in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		      handle_vtypes_vweights<true, false, false>(batch, meta_index, 0, source_str[s], 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		    }
		  } else {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		    handle_vtypes_vweights<true, false, false>(batch, meta_index, 0, source_str[s], 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		}
	      } else {
		if(time.size()) {
		  for(int64_t m = 0; m < time.size(); m++) {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(1); in->set_time(time[m]);
		    handle_vtypes_vweights<true, false, false>(batch, meta_index, 0, source_str[s], 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		} else {
		  EdgeInsertion * in = get_insertion(batch, meta_index);
		  in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		  in->set_weight(1); in->set_time(timestamp++);
		  handle_vtypes_vweights<true, false, false>(batch, meta_index, 0, source_str[s], 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		}
	      }
	    }
	  }
	}
	if(!type.size() && !type_str.size()) {
	  for(int64_t s = 0; s < source.size(); s++) {
	    for(int64_t d = 0; d < destination.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = get_insertion(batch, meta_index);
		      in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		      handle_vtypes_vweights<true, true, true>(batch, meta_index, source[s], NULL, destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		    }
		  } else {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		    handle_vtypes_vweights<true, true, true>(batch, meta_index, source[s], NULL, destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		}
	      } else {
		if(time.size()) {
		  for(int64_t m = 0; m < time.size(); m++) {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		    in->set_weight(1); in->set_time(time[m]);
		    handle_vtypes_vweights<true, true, true>(batch, meta_index, source[s], NULL, destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		} else {
		  EdgeInsertion * in = get_insertion(batch, meta_index);
		  in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		  in->set_weight(1); in->set_time(timestamp++);
		  handle_vtypes_vweights<true, true, true>(batch, meta_index, source[s], NULL, destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		}
	      }
	    }
	    for(int64_t d = 0; d < destination_str.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = get_insertion(batch, meta_index);
		      in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		      handle_vtypes_vweights<true, true, false>(batch, meta_index, source[s], NULL, 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		    }
		  } else {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		    handle_vtypes_vweights<true, true, false>(batch, meta_index, source[s], NULL, 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		}
	      } else {
		if(time.size()) {
		  for(int64_t m = 0; m < time.size(); m++) {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(1); in->set_time(time[m]);
		    handle_vtypes_vweights<true, true, false>(batch, meta_index, source[s], NULL, 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		} else {
		  EdgeInsertion * in = get_insertion(batch, meta_index);
		  in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		  in->set_weight(1); in->set_time(timestamp++);
		  handle_vtypes_vweights<true, true, false>(batch, meta_index, source[s], NULL, 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		}
	      }
	    }
	  }
	  for(int64_t s = 0; s < source_str.size(); s++) {
	    for(int64_t d = 0; d < destination.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = get_insertion(batch, meta_index);
		      in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		      handle_vtypes_vweights<true, false, true>(batch, meta_index, 0, source_str[s], destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		    }
		  } else {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		    handle_vtypes_vweights<true, false, true>(batch, meta_index, 0, source_str[s], destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		}
	      } else {
		if(time.size()) {
		  for(int64_t m = 0; m < time.size(); m++) {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		    in->set_weight(1); in->set_time(time[m]);
		    handle_vtypes_vweights<true, false, true>(batch, meta_index, 0, source_str[s], destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		} else {
		  EdgeInsertion * in = get_insertion(batch, meta_index);
		  in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		  in->set_weight(1); in->set_time(timestamp++);
		  handle_vtypes_vweights<true, false, true>(batch, meta_index, 0, source_str[s], destination[d], NULL, src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		}
	      }
	    }
	    for(int64_t d = 0; d < destination_str.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = get_insertion(batch, meta_index);
		      in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		      handle_vtypes_vweights<true, false, false>(batch, meta_index, 0, source_str[s], 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		    }
		  } else {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		    handle_vtypes_vweights<true, false, false>(batch, meta_index, 0, source_str[s], 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		}
	      } else {
		if(time.size()) {
		  for(int64_t m = 0; m < time.size(); m++) {
		    EdgeInsertion * in = get_insertion(batch, meta_index);
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(1); in->set_time(time[m]);
		    handle_vtypes_vweights<true, false, false>(batch, meta_index, 0, source_str[s], 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		  }
		} else {
		  EdgeInsertion * in = get_insertion(batch, meta_index);
		  in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		  in->set_weight(1); in->set_time(timestamp++);
		  handle_vtypes_vweights<true, false, false>(batch, meta_index, 0, source_str[s], 0, destination_str[d], src_type, src_weight, src_weight_incr, dest_type, dest_weight, dest_weight_incr);
		}
	      }
	    }
	  }
	}
      } break;

      case PATH_ORDERED: {
	int64_t stop = 0;
	stop = stop > source.size() ? stop : source.size();
	stop = stop < destination.size() ? stop : destination.size();

	for(int64_t e = 0; e < stop; e++) {
	  EdgeInsertion * in = get_insertion(batch, meta_index);

	  bool src_str = false;
	  bool dest_str = false;
	  bool src_int = false;
	  bool dest_int = false;

	  if(e < type.size()) {
	    in->set_type(type[e]);
	  } else if(e + type.size() < type_str.size()) {
	    in->set_type_str(type_str[e-type.size()]);
	  } else {
	    in->set_type(0);
	  }

	  if(e < source.size()) {
	    in->set_source(source[e]);
	    src_int = true;
	  } else if(e + source.size() < source_str.size()) {
	    in->set_source_str(source_str[e-source.size()]);
	    src_str = true;
	  } else {
	    in->set_source_str("CSV_PARSE_ERR_NO_SRC");
	  }

	  if(e < destination.size()) {
	    in->set_destination(destination[e]);
	    dest_int = true;
	  } else if(e + destination.size() < destination_str.size()) {
	    in->set_destination_str(destination_str[e-destination.size()]);
	    dest_str = true;
	  } else {
	    in->set_destination_str("CSV_PARSE_ERR_NO_DEST");
	  }

	  if(e < weight.size()) {
	    in->set_weight(weight[e]);
	  } else if(weight.size()) {
	    in->set_weight(weight[weight.size()-1]);
	  } else {
	    in->set_weight(1);
	  }

	  if(e < time.size()) {
	    in->set_time(time[e]);
	  } else if(time.size()) {
	    in->set_time(time[time.size()-1]);
	  } else {
	    in->set_time(timestamp++);
	  }

	  if(src_int && dest_int)
	    handle_vtypes_vweights<true, true, true>(batch, meta_index, source[e], NULL, destination[e], NULL, e, e, e, e, e, e);
	  if(src_int && dest_str)
	    handle_vtypes_vweights<true, true, false>(batch, meta_index, source[e], NULL, 0, destination_str[e], e, e, e, e, e, e);
	  if(src_str && dest_int)
	    handle_vtypes_vweights<true, false, true>(batch, meta_index, 0, source_str[e], destination[e], NULL, e, e, e, e, e, e);
	  if(src_str && dest_str)
	    handle_vtypes_vweights<true, false, false>(batch, meta_index, 0, source_str[e], 0, destination_str[e], e, e, e, e, e, e);
	}
      } break;
    }
    return 1;
  }
};

struct ExploreCSVValue : public ExploreCSVGeneric {
  value_type_t value_type;

  ExploreCSVValue(value_type_t type) : ExploreCSVGeneric(), value_type(type) {}

  virtual bool operator()(EdgeCollection & edges, char ** fields, int64_t * lengths, int64_t count, char * field) {
    LOG_D_A("Value type %ld", (long) value_type);
    switch(value_type) {
      case VALUE_TYPE:
	edges.type.push_back(atol(field));
	break;

      case VALUE_TYPE_STR:
	edges.type_str.push_back(field);
	break;

      case VALUE_SOURCE:
	edges.source.push_back(atol(field));
	break;

      case VALUE_SOURCE_STR:
	edges.source_str.push_back(field);
	break;

      case VALUE_SOURCE_TYPE:
	edges.source_type.push_back(field);
	break;

      case VALUE_SOURCE_WEIGHT:
	edges.source_weight.push_back(atol(field));
	break;

      case VALUE_SOURCE_WEIGHT_INCR:
	edges.source_weight_incr.push_back(atol(field));
	break;

      case VALUE_DESTINATION:
	edges.destination.push_back(atol(field));
	break;

      case VALUE_DESTINATION_STR:
	edges.destination_str.push_back(field);
	break;

      case VALUE_DESTINATION_TYPE:
	edges.destination_type.push_back(field);
	break;

      case VALUE_DESTINATION_WEIGHT:
	edges.destination_weight.push_back(atol(field));
	break;

      case VALUE_DESTINATION_WEIGHT_INCR:
	edges.destination_weight_incr.push_back(atol(field));
	break;

      case VALUE_WEIGHT:
	edges.weight.push_back(atol(field));
	break;

      case VALUE_TIME:
	edges.time.push_back(atol(field));
	break;

      default:
	LOG_E_A("Unknown type %ld", value_type);
	return false;
    }
    return true;
  }

  virtual void print() {
    printf("value %ld\n", (long)value_type);
  }

  virtual ExploreCSVGeneric * copy(path_type_t path) {
    LOG_D("Called");
    ExploreCSVValue * rtn = new ExploreCSVValue(value_type);
    if(child) {
      rtn->child = child->copy(path);
    } else {
      rtn->child = NULL;
    }
    LOG_D_A("Returning %p", rtn);
    return rtn;
  }
};

struct ExploreCSVRow : public ExploreCSVGeneric {
  int64_t index;

  ExploreCSVRow() : ExploreCSVGeneric(), index(0) {}

  virtual bool operator()(EdgeCollection & edges, char ** fields, int64_t * lengths, int64_t count, char * field) {
    if(index < count) {
      return (*child)(edges, fields, lengths, count, fields[index]);
    } else {
      LOG_W_A("skipping - Column index (%ld) is greater than number of columns (%ld)", index, count);
      return false;
    }
  }

  virtual void print() {
    if(index == -1)
      printf("@.");
    else
      printf("@.%ld.", index);
    if(child)
      child->print();
    else
      printf("no child\n");
  }

  virtual ExploreCSVGeneric * copy(path_type_t path) {
    LOG_D("Called");
    ExploreCSVRow * rtn = new ExploreCSVRow();
    rtn->index = index;

    if(child) {
      rtn->child = child->copy(path);
    } else {
      rtn->child = NULL;
    }
    LOG_D_A("Returning %p", rtn);
    return rtn;
  }
};

struct EdgeCollectionSet {
  std::vector<EdgeCollection *> set;
  int64_t time;

  EdgeCollectionSet() : time(0) {}

  ~EdgeCollectionSet() {
    for(int64_t e = 0; e < set.size(); e++) {
      if(set[e])
	delete set[e];
    }
  }

  EdgeCollection * get_collection(int64_t index) {
    while(index >= set.size()) {
      set.push_back(NULL);
    }

    return set[index];
  }

  void print() {
    for(int64_t index = 0; index < set.size(); index++) {
      if(set[index]) {
	set[index]->print();
      }
    }
  }

  EdgeCollection * set_collection(int64_t index, EdgeCollection * val) {
    while(index >= set.size()) {
      set.push_back(NULL);
    }

    return set[index] = val;
  }

  int64_t apply(StingerBatch & batch, char ** fields, int64_t * lengths, int64_t count, int64_t meta_index) {
    int64_t rtn = 0;

    LOG_D_A("about to apply %ld\n", set.size());
    for(int64_t index = 0; index < set.size(); index++) {
      if(set[index]) {
	LOG_D_A("applying %ld", index);
	rtn += set[index]->apply(batch, fields, lengths, count, time, meta_index);
      }
    }

    return rtn;
  }

  int64_t learn(char ** fields, int64_t * lengths, int64_t count, char * field = NULL, ExploreCSVGeneric * top = NULL, ExploreCSVGeneric * bottom = NULL) {
    LOG_D("Learning");
    int64_t rtn = 0;
    ExploreCSVGeneric * parent = bottom;

    if(!field) {
      LOG_D("Learning a row");
      if(top) {
	bottom->child = new ExploreCSVRow();
	bottom = bottom->child;
      } else {
	top = new ExploreCSVRow();
	bottom = top;
      }
      for (int64_t i = 0; i < count; i++) {
	(static_cast<ExploreCSVRow *>(bottom))->index = i;
	rtn += learn(fields, lengths, count, fields[i], top, bottom);
      }
    } else {
      const char * string = field;
      LOG_D_A("Learning a string %s", string);
      if(string[0] != '$') {
	return rtn;
      }
      string++;

      value_type_t type;
      if(0 == strncmp(string, "type", 4)) {
	string += 4;
	if(0 == strncmp(string, "_str", 4)) {
	  string += 4;
	  type = VALUE_TYPE_STR;
	} else {
	  type = VALUE_TYPE;
	}
      } else if(0 == strncmp(string, "source", 6)) {
	string += 6;
	if(0 == strncmp(string, "_str", 4)) {
	  type = VALUE_SOURCE_STR;
	  string += 4;
	} else if(0 == strncmp(string, "_type", 5)) {
	  type = VALUE_SOURCE_TYPE;
	  string += 5;
	} else if(0 == strncmp(string, "_weight_incr", 12)) {
	  type = VALUE_SOURCE_WEIGHT_INCR;
	  string += 12;
	} else if(0 == strncmp(string, "_weight", 7)) {
	  type = VALUE_SOURCE_WEIGHT;
	  string += 7;
	} else {
	  type = VALUE_SOURCE;
	}
      } else if(0 == strncmp(string, "destination", 11)) {
	string += 11;
	if(0 == strncmp(string, "_str", 4)) {
	  type = VALUE_DESTINATION_STR;
	  string += 4;
	} else if(0 == strncmp(string, "_type", 5)) {
	  type = VALUE_DESTINATION_TYPE;
	  string += 5;
	} else if(0 == strncmp(string, "_weight_incr", 12)) {
	  type = VALUE_DESTINATION_WEIGHT_INCR;
	  string += 12;
	} else if(0 == strncmp(string, "_weight", 7)) {
	  type = VALUE_DESTINATION_WEIGHT;
	  string += 7;
	} else {
	  type = VALUE_DESTINATION;
	}
      } else if(0 == strncmp(string, "weight", 6)) {
	string += 6;
	type = VALUE_WEIGHT;
      } else if(0 == strncmp(string, "time", 4)) {
	string += 4;
	type = VALUE_TIME;
      } else {
	LOG_W_A("Unknown type string: %s", string);
	return rtn;
      }

      char * end;
      int64_t index = strtol(string, &end, 0);
      EdgeCollection * col = get_collection(index);
      if(!col) {
	col = new EdgeCollection();
	set_collection(index, col);
      }
      string = end;

      path_type_t path = PATH_DEFAULT;
      if(0 == strncmp(string, "ordered", 7)) {
	path = PATH_ORDERED;
	string += 7;
      } else if(0 == strncmp(string, "exact", 5)) {
	path = PATH_EXACT;
	string += 5;
      }

      if(string[0] == ' ')
	string++;

      if(string[0] == '=') {
	string++;
	LOG_D_A("Found constant value %s", string);

	while(string[0] == ' ')
	  string++;

	switch(type) {
	  case VALUE_TYPE:
	    col->has_const_type = true;
	    col->const_type = atol(string);
	    break;

	  case VALUE_TYPE_STR:
	    col->has_const_type_str = true;
	    col->const_type_str = string;
	    break;

	  case VALUE_SOURCE:
	    col->has_const_source = true;
	    col->const_source = atol(string);
	    break;

	  case VALUE_SOURCE_STR:
	    col->has_const_source_str = true;
	    col->const_source_str = string;
	    break;

	  case VALUE_SOURCE_TYPE:
	    col->has_const_source_type = true;
	    col->const_source_type = string;
	    break;

	  case VALUE_SOURCE_WEIGHT:
	    col->has_const_source_weight = true;
	    col->const_source_weight = atol(string);
	    break;

	  case VALUE_DESTINATION:
	    col->has_const_destination = true;
	    col->const_destination = atol(string);
	    break;

	  case VALUE_DESTINATION_STR:
	    col->has_const_destination_str = true;
	    col->const_destination_str = string;
	    break;

	  case VALUE_DESTINATION_TYPE:
	    col->has_const_destination_type = true;
	    col->const_destination_type = string;
	    break;

	  case VALUE_DESTINATION_WEIGHT:
	    col->has_const_destination_weight = true;
	    col->const_destination_weight = atol(string);
	    break;

	  case VALUE_WEIGHT:
	    col->has_const_weight = true;
	    col->const_weight = atol(string);
	    break;

	  case VALUE_TIME:
	    col->has_const_time = true;
	    col->const_time = atol(string);
	    break;

	  default:
	    LOG_E("Unknown type");
	}
	return rtn;
      }

      if(top) {
	bottom->child = new ExploreCSVValue(type);
	bottom = bottom->child;
      } else {
	top = new ExploreCSVValue(type);
	bottom = top;
      }

      ExploreCSVGeneric * chain = top->copy(path);
      col->start.push_back(chain);
      col->path = path;
      rtn++;
    }

    if(parent)
      parent->child = NULL;
    delete bottom;

    return rtn;
  }
};

#undef LOG_AT_W

#endif  /*EXPLORE_CSV_H*/
