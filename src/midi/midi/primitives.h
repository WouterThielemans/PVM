#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include "util/tagged.h"
#include <sstream>

namespace midi {
	struct __declspec(empty_bases)Channel : tagged<uint8_t, Channel>, equality<Channel>, show_value<Channel, int>{ using tagged::tagged; };
	struct __declspec(empty_bases)Instrument : tagged<uint8_t, Instrument>, equality<Instrument>, show_value<Instrument, int>{ using tagged::tagged;};
	struct __declspec(empty_bases)NoteNumber : tagged<uint8_t, NoteNumber>, ordered<NoteNumber>, show_value<NoteNumber, int>{ using tagged::tagged; };
	struct __declspec(empty_bases)Time : tagged<uint64_t, Time>, ordered<Time>, show_value<Time, int>{ using tagged::tagged; };
	struct __declspec(empty_bases)Duration : tagged<uint64_t, Duration>, ordered<Duration>, show_value<Duration, int>{using tagged::tagged; };

	Duration operator +(const Duration& a, const Duration& b);
	Duration operator -(const Time& a, const Time& b);
	Duration operator -(const Duration& a, const Duration& b);
	Duration& operator +=(Duration& a, const Duration& b);
	Duration& operator -=(Duration& a, const Duration& b);
	Time& operator +=(Time& a, const Duration& b);
	Time operator +(const Time& a, const Duration& b);
	Time operator +(const Duration& a, const Time& b);
}
#endif