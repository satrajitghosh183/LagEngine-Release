#pragma once
#include <array>

/**
 * Line Class
 * not as important, just describes pathdrawing
 */

class Line {
private:
	float x, y, z;
public:
	Line(float a, float b, float c) {
		x = a; y = b; z = c;
	}
	std::array<float, 3> getPos() {
		return std::array<float, 3>{ x,y,z };
	}
};