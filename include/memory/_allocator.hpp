#ifndef  ALLOCATOR_HPP
#define  ALLOCATOR_HPP

namespace Stinger {
  namespace Memory {
    class Allocator {
      uint8_t * pool = NULL;
      Allocator(size_t space, size_t min) {
	pool = malloc(space);
      }
    };
  }
}

#endif  /*ALLOCATOR_HPP*/
