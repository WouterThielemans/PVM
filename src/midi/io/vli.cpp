#include "vli.h"

uint64_t io::read_variable_length_integer(std::istream& in) {
	uint8_t byte = in.get();
	uint64_t res = 0x0000000000000000;
	while ((byte >> 7) == 0b00000001) {
		res = (res << 7) | (byte & 0b01111111);
		byte = in.get();
	}
	res = (res << 7) | (byte & 0b01111111);
	return res;
}