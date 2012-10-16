#ifndef  MAPPED_ARRAY_HPP
#define  MAPPED_ARRAY_HPP

#include "util/array.hpp"

namespace Stinger {
  namespace Util {

    template<typename type_t>
    class MappedArray : public Array<type_t> {
      public:
	MappedArray(size_t count);
	MappedArray(const char * name);
	MappedArray(const MappedArray & a);
	~MappedArray();

	const char *  name()		{ return map_name; }

      private:
	const char *  map_name;
	bool is_copy;
    };
  }
}

#endif  /*MAPPED_ARRAY_HPP*/
