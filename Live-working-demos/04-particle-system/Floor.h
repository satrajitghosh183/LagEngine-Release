#pragma once

/**
 * Floor Class
 * not as important, just a way to set position for floors
 */

class Floor {
private:
	float position;
	float size;
public:
	Floor(float p, float s) {
		position = p;
		size = s;
	}
	float getPos() {
		return position;
	}
	float getSize() {
		return size;
	}
};