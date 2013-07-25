#include <cstdio>

#include "rapidjson/document.h"
#include "rapidjson/filestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "twitter_stream.h"

void
print_list (std::list<std::string> l)
{
  for (std::list<std::string>::const_iterator iterator = l.begin(), end = l.end(); iterator != end; ++iterator) {
    std::cout << *iterator << " -> ";
  }
  std::cout << std::endl;
}

int
list_diff (std::list<std::string> a, std::list<std::string> b)
{
  for (std::list<std::string>::const_iterator it_a = a.begin(), it_b = b.begin(), end_a = a.end(), end_b = b.end();
	it_a != end_a, it_b != end_b; ++it_a, ++it_b) {
    if (*it_a != *it_b)
      return 0;
  }
  return 1;
}

int
describe_object(rapidjson::Document& document, std::list<std::string> breadcrumbs, std::map<int, std::list<std::string> > found, int level)
{
  assert(document.IsObject());

  for (rapidjson::Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr) {
    breadcrumbs.push_back(itr->name.GetString());
    print_list(breadcrumbs);
    printf("Type of member %s is %s\n", itr->name.GetString(), kTypeNames[itr->value.GetType()]);
    if (itr->value.GetType() == 3) { /* object */
      rapidjson::StringBuffer sb;
      rapidjson::Writer<rapidjson::StringBuffer> writer (sb);
      itr->value.Accept(writer);
      rapidjson::Document obj;
      obj.Parse<0>(sb.GetString());
      describe_object(obj, breadcrumbs, found, level+1);
    }
    else if (itr->value.GetType() == 4) { /* array */
      describe_array((rapidjson::Value&) itr->value, breadcrumbs, found, level+1);
    }
    else if (itr->value.GetType() == 5) { /* string */
      printf("value: %S\n", itr->value.GetString());
      if (itr->value.GetString() == "$STINGER$") {
	found[0] = breadcrumbs;
	printf("found it!\n");
      }
    }
    breadcrumbs.pop_back();
  }

  printf("\n");
  return 0;
}

int
describe_array(rapidjson::Value& array, std::list<std::string> breadcrumbs, std::map<int, std::list<std::string> > found, int level)
{
  assert(array.IsArray());

  for (rapidjson::SizeType i = 0; i < array.Size(); i++) {

    if (array[i].GetType() == 3) { /* object */
      rapidjson::StringBuffer sb;
      rapidjson::Writer<rapidjson::StringBuffer> writer (sb);
      array[i].Accept(writer);
      rapidjson::Document obj;
      obj.Parse<0>(sb.GetString());
      describe_object(obj, breadcrumbs, found, level+1);
    }
    else if (array[i].GetType() == 4) { /* array */
      describe_array(array[i], breadcrumbs, found, level+1);
    }
    else if (array[i].GetType() == 5) { /* string */
    }
    else if (array[i].GetType() == 6) { /* number */
    }
  }

  printf("\n");

  return 0;
}

