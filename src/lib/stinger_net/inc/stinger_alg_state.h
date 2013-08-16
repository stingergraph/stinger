#ifndef  STINGER_ALG_STATE_H
#define  STINGER_ALG_STATE_H

#include <string>
#include <vector>
#include <stdint.h>

namespace gt {
  namespace stinger {

    /* NOTE: all non-processing states (error, etc.) must come after DONE */
    #define ALG_STATES \
      STATE(READY_INIT), \
      STATE(PERFORMING_INIT), \
      STATE(READY_PRE), \
      STATE(PERFORMING_PRE), \
      STATE(READY_POST), \
      STATE(PERFORMING_POST), \
      STATE(DONE), \
      STATE(ERROR), \
      STATE(MAX)

    #define STATE(X) ALG_STATE_##X
    enum alg_state {
      ALG_STATES
    };
    #undef STATE

    /**
    * @brief Struct to handle outgoing algorithm state and configuration.
    * Data should be handled through the server state.
    */
    struct StingerAlgState {
      StingerAlgState() : state(ALG_STATE_READY_INIT), data_loc(""), level(0) { }

      std::string name;
      std::string data_loc;
      std::string data_description;
      void * data;
      int64_t data_per_vertex;
      int64_t level;

      std::vector<std::string> req_dep;
      std::vector<std::string> opt_dep;

      int sock_handle;

      enum alg_state state;

      static const char *
      get_state_string(int state);
    };

  } /* gt */
} /* stinger */

#endif  /*STINGER_ALG_STATE_H*/
