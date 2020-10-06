#pragma once
#include "primitives.h"

namespace midi  {
	Duration operator+(const Duration& a, const Duration& b){
		return Duration(value(a) + value(b));
	}
	Time operator+(const Time& a, const Duration& b){
		return Time(value(a) + value(b));
	}
	Time operator+(const Duration& a, const Time& b){
		return Time(value(a) + value(b));
	}
	Duration operator-(const Time& a, const Time& b){
		return Duration(value(a) - value(b));
	}
	Duration operator-(const Duration& a, const Duration& b){
		return Duration(value(a) - value(b));
	}
	Duration& operator+=(Duration& a, const Duration& b){
		a = a + b;
		return a;
	}
	Duration& operator-=(Duration& a, const Duration& b){
		a = a - b;
		return a;
	}
	Time& operator+=(Time& a, const Duration& b){
		a = a + b;
		return a;
	}
}