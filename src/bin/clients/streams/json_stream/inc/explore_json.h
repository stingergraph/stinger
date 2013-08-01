#ifndef  EXPLORE_JSON_H
#define  EXPLORE_JSON_H

#include "rapidjson/document.h"
#include "stinger_net/proto/stinger-batch.pb.h"
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;

typedef enum {
  VALUE_TYPE,
  VALUE_TYPE_STR,
  VALUE_SOURCE,
  VALUE_SOURCE_STR,
  VALUE_DESTINATION,
  VALUE_DESTINATION_STR,
  VALUE_WEIGHT,
  VALUE_TIME
} value_type_t;

typedef enum {
  PATH_DEFAULT,
  PATH_EXACT,
  PATH_ORDERED
} path_type_t;

struct EdgeCollection;

struct ExploreJSONGeneric {
  ExploreJSONGeneric * child;
  ExploreJSONGeneric() : child(NULL) {}
  ~ExploreJSONGeneric() { if (child) delete child; }

  virtual bool operator()(EdgeCollection & edges, rapidjson::Value & document) { LOG_E("Error, this is a generic object"); }
  virtual void print() { LOG_E("Error, this is a generic object"); }
  virtual ExploreJSONGeneric * copy(path_type_t path) { LOG_E("Error, this is a generic object"); }
};

struct EdgeCollection {
  std::vector<ExploreJSONGeneric *> start;
  path_type_t path;

  EdgeCollection() : path(PATH_DEFAULT) {}

  ~EdgeCollection() { 
    for(int64_t s = 0; s < start.size(); s++) {
      if(start[s])
	delete start[s];
    }
  }

  std::vector<int64_t> type;
  std::vector<std::string> type_str;
  std::vector<int64_t> source;
  std::vector<std::string> source_str;
  std::vector<int64_t> destination;
  std::vector<std::string> destination_str;
  std::vector<int64_t> weight;
  std::vector<int64_t> time;

  void print() {
    for(int64_t s = 0; s < start.size(); s++) {
      if(start[s])
	start[s]->print();
    }
  }

  int64_t apply(StingerBatch & batch, rapidjson::Value & document, int64_t & timestamp) {
    type.clear();
    type_str.clear();
    source.clear();
    source_str.clear();
    destination.clear();
    destination_str.clear();
    weight.clear();
    time.clear();

    /* TODO figure out what to do about missing fields */
    for(int64_t i = 0; i < start.size(); i++) {
      if(!(*start[i])(*this, document)) {
	return 0;
      }
    }
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
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(type[t]); in->set_source(source[s]); in->set_destination(destination[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(type[t]); in->set_source(source[s]); in->set_destination(destination[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		  }
		}
	      } else {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(type[t]); in->set_source(source[s]); in->set_destination(destination[d]);
		      in->set_weight(1); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(type[t]); in->set_source(source[s]); in->set_destination(destination[d]);
		    in->set_weight(1); in->set_time(timestamp++);
		  }
	      }
	    }
	    for(int64_t d = 0; d < destination_str.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(type[t]); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(type[t]); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		  }
		}
	      } else {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(type[t]); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(1); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(type[t]); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(1); in->set_time(timestamp++);
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
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		  }
		}
	      } else {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		      in->set_weight(1); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		    in->set_weight(1); in->set_time(timestamp++);
		  }
	      }
	    }
	    for(int64_t d = 0; d < destination_str.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		  }
		}
	      } else {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(1); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(type[t]); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(1); in->set_time(timestamp++);
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
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		  }
		}
	      } else {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		      in->set_weight(1); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		    in->set_weight(1); in->set_time(timestamp++);
		  }
	      }
	    }
	    for(int64_t d = 0; d < destination_str.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		  }
		}
	      } else {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(1); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(1); in->set_time(timestamp++);
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
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		  }
		}
	      } else {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		      in->set_weight(1); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		    in->set_weight(1); in->set_time(timestamp++);
		  }
	      }
	    }
	    for(int64_t d = 0; d < destination_str.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		  }
		}
	      } else {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(1); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(1); in->set_time(timestamp++);
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
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		  }
		}
	      } else {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		      in->set_weight(1); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source(source[s]); in->set_destination(destination[d]);
		    in->set_weight(1); in->set_time(timestamp++);
		  }
	      }
	    }
	    for(int64_t d = 0; d < destination_str.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		  }
		}
	      } else {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(1); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source(source[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(1); in->set_time(timestamp++);
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
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		  }
		}
	      } else {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		      in->set_weight(1); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination(destination[d]);
		    in->set_weight(1); in->set_time(timestamp++);
		  }
	      }
	    }
	    for(int64_t d = 0; d < destination_str.size(); d++) {
	      if(weight.size()) {
		for(int64_t w = 0; w < weight.size(); w++) {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(weight[w]); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(weight[w]); in->set_time(timestamp++);
		  }
		}
	      } else {
		  if(time.size()) {
		    for(int64_t m = 0; m < time.size(); m++) {
		      EdgeInsertion * in = batch.add_insertions();
		      in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		      in->set_weight(1); in->set_time(time[m]);
		    }
		  } else {
		    EdgeInsertion * in = batch.add_insertions();
		    in->set_type(0); in->set_source_str(source_str[s]); in->set_destination_str(destination_str[d]);
		    in->set_weight(1); in->set_time(timestamp++);
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
	  EdgeInsertion * in = batch.add_insertions();
	  if(e < type.size()) {
	    in->set_type(type[e]);
	  } else if(e + type.size() < type_str.size()) {
	    in->set_type_str(type_str[e-type.size()]);
	  } else {
	    in->set_type(0);
	  }

	  if(e < source.size()) {
	    in->set_source(source[e]);
	  } else if(e + source.size() < source_str.size()) {
	    in->set_source_str(source_str[e-source.size()]);
	  }

	  if(e < destination.size()) {
	    in->set_destination(destination[e]);
	  } else if(e + destination.size() < destination_str.size()) {
	    in->set_destination_str(destination_str[e-destination.size()]);
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
	}
      } break;
    }
  }
};

