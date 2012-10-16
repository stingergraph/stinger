#ifndef  ARRAY_HPP
#define  ARRAY_HPP

#include <string.h>
#include <string>
#include "defs.hpp"
#include "util/parallel_primitives.hpp"

namespace Stinger {
  namespace Util {

    template<typename type_t>
    class Array {
      public:
	Array(size_t count);
	Array(const Array & a);
	~Array();

	size_t	      sizeBytes()	{ return sizeof(type_t) * elements; }
	size_t	      sizeElements()	{ return elements; }
	type_t *      rawPointer()	{ return array; }
	type_t &      operator[](int x)	{ return array[x]; }

      private:
	type_t *      array;
	size_t	      elements;
    };
  }
}

template<typename type_t>
Stinger::Util::Array<type_t>::Array(size_t count) {
  array = new type_t[count];
  stingerThrowIf(!array, "Allocation failed.");
  elements = count;
}

template<typename type_t>
Stinger::Util::Array<type_t>::~Array() {
  elements = 0;
  if(array)
    delete[] array;
}

template<typename type_t>
Stinger::Util::Array<type_t>::Array(const Array<type_t> & a) {
  Array(a.elements);
  copy(a.array, a.elements, array);
}

#endif  /*MAPPED_ARRAY_HPP*/
