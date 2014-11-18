#ifndef _ALG_DATA_ARRAY_H
#define _ALG_DATA_ARRAY_H

#include "rapidjson/document.h"
#include "stinger_net/stinger_alg_state.h"
#include "rpc_state.h"

namespace gt {
  namespace stinger {

    class AlgDataArray {
      private:
	char t;
	void * d;
	int64_t len;

	JSON_RPCServerState * state;
	char alg[128];
	int64_t offset;

      public:
	AlgDataArray(
		    JSON_RPCServerState * server_state,
		    const char * algorithm_name,
		    void * data, char type, int64_t length);

	AlgDataArray(
		    JSON_RPCServerState * server_state,
		    const char * algorithm_name,
		    const char * field);

	int64_t length();
	char    type();
	void	refresh();

        template <typename T> T getval(int64_t index);
        template <typename T> const T * getptr();

	int32_t get_int32(int64_t index);
	int64_t get_int64(int64_t index);
	float   get_float(int64_t index);
	double  get_double(int64_t index);
	uint8_t get_uint8(int64_t index);

	bool    equal(int64_t a, int64_t b);
	void    to_value(int64_t index, rapidjson::Value & val, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
    };

  }
}

#endif /* _ALG_DATA_ARRAY_H */