struct ExploreJSONValue : public ExploreJSONGeneric {
  value_type_t value_type;

  ExploreJSONValue(value_type_t type) : ExploreJSONGeneric(), value_type(type) {}

  virtual bool operator()(EdgeCollection & edges, rapidjson::Value & document) {
    LOG_D_A("Value type %ld", value_type);
    switch(value_type) {
      case VALUE_TYPE:
	if(document.IsInt64())
	  edges.type.push_back(document.GetInt64());
	break;

      case VALUE_TYPE_STR:
	if(document.IsString())
	  edges.type_str.push_back(std::string(document.GetString(), document.GetStringLength()));
	break;

      case VALUE_SOURCE:
	if(document.IsInt64())
	  edges.source.push_back(document.GetInt64());
	break;

      case VALUE_SOURCE_STR:
	if(document.IsString())
	  edges.source_str.push_back(std::string(document.GetString(), document.GetStringLength()));
	break;

      case VALUE_DESTINATION:
	if(document.IsInt64())
	  edges.destination.push_back(document.GetInt64());
	break;

      case VALUE_DESTINATION_STR:
	if(document.IsString())
	  edges.destination_str.push_back(std::string(document.GetString(), document.GetStringLength()));
	break;

      case VALUE_WEIGHT:
	if(document.IsInt64())
	  edges.weight.push_back(document.GetInt64());
	break;

      case VALUE_TIME:
	if(document.IsInt64())
	  edges.time.push_back(document.GetInt64());
	break;

      default:
	return false;
    }
    return true;
  }

  virtual void print() {
    printf("value %ld\n", (long)value_type);
  }

  virtual ExploreJSONGeneric * copy(path_type_t path) {
    LOG_D("Called");
    ExploreJSONValue * rtn = new ExploreJSONValue(value_type);
    if(child) {
      rtn->child = child->copy(path);
    } else {
      rtn->child = NULL;
    }
    LOG_D_A("Returning %p", rtn);
    return rtn;
  }
};

struct ExploreJSONArray : public ExploreJSONGeneric {
  rapidjson::SizeType index;

  ExploreJSONArray() : ExploreJSONGeneric(), index(0) {}

  virtual bool operator()(EdgeCollection & edges, rapidjson::Value & document) {
    if(document.IsArray()) {
      if(index == -1) {
	for(rapidjson::SizeType i = 0; i < document.Size(); i++) {
	  return (*child)(edges, document[i]);
	}
      } else {
	  return (*child)(edges, document[index]);
      }
    } else {
      return false;
    }
  }

