#ifndef  STINGER_MON_STATE_H
#define  STINGER_MON_STATE_H

#include <string>
#include <vector>
#include <stdint.h>

namespace gt {
  namespace stinger {

    #define MON_STATES \
      STATE(READY_UPDATE), \
      STATE(PERFORMING_UPDATE), \
      STATE(DONE), \
      STATE(ERROR), \
      STATE(MAX)

    #define STATE(X) MON_STATE_##X
    enum mon_state {
      MON_STATES
    };
    #undef STATE

    /**
    * @brief Struct to handle outgoing monitor state and configuration.
    * Data should be handled through the server state.
    */
    struct StingerMonState {
      StingerMonState() : state(MON_STATE_READY_UPDATE) { }

      std::string name;

      int sock_handle;

      enum mon_state state;

      static const char *
      get_state_string(int state);
    };

  } /* gt */
} /* stinger */

#endif  /*STINGER_MON_STATE_H*/
