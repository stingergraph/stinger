#ifndef _TWITTER_STREAM_H
#define _TWITTER_STREAM_H

#include <iostream>
#include <fstream>
#include <istream>
#include <cstdio>
#include <string>
#include <vector>
#include <list>
#include "stinger_net/proto/stinger-batch.pb.h"
#include "stinger_net/send_rcv.h"

using namespace gt::stinger;

static const char* kTypeNames[] = { "Null", "False", "True", "Object", "Array", "String", "Number" };

#define SOURCE_VTX 0
#define TARGET_VTX 1
#define EDGE_WEIGHT 2
#define EDGE_TIME 3
#define EDGE_TYPE 4

void
print_list (std::list<std::string> l);

int
train_describe_object(rapidjson::Document& document, std::list<std::string> breadcrumbs, std::map<int, std::list<std::string> > &found, int level);

int
train_describe_array(rapidjson::Value& array, std::list<std::string> breadcrumbs, std::map<int, std::list<std::string> > &found, int level);

int
load_template_file (char * filename, char delimiter, std::map<int, std::list<std::string> > &found);

int
test_describe_object(rapidjson::Document& document, std::list<std::string> breadcrumbs, const std::map<int, std::list<std::string> > &found, int level);

int
test_describe_array(rapidjson::Value& array, std::list<std::string> breadcrumbs, const std::map<int, std::list<std::string> > &found, int level);

#endif /* _TWITTER_STREAM_H */
