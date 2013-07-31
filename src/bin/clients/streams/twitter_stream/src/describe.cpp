#include <cstdio>

#include "rapidjson/document.h"
#include "rapidjson/filestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "twitter_stream.h"
#include "stinger_net/proto/stinger-batch.pb.h"

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
  ~ExploreJSONGeneric() { delete child; }

  virtual bool operator()(EdgeCollection & edges, rapidjson::Value & document);
  virtual void print();
  virtual ExploreJSONGeneric * copy(path_type_t path);
};

struct EdgeCollection {
  std::vector<ExploreJSONGeneric *> start;
  path_type_t path;

  std::vector<int64_t> type;
  std::vector<std::string> type_str;
  std::vector<int64_t> source;
  std::vector<std::string> source_str;
  std::vector<int64_t> destination;
  std::vector<std::string> destination_str;
  std::vector<int64_t> weight;
  std::vector<int64_t> time;

  int64_t apply(StingerBatch & batch, rapidjson::Value & document, int64_t & timestamp) {
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
/*	int64_t max = 0;
	max = max > type.size() ? max : type.size();
	max = max > type_str.size() ? max : type_str.size();
	max = max > source.size() ? max : source.size();
	max = max > source_str.size() ? max : source_str.size();
	max = max > destination.size() ? max : destination.size();
	max = max > destination_str.size() ? max : destination_str.size();
	for(int64_t e = 0; e < max; e++) {
	  EdgeInsertion * in = batch.add_insertions();
	  if(e < type.size()) {
	    if(e <
	  } else if(e < type_str.size()) {
	  } else {
	  }
	}*/
	/* TODO */
      } break;
    }
  }
};

struct EdgeCollectionSet {
  std::vector<EdgeCollection *> set;
  int64_t time;

  EdgeCollectionSet() : time(0) {}

  EdgeCollection * get_collection(int64_t index) {
    while(index > set.size()) {
      set.push_back(NULL);
    }

    return set[index];
  }

