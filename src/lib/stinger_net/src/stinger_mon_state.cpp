#include "stinger_mon_state.h"

#include "stinger_core/stinger_error.h"

using namespace gt::stinger;

const char *
StingerMonState::get_state_string(int state)
{
  #define STATE(X) "MON_STATE_"#X
  const char * state_strings [] = {
    MON_STATES
  };
  #undef STATE

  if(state < MON_STATE_MAX) {
    return state_strings[state];
  } else {
    LOG_W_A("Invalid state number requested %d", state);
    return "ERROR_INVALID_STATE";
  }
}