  virtual void print() {
    if(index == -1)
      printf("@.");
    else
      printf("@.%ld.", index);
    child->print();
  }

  virtual ExploreJSONGeneric * copy(path_type_t path) {
    LOG_D("Called");
    ExploreJSONArray * rtn = new ExploreJSONArray();
    switch(path) {
      default:
      case PATH_ORDERED:
      case PATH_DEFAULT:
       rtn->index = -1;
       break;
      case PATH_EXACT:
       rtn->index = index;
       break;
    }

    if(child) {
      rtn->child = child->copy(path);
    } else {
      rtn->child = NULL;
    }
    LOG_D_A("Returning %p", rtn);
    return rtn;
  }
};

struct ExploreJSONObject : public ExploreJSONGeneric {
  std::string field_name;

  ExploreJSONObject() : ExploreJSONGeneric(), field_name("") {}

  virtual bool operator()(EdgeCollection & edges, rapidjson::Value & document) {
    if(!document.IsObject())
      LOG_E_A("Document isn't object - field: %s", field_name.c_str());
    if(document.HasMember(field_name.c_str()))
      return (*child)(edges, document[field_name.c_str()]);
    else
      return false;
  }

  virtual void print() {
    printf("$.%s.", field_name.c_str());
    child->print();
  }

  virtual ExploreJSONGeneric * copy(path_type_t path) {
    LOG_D("Called");
    ExploreJSONObject * rtn = new ExploreJSONObject();
    rtn->field_name = field_name;
    rtn->child = child->copy(path);
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

  int64_t apply(StingerBatch & batch, rapidjson::Value & document) {
    int64_t rtn = 0;

    for(int64_t index = 0; index < set.size(); index++) {
      if(set[index]) {
	rtn += set[index]->apply(batch, document, time);
      }
    }

    return rtn;
  }

  int64_t learn(const rapidjson::Value & document, ExploreJSONGeneric * top = NULL, ExploreJSONGeneric * bottom = NULL) {
    LOG_D("Learning");
    int64_t rtn = 0;
    ExploreJSONGeneric * parent = bottom;

    if(document.IsObject()) {
      LOG_D("Learning an object");
      if(top) {
	bottom->child = new ExploreJSONObject();
	bottom = bottom->child;
      } else {
	top = new ExploreJSONObject();
	bottom = top;
      }
      for (rapidjson::Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr) {
	(static_cast<ExploreJSONObject *>(bottom))->field_name = itr->name.GetString();
	rtn += learn(itr->value, top, bottom);
      }
    } else if(document.IsArray()) {
      LOG_D("Learning an array");
      if(top) {
	bottom->child = new ExploreJSONArray();
	bottom = bottom->child;
      } else {
	top = new ExploreJSONArray();
	bottom = top;
      }
      for (int64_t i = 0; i < document.Size(); i++) {
	(static_cast<ExploreJSONArray *>(bottom))->index = i;
	rtn += learn(document[i], top, bottom);
      }
    } else if(document.IsString()) {
      const char * string = document.GetString();
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
	} else {
	  type = VALUE_SOURCE;
	}
      } else if(0 == strncmp(string, "destination", 11)) {
	string += 11;
	if(0 == strncmp(string, "_str", 4)) {
	  type = VALUE_DESTINATION_STR;
	  string += 4;
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

      LOG_D_A("String %s end %s", string, end);
      string = end;
      if(top) {
	bottom->child = new ExploreJSONValue(type);
	bottom = bottom->child;
      } else {
	top = new ExploreJSONValue(type);
	bottom = top;
      }

      path_type_t path = PATH_DEFAULT;
      if(0 == strncmp(string, "ordered", 7)) {
	path = PATH_ORDERED;
      } else if(0 == strncmp(string, "exact", 5)) {
	path = PATH_EXACT;
      }

      ExploreJSONGeneric * chain = top->copy(path);
      col->start.push_back(chain);
      col->path = path;
      rtn++;
    } else {
      return rtn;
    }

    if(parent)
      parent->child = NULL;
    delete bottom;

    return rtn;
  }
};

#endif  /*EXPLORE_JSON_H*/
