#ifndef _ALG_DATA_TO_JSON_H
#define _ALG_DATA_TO_JSON_H

template <typename T>
struct compare_pair_desc {
  compare_pair_desc (T * vals) : _vals(vals) {}
  bool operator()(int64_t a, int64_t b) {
    if (_vals[a] > _vals[b])
      return true;
    else
      return false;
  }

private:
  T * _vals;
};


rapidjson::Document *
description_string_to_json (char * description_string);

rapidjson::Document *
array_to_json_range (char * description_string, int64_t nv, uint8_t * data, char * search_string,
		     int64_t start, int64_t end);

rapidjson::Document *
array_to_json_sorted_range (char * description_string, int64_t nv, uint8_t * data, char * search_string,
		     int64_t start, int64_t end);

rapidjson::Document *
array_to_json_set (char * description_string, int64_t nv, uint8_t * data, char * search_string,
		     int64_t * set);

rapidjson::Document *
array_to_json (char * description_string, int64_t nv, uint8_t * data, char * search_string);


#endif /* _ALG_DATA_TO_JSON_H */
