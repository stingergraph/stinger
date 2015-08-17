#ifndef  STINGER_STREAM_STATE_H
#define  STINGER_STREAM_STATE_H

#include <string>

namespace gt {
  namespace stinger {

    /**
    * @brief Struct to handle incoming stream state and configuration.
    * Data should be handled through the server state.
    */
    struct StingerStreamState {
      std::string name;
    };

  } /* gt */
} /* stinger */

#endif  /*STINGER_STREAM_STATE_H*/
