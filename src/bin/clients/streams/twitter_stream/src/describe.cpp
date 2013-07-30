#include <cstdio>

#include "rapidjson/document.h"
#include "rapidjson/filestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "twitter_stream.h"

/*
struct EdgeCollection {
  std::vector<int64_t> type;
  std::vector<std::string> type_str;
  std::vector<int64_t> source;
  std::vector<std::string> source_str;
  std::vector<int64_t> destination;
  std::vector<std::string> destination_str;
  std::vector<int64_t> weight;
  std::vector<int64_t> time;
};

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

struct ExploreJSONGeneric {
  ExploreJSONGeneric & child = NULL;

  bool operator()(EdgeCollection & edges, rapidjson::Document & document);
  void print();
};

struct ExploreJSONValue : ExploreJSONGeneric{
  value_type_t value_type;

  ExploreJSONValue(value_type_t type) : value_type(type) {}

  bool operator()(EdgeCollection & edges, rapidjson::Document & document) {
    switch(value_type) {
      case VALUE_TYPE:
	edges.type.append(document.GetInt64());
	break;

      case VALUE_TYPE_STR:
	edges.type_str.append(std::string(document.GetString(), document.GetStringLength());
	break;

      case VALUE_SOURCE:
	edges.source.append(document.GetInt64());
	break;

      case VALUE_SOURCE_STR:
	edges.source_str.append(std::string(document.GetString(), document.GetStringLength());
	break;

      case VALUE_DESTINATION:
	edges.destination.append(document.GetInt64());
	break;

      case VALUE_DESTINATION_STR:
	edges.destination_str.append(std::string(document.GetString(), document.GetStringLength());
	break;

      case VALUE_WEIGHT:
	edges.weight.append(document.GetInt64());
	break;

      case VALUE_TIME:
	edges.time.append(document.GetInt64());
	break;

      default:
	return false;
    }
    return true;
  }

  void print() {
    printf(".value %ld", value_type);
    child.print();
  }
};

struct ExploreJSONArray : ExploreJSONGeneric {
  ExploreJSONArray() : child = NULL {}

  bool operator()(EdgeCollection & edges, rapidjson::Document & document) {
    if(document.IsArray()) {
      for(rapidjson::SizeType i = 0; i < document.Size(); i++) {
	return child(edges, document[i]);
      }
    } else {
      return false;
    }
  }

  void print() {
    printf(".@.");
    child.print();
  }
};

struct ExploreJSONObject : ExploreJSONGeneric {
  std::string field_name;

  ExploreJSONObject(std::string & field) : child = NULL, field_name(field) {}

  bool operator()(EdgeCollection & edges, rapidjson::Document & document) {
    if(document.HasMember(field_name.c_str()))
      return child(edges, document[field_name.c_str()]);
    else
      return false;
  }

  void print() {
    printf(".$.%s", field_name.c_str());
    child.print();
  }
};

*/
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

