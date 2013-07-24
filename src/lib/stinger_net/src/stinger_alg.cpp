#include "stinger_alg.h"

#include <netdb.h>
#include <cstdio>
#include <limits>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

extern "C" {
  #include "stinger_core/xmalloc.h"
  #include "stinger_core/stinger_error.h"
}

extern "C" stinger_registered_alg *
stinger_register_alg_impl(stinger_register_alg_params params)
{
  struct addrinfo * result;
  struct addrinfo hint = {.ai_protocol = params.port};
  if(getaddrinfo(params.host, NULL, &hint, &result)) {
    LOG_E_A("Resolving %s %d failed", params.host, params.port);
  }

  stinger_registered_alg * rtn = (stinger_registered_alg *) xmalloc(sizeof(stinger_registered_alg));
}
