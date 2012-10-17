#ifndef  RETURN_HPP
#define  RETURN_HPP

#include "defs.hpp"

namespace Stinger {
  namespace Core {
    #define ENUM_ELEMENTS()  \
      ENUM(STINGER_SUCCESS),  \
      ENUM(STINGER_FAILURE),  \
      ENUM(STINGER_MAX)

    #define ENUM(X) X
    enum StingerReturn {
      ENUM_ELEMENTS()
    };
    #undef ENUM(X)

    #define ENUM(X) "#X"
    const char * StingerReturnStrings[] = {
      ENUM_ELEMENTS()
    };
    #undef ENUM(X)
  }
}

#endif  /*RETURN_HPP*/
