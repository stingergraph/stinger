extern "C" {
  #include "stinger_core/stinger.h"
  #include "stinger_net/stinger_alg.h"
}

int
main(int argc, char *argv[])
{
  stinger_registered_alg * alg = stinger_register_alg(.name="test_alg", .host="localhost");
}
