#ifndef  ASSOCIATIVE_ARRAY_HPP
#define  ASSOCIATIVE_ARRAY_HPP

// Types supported
// (u)int64_t, double, string, array, associative_array

namespace Stinger {
  namespace Util {
    class AssociativeArray {
      template<typename in_t, typename out_t>
      out_t & operator[] (int_t &index) {
	return *(out_t *)(arr + xorHashAndMix64(index));
      }

      private:
	void * arr = NULL;
    };
  }
}

#endif  /*ASSOCIATIVE_ARRAY_HPP*/
