#pragma once
#include <random>
#include <list>
#include <array>
#include "Line.h"

/**
 * Particle Class
 * this describes all of the logic based on particles
 * including movement, collision, positioning, friction, gravity, etc
 * as well as pathdrawing logic
 */

class Particle{
private:
	float x, y, z; // position in 3-space
	float dx, dy, dz; // velocity/direction in 3-space
	float size; // size of particle
	float speed; // speed of particle
	int life; // life of particle
	int color; // color of particle
	int lineDivisor; // for pathdrawing
	static const int maxDivisor = 1; // how aliased the line is, higher is worse
	int id; // for identification
	int buffer;
	static const int maxBuffer = 5;
	std::list<Line> path; // for pathdrawing
public:
	/**
	 * Particle Constructor
	 * @param fp - default spawn position
	 * @param sr - spread randomness
	 * @param sf - scale factor of particle
	 * @param pn - particle number
	 * @param rs - randomness toggle for speed
	 */
	Particle(float fp[3], double sr, float sf, int pn, bool rs) {
		id = pn; // identifies which number particle it is
		color = 0; // default color is cyan, 1 = yellow, 2 = magenta
		/**
		 * fp is the firing position of the particle, or
		 * where the invisible "particle cannon" exists.
		 * this is the default spawning point for a particle
		 */
		x = fp[0];
		y = fp[1];
		z = fp[2];
		// random direction in x-plane
		dx = (((float)(rand() % 100) / 100) - 0.5) * sr;
		// direction is down
		dy = 0;
		// random direction in z-plane
		dz = (((float)(rand() % 100) / 100) - 0.5) * sr;
		// random speed if randomized speed toggle is on
		speed = (rs) ? ((double)(rand() % 30) / -10) - 0.01 : -0.01;
		life = 100; // begins with 100 life, removed from record at 0
		size = sf; // global variable in main determines sizing
		/**
		 * how aliased a pathdrawn line is. 1-2 works well. 0 would be
		 * ideal but performance suffers. Upper threshold is
		 * something like 4 where it becomes too ugly
		 */
		lineDivisor = maxDivisor;
		path = std::list<Line>(); // line for path drawing
		path.push_back(Line(x, y, z));
		buffer = maxBuffer;
	}
	// getters for various fields of class
	std::array<float, 3> getPos() {
		return std::array<float,3>{ x,y,z };
	}
	std::array<float, 3> getVel() {
		return std::array<float, 3>{ dx, dy, dz };
	}
	int getLife() {
		return life;
	}
	float getSize() {
		return size;
	}
	float getSpeed() {
		return speed;
	}
	int getCol() {
		return color;
	}
	int getId() {
		return id;
	}
	std::list<Line> getPath() {
		return path;
	}
	/**
	 * Velocity Changing function
	 * for interparticle collision, determine in which way to reflect off others
	 * @param a - whether to reflect in x-plane
	 * @param b - in z-plane
	 * @param c - in y-plane
	 */
	void changeDirection(bool a, bool b, bool c) {
		if (buffer == 5) {
			dx = (a) ? -dx : dx; // change in x
			dz = (b) ? -dz : dz; // change in z
			speed = (c) ? -speed : speed; // change in y
		}
		// collisions may happen upon multiple frames, don't bounce and then bounce back
		buffer = (buffer == 0) ? maxBuffer : buffer - 1;
	}
	/**
	 * Particle movement function
	 * @param g - gravity
	 */
	void move(double g) {
		speed -= g; // gravity affects speed in y-plane
		// position changes based on velocity and speed
		x += dx;
		y += dy + speed;
		z += dz;
		// for path drawing
		if (lineDivisor > 0) { // if not yet time
			/**
			 * I only want paths drawn every 2 units of time
			 * so this is just a counting method to determine
			 * when to make a new point in the line
			 */
			lineDivisor--;
		}
		else if (lineDivisor == 0 && speed != 0) { // if it is time
			path.push_back(Line(x, y, z));
			lineDivisor = maxDivisor; // reset counter
		}
	}
	/**
	 * Bounce physics vs floor function
	 * @param c - floor position
	 * @param f - friction
	 */
	void bounce(float c, double f) {
		y = c + 5 * size; // need offset for particle size
		/**
		 * simplified friction model, just a slight big of 
		 * speed taken off with each bounce. 
		 */
		speed = round(10000 * (-speed / (1 + f))) / 10000;
	}
	/**
	 * Particle Life function
	 * @param g - gravity
	 */
	bool checkDead(double g) {
		// particles slowly start to die once stationary
		if (life < 100) { // non-100 life is stationary
			life--; // decrease life
			return true; // return dead/dying
		} else if (speed <= (g - 0.01) && speed != 0) {
			speed = 0.0; // if arbitrarily close to stationary
			return true; // mark dead
		}
		return false; // otherwise it's still alive
	}
	/**
	 * Check if below killplane method
	 * @param rp - if we want to delete particle records
	 * @param lf - the lowest floor of the pyramid
	 */
	void checkOffPyramid(bool rp, float lf) {
		if (speed == 0 || y < lf) {
			color = 2; // change to magenta
			if (rp || y < lf) {
				life--;
			}
		}
	}
	// to change color of particle
	void changeColor() {
		color = (color == 2) ? 2 : color + 1;
	}
};