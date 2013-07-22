#include "stinger_alg_state.h"

#include "stinger_core/stinger_error.h"

using namespace gt::stinger;

const char *
StingerAlgState::get_state_string(int state)
{
  #define STATE(X) "ALG_STATE_"#X
  const char * state_strings [] = {
    ALG_STATES
  };
  #undef STATE

  if(state < ALG_STATE_MAX) {
    return state_strings[state];
  } else {
    LOG_W_A("Invalid state number requested %d", state);
    return "ERROR_INVALID_STATE";
  }
}
