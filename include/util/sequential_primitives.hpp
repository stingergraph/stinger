namespace Stinger {
  namespace Util {

    uint64_t xorHashAndMix64(uint8_t * byte_string, uint64_t length) {
      uint64_t out = 0;
      uint64_t * byte64 = (uint64_t *)byte_string;
      while(length > 8) {
	length -= 8;
	out ^= *(byte64);
	byte64++;
      }
      if(length > 0) {
	uint64_t last = 0;
	uint8_t * cur = (uint8_t *)&last;
	uint8_t * byte8 = (uint8_t *)byte64;
	while(length > 0) {
	  length--;
	  *(cur++) = *(byte8++);
	}
	out ^= last;
      }
    }
  }
}