  EdgeCollection * set_collection(int64_t index, EdgeCollection * val) {
    while(index > set.size()) {
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

  int64_t learn(rapidjson::Value & document) {
    /* TODO */
  }
};

struct ExploreJSONValue : public ExploreJSONGeneric {
  value_type_t value_type;

  ExploreJSONValue(value_type_t type) : ExploreJSONGeneric(), value_type(type) {}

  virtual bool operator()(EdgeCollection & edges, rapidjson::Value & document) {
    switch(value_type) {
      case VALUE_TYPE:
	edges.type.push_back(document.GetInt64());
	break;

      case VALUE_TYPE_STR:
	edges.type_str.push_back(std::string(document.GetString(), document.GetStringLength()));
	break;

      case VALUE_SOURCE:
	edges.source.push_back(document.GetInt64());
	break;

      case VALUE_SOURCE_STR:
	edges.source_str.push_back(std::string(document.GetString(), document.GetStringLength()));
	break;

      case VALUE_DESTINATION:
	edges.destination.push_back(document.GetInt64());
	break;

      case VALUE_DESTINATION_STR:
	edges.destination_str.push_back(std::string(document.GetString(), document.GetStringLength()));
	break;

      case VALUE_WEIGHT:
	edges.weight.push_back(document.GetInt64());
	break;

      case VALUE_TIME:
	edges.time.push_back(document.GetInt64());
	break;

      default:
	return false;
    }
    return true;
  }

  virtual void print() {
    printf(".value %ld", (long)value_type);
  }

  virtual ExploreJSONGeneric * copy(path_type_t path) {
    ExploreJSONValue * rtn = new ExploreJSONValue(value_type);
    rtn->child = child->copy(path);
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
    printf(".@.");
    child->print();
  }

  virtual ExploreJSONGeneric * copy(path_type_t path) {
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

    rtn->child = child->copy(path);
    return rtn;
  }
};

struct ExploreJSONObject : public ExploreJSONGeneric {
  std::string field_name;

  ExploreJSONObject(std::string & field) : ExploreJSONGeneric(), field_name(field) {}

  virtual bool operator()(EdgeCollection & edges, rapidjson::Value & document) {
    if(document.HasMember(field_name.c_str()))
      return (*child)(edges, document[field_name.c_str()]);
    else
      return false;
  }

  virtual void print() {
    printf(".$.%s", field_name.c_str());
    child->print();
  }

  virtual ExploreJSONGeneric * copy(path_type_t path) {
    ExploreJSONObject * rtn = new ExploreJSONObject(field_name);
    rtn->child = child->copy(path);
    return rtn;
  }
};

void
print_list (std::list<std::string> l)
{
  for (std::list<std::string>::const_iterator iterator = l.begin(), end = l.end(); iterator != end; ++iterator) {
    std::cout << *iterator << " -> ";
  }
  std::cout << std::endl;
}

int
list_diff (const std::list<std::string> &a, const std::list<std::string> &b)
{
  if (a.empty() || b.empty())
    return 0;

//  printf("\n\nstart diff\n");
  for (std::list<std::string>::const_iterator it_a = a.begin(), it_b = b.begin();
	it_a != a.end() && it_b != b.end(); it_a++, it_b++) {
//    printf("a: %s\t b: %s", (*it_a).c_str(), (*it_b).c_str());
    if ( strcmp((*it_a).c_str(), (*it_b).c_str()) != 0 ) {
//      printf("\nreturn 0\n");
      return 0;
    }
//    printf("\n");
  }

  if (a.size() == b.size()) {
//    printf("return 2\n");
    return 2;
  }
  else {
//    printf("return 1\n");
    return 1;
  }
}

int
train_describe_object(rapidjson::Document& document, std::list<std::string> &breadcrumbs, std::map<int, std::list<std::string> > &found, int level)
{
  assert(document.IsObject());

  for (rapidjson::Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr) {
    breadcrumbs.push_back(itr->name.GetString());
//    print_list(breadcrumbs);
//    printf("Type of member %s is %s\n", itr->name.GetString(), kTypeNames[itr->value.GetType()]);
    if (itr->value.GetType() == 3) { /* object */
      rapidjson::StringBuffer sb;
      rapidjson::Writer<rapidjson::StringBuffer> writer (sb);
      itr->value.Accept(writer);
      rapidjson::Document obj;
      obj.Parse<0>(sb.GetString());
      train_describe_object(obj, breadcrumbs, found, level+1);
    }
    else if (itr->value.GetType() == 4) { /* array */
      train_describe_array((rapidjson::Value&) itr->value, breadcrumbs, found, level+1);
    }
    else if (itr->value.GetType() == 5) { /* string */
      assert(itr->value.IsString());
      //printf("%s is %s\n", itr->name.GetString(), itr->value.GetString());
      if ( strncmp(itr->value.GetString(),"$STINGER SOURCE VERTEX$", 23) == 0 ) {
	found[SOURCE_VTX] = breadcrumbs;
      }
      if ( strncmp(itr->value.GetString(),"$STINGER TARGET VERTEX$", 23) == 0 ) {
	found[TARGET_VTX] = breadcrumbs;
      }
      if ( strncmp(itr->value.GetString(),"$STINGER EDGE WEIGHT$", 21) == 0 ) {
	found[EDGE_WEIGHT] = breadcrumbs;
      }
      if ( strncmp(itr->value.GetString(),"$STINGER EDGE TIME$", 19) == 0 ) {
	found[EDGE_TIME] = breadcrumbs;
      }
      if ( strncmp(itr->value.GetString(),"$STINGER EDGE TYPE$", 19) == 0 ) {
	found[EDGE_TYPE] = breadcrumbs;
      }
    }
    else if (itr->value.GetType() == 6) {
      assert(itr->value.IsNumber());
    }
    breadcrumbs.pop_back();
  }

//  printf("\n");
  return 0;
}

int
train_describe_array(rapidjson::Value& array, std::list<std::string> &breadcrumbs, std::map<int, std::list<std::string> > &found, int level)
{
  assert(array.IsArray());

  for (rapidjson::SizeType i = 0; i < array.Size(); i++) {

    if (array[i].GetType() == 3) { /* object */
      rapidjson::StringBuffer sb;
      rapidjson::Writer<rapidjson::StringBuffer> writer (sb);
      array[i].Accept(writer);
      rapidjson::Document obj;
      obj.Parse<0>(sb.GetString());
      train_describe_object(obj, breadcrumbs, found, level+1);
    }
    else if (array[i].GetType() == 4) { /* array */
      train_describe_array(array[i], breadcrumbs, found, level+1);
    }
    else if (array[i].GetType() == 5) { /* string */
    }
    else if (array[i].GetType() == 6) { /* number */
    }
  }

//  printf("\n");

  return 0;
}

int
load_template_file (char * filename, char delimiter, std::map<int, std::list<std::string> > &found)
{
  std::ifstream fp;
  fp.open (filename);

  if (!fp.is_open()) {
    char errmsg[257];
    snprintf (errmsg, 256, "Opening \"%s\" failed", filename);
    errmsg[256] = 0;
    perror (errmsg);
    exit (-1);
  } 
  
  std::list<std::string> breadcrumbs;

  /* read the lines */
  while (!fp.eof())
  {
    rapidjson::Document document;
    
    std::string line;
    std::getline (fp, line, delimiter);
    document.Parse<0>(line.c_str());

    train_describe_object (document, breadcrumbs, found, 0);

  }

  fp.close();

  return 0;

}

int
test_describe_object(rapidjson::Document& document, std::list<std::string> &breadcrumbs, const std::map<int, std::list<std::string> > &found, int level)
{
  assert(document.IsObject());

  for (rapidjson::Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr) {
    breadcrumbs.push_back(itr->name.GetString());

    int flag = -1;
    for (int64_t i = 0; i < 5; i++) {
      int rtn = list_diff (breadcrumbs, found.at(i));
      if (rtn == 1)
	flag = 1000;   /* continue down this branch of the tree */
      if (rtn == 2) {
	flag = i;      /* this is something we are looking for; code set */
	break;
      }
    }

    const rapidjson::Value& val = itr->value;
    int type = val.GetType();
    if (flag == 1000) {  /* continue down this branch */
      //print_list(breadcrumbs);
      if (type == 3) { /* object */
	rapidjson::StringBuffer sb;
	rapidjson::Writer<rapidjson::StringBuffer> writer (sb);
	val.Accept(writer);
	rapidjson::Document obj;
	obj.Parse<0>(sb.GetString());
	test_describe_object(obj, breadcrumbs, found, level+1);
      }
      else if (type == 4) { /* array */
	test_describe_array((rapidjson::Value&) val, breadcrumbs, found, level+1);
      }
    }
    else if (flag >= 0)   /* this is something we are looking for */
    {
      if (val.GetType() == 5) { /* string */
	assert(val.IsString());
	//printf("FOUND: %d, %s\n", flag, itr->value.GetString());
      }
      else if (val.GetType() == 6) { /* number */
	assert(val.IsNumber());
	assert(val.IsInt64());
	//printf("FOUND: %d, %ld\n", flag, itr->value.GetInt64());
      }
    }

    breadcrumbs.pop_back();
  }

  return 0;
}

int
test_describe_array(rapidjson::Value& array, std::list<std::string> &breadcrumbs, const std::map<int, std::list<std::string> > &found, int level)
{
  assert(array.IsArray());

  for (rapidjson::SizeType i = 0; i < array.Size(); i++) {

    if (array[i].GetType() == 3) { /* object */
      rapidjson::StringBuffer sb;
      rapidjson::Writer<rapidjson::StringBuffer> writer (sb);
      array[i].Accept(writer);
      rapidjson::Document obj;
      obj.Parse<0>(sb.GetString());
      test_describe_object(obj, breadcrumbs, found, level+1);
    }
    else if (array[i].GetType() == 4) { /* array */
      test_describe_array(array[i], breadcrumbs, found, level+1);
    }
    else if (array[i].GetType() == 5) { /* string */
    }
    else if (array[i].GetType() == 6) { /* number */
    }
  }


  return 0;
}

