#ifndef  STINGER_ALG_STATE_H
#define  STINGER_ALG_STATE_H

#include <string>

namespace gt {
  namespace stinger {

    #define ALG_STATES \
      STATE(INIT), \
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
      StingerAlgState() : state(ALG_STATE_INIT), data_loc("") { }

      std::string name;
      std::string data_loc;
      int64_t data_per_vertex;

      int sock_handle;

      enum alg_state state;

      static const char *
      get_state_string(int state);
    };

  } /* gt */
} /* stinger */

#endif  /*STINGER_ALG_STATE_H*/
