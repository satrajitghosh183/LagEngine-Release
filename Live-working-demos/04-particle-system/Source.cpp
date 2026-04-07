#include <GL/freeglut.h>
#include <time.h>
#include <list>
#include <math.h>
#include <iostream>
#include "Particle.h"
#include "Floor.h"
#include "Line.h"

using namespace std;

// window properties
int refreshRate = 20; // how fast the scene is redrawn
int height = 800;
int width = 800;
double yRotate = 212.50;
int xRotate = 25;
int zoom = 50;

// particle properties
float scaleFactor = 0.25; // size of particle
int appType = 3; // appearance of the particles
bool particlePaths = false; // draw paths?
bool particleBumping = false; // collision on/off?
// colors for particles
int cArr[3][3] = { {0, 162, 211}, {250, 224, 20}, {224, 8, 133} };

// environment properties
double gravity = 0.1;
double friction = 0.2;
int numFloors = 5;
bool removeParticles = true; // remove dead particles?
int particleCount = 0;
float firePosition[3] = { 0,15,0 }; // cannon position
// light is positioned in a corner of the environment
GLfloat lightSource[] = { 150.0, 150.0, 150.0, 0.0 }; // light position
GLfloat light[] = { 1.0, 1.0, 1.0, 1.0 }; // light color

// graphics properties
bool shadeMode = true; // flat or smooth?
bool useLight = true; // light on/off?
bool useCull = false; // use culling?
bool animationPause = false; // pause animation?

// cannon properties
double spreadRandomness = 0.2; // randomness of stream
bool constantFire = false; // constant fire?
bool randSpeed = false; // random velocity of particle?

list<Particle> listParticles;
list<Floor> listFloors;

/**
 * Glut Display Init function
 */
void initGlut(void) {
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(height, width);
	glutCreateWindow("Particle Simulation");
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, height, width);
}

/**
 * Display Initialization function
 * sets up display for the program
 */
void initDisplay(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear buffers
	glClearDepth(1.0f); // set depth clear
	glEnable(GL_DEPTH_TEST); // enable depth test
	if (!useLight) { // if lights off
		glDisable(GL_LIGHTING); // turn 'em off
	}
	else { // otherwise they're on
		glEnable(GL_LIGHTING); // keep/turn them on
	}
	glEnable(GL_LIGHT0); // single light source
	glLightfv(GL_LIGHT0, GL_POSITION, lightSource); // init light source
	glEnable(GL_COLOR_MATERIAL); // enable material color tracking
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE); // set it
	glEnable(GL_BLEND); // enable blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // set it
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (!useCull) { // using backface culling or not
		glDisable(GL_CULL_FACE);
	}
	else { // yes
		glEnable(GL_CULL_FACE);
	}
	glCullFace(GL_BACK); // only cull back
	glDepthFunc(GL_LEQUAL); // depth buffer compares =<
	if (shadeMode) { // flat or smooth
		glShadeModel(GL_SMOOTH);
	}
	else {
		glShadeModel(GL_FLAT);
	}
	// nicest perspective correction
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	double aspect = (double)viewport[2] / (double)viewport[3];
	gluPerspective(74, aspect, 2, 300); // taken online for square aspect (n*n)
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_NORMALIZE);
	double xCam = zoom * cos(yRotate); // for y-rot
	double zCam = zoom * sin(yRotate); // for y-rot
	gluLookAt(xCam, xRotate, zCam, 0, -20, 0, 0, 1, 0);
}

/**
 * Particle Creation function
 * creates a new Particle for the environment
 */
void addParticle() {
	Particle p = Particle(firePosition, spreadRandomness, scaleFactor, ++particleCount, randSpeed);
	listParticles.push_back(p);
}

/**
 * Floor Collision Detection function
 */
float floorCollision(Particle &p) {
	float y = p.getPos()[1], x = p.getPos()[0], z = p.getPos()[2];
	for (list<Floor>::iterator f = listFloors.begin(); f != listFloors.end(); ++f) { // check each floor
		if ((y < (f->getPos() + (5 * p.getSize()))) // if collision
			&& (x > (-f->getSize() - (5 * p.getSize()))) // and within certain dist
			&& (x < (f->getSize() + (5 * p.getSize())))
			&& (z > (-f->getSize() - (5 * p.getSize()))) // in both x and z planes
			&& (z < (f->getSize() + (5 * p.getSize())))) {
			return f->getPos(); // return collision y-position
		}
	}
	return 0; // else no collision
}

/**
 * Interparticle Collision function
 */
void particleCollision(Particle &p) {
	/**
	 * p = source particle, what to check against
	 * q = particles in record
	 */
	for (list<Particle>::iterator q = listParticles.begin(); q != listParticles.end(); ++q) {
		if (p.getId() != q->getId()) { // if not source particle
			if (q->getCol() == 2 && p.getCol() == 1) {
				/**
				 * euclidean distance, etc
				 */
				double x = pow(((double)q->getPos()[0] - (double)p.getPos()[0]), 2);
				double y = pow(((double)q->getPos()[1] - (double)p.getPos()[1]), 2);
				double z = pow(((double)q->getPos()[2] - (double)p.getPos()[2]), 2);
				double d = sqrt(x + y + z);
				/**
				 * change direction if collision along x or z plane
				 * change in speed if collision along y plane
				 */
				if (d <= ((p.getSize() * 5) + (q->getSize() * 5))) {
					bool bounceX = (p.getPos()[0] > q->getPos()[0]);
					bool bounceY = (p.getPos()[1] > q->getPos()[1]);
					bool bounceZ = (p.getPos()[2] > q->getPos()[2]);
					p.changeDirection(bounceX, bounceZ, bounceY);
				}
			}
		}
	}
}

/**
 * Particle Movement Function
 * For each particle in the record, move it based on
 * environment variables like friction, gravity, etc
 */
void moveParticles() {
	for (list<Particle>::iterator p = listParticles.begin(); p != listParticles.end(); ++p) { // for each particle
		if (p->getSpeed() != 0) { // if particle is alive
			p->move(gravity); // move it with regards to gravity
		}
		/**
			* The below function checks the position a potential
			* floor collision occurs. If one doesn't occur, it will
			* return zero, otherwise a position value is returned
			*/
		float collisionPosition = floorCollision(*p);
		if (numFloors != 0) { // if floors exist
			if (collisionPosition != 0) { // if hit a floor
				// change color status to indicate >0 bounces
				if (p->getCol() == 0) { p->changeColor(); }
				p->bounce(collisionPosition, friction); // apply friction
				if (p->checkDead(gravity)) { // if particle is dead/dying
					// change color status to indicate dying
					if (p->getCol() == 1) { p->changeColor(); }
				}
			}
			// check if particle off "killplane"
			p->checkOffPyramid(removeParticles, listFloors.back().getPos());
		}
		// perform interparticle collision if flag set
		if (particleBumping) { particleCollision(*p); }
	}
}

/**
 * Function to remove particles from record
 * Below two functions combined for readability
 */
bool recordRemovalPredicate(Particle &p) { return (p.getLife() <= 0); }
void removeRecord() {
	// removes particles based on conditional above
	listParticles.remove_if(recordRemovalPredicate);
}

/**
 * Function to Render Scene
 */
void drawScene(void) {
	initDisplay(); // initialize display variables
	if (constantFire && !animationPause) { // if constant stream enabled
		addParticle(); // add a particle to scene
	}
	int i = numFloors-1;
	for (list<Floor>::iterator f = listFloors.begin(); f != listFloors.end(); ++f) { // for each floor
		// render that floor
		double blend = 255-((double)(i--) / (double)(listFloors.size())) * 255;
		glColor4ub(186, 126, 207, blend); // purple of varying alpha
		glPushMatrix();
		glTranslatef(0, f->getPos(), 0);
		glScalef(f->getSize(), 0.1, f->getSize());
		glutSolidCube(2);
		glPopMatrix();
	}
	glTranslatef(0, 0, 0); // go back to origin
	for (list<Particle>::iterator p = listParticles.begin(); p != listParticles.end(); ++p) { // for each particle
		if (p->getLife() > 0) { // if particle is alive
			glPushMatrix();
			double blend = ((double)p->getLife() / 100) * 255;
			/**
			 * unbounced particles are cyan, bounced are yellow
			 * and stationary are magenta. Magenta particles slowly fade
			 * away which is done using the alpha channel
			 */
			glColor4ub(cArr[p->getCol()][0], cArr[p->getCol()][1], cArr[p->getCol()][2], blend);
			// go to position of particle
			glTranslatef(p->getPos()[0], p->getPos()[1], p->getPos()[2]);
			switch (appType) { // for appearance
				case 1: glutSolidCube(p->getSize() * 5); break;
				case 2: glutWireCube(p->getSize() * 5); break;
				case 3: glutSolidSphere(p->getSize() * 5, 10, 15); break;
				case 4: glutWireSphere(p->getSize() * 5, 10, 15); break;
			}
			glPopMatrix();
			if (particlePaths) { // if pathdrawing enabled
				glColor4ub(255, 255, 255, blend); // white
				glBegin(GL_LINES); // start drawing path
				list<Line> path = p->getPath();
				Line lp = path.front(); // need an initial point
				for (list<Line>::iterator l = path.begin(); l != path.end(); ++l) {
					glVertex3f(lp.getPos()[0], lp.getPos()[1], lp.getPos()[2]); // connect l1
					glVertex3f(l->getPos()[0], l->getPos()[1], l->getPos()[2]); // to l2
					lp = *l; // create new initial point
				}
				glEnd();
			}
		}
	}
	if (!animationPause) { // if not paused
		moveParticles(); // do movement logic
	}
	glFlush();
	glutSwapBuffers();
	removeRecord(); // search for dead particles to remove
}

/**
 * Function to generate pyramid floors for environment
 * this creates five floors for the pyramid with params
 * -15,25, -12.5,20, -10,15, -7.5,10, -5,5 by default
 */
void addFloor(int k) {
	for (double i = -5.0, j = 5.0; k != 0; i -= 2.5, j += 5, k--) {
		listFloors.push_back(Floor(i, j));
	}
}

/**
 * Function to print environment variables
 */
void printVariables() {
	cout << "Number of Particles: " << listParticles.size() << endl;
	cout << "Current Gravity: " << gravity << endl;
	cout << "Current Friction: " << friction << endl;
}

/**
 * Function to reset environment variables to defaults
 * Looks ugly but it takes a lot of space otherwise
 */
void reset() {
	scaleFactor = 0.25; gravity = 0.1; friction = 0.2;
	appType = 3; spreadRandomness = 0.2; removeParticles = true;
	constantFire = false; randSpeed = false; shadeMode = true; 
	useLight = true; useCull = false; animationPause = false;
	particleBumping = false; particlePaths = false;
	yRotate = 212.50; xRotate = 25;	zoom = 50;
	listParticles.clear(); listFloors.clear(); refreshRate = 20;
	double xCam = zoom * cos(yRotate), zCam = zoom * sin(yRotate);
	gluLookAt(xCam, xRotate, zCam, 0, -20, 0, 0, 1, 0);
	firePosition[0] = 0; firePosition[2] = 0; numFloors = 5; addFloor(5);
}

/**
 * Display Refresher function
 */
void repeater(int val) {
	glutPostRedisplay();
	glutTimerFunc(refreshRate, repeater, 0);
}

/**
 * Particle Gravity menu
 */
void particleGravityMenu(int choice) {
	switch (choice) {
		case 1: { gravity = 0.000; break; } // zero gravity
		case 2: { gravity = 0.025; break; } // 1/4 gravity
		case 3: { gravity = 0.100; break; } // 1/1 gravity
		case 4: { gravity = 0.400; break; } // 4/1 gravity
		case 5: { gravity = 1.600; break; } // 16/1 gravity
	}
}

/**
 * Particle Friction menu
 */
void particleFrictionMenu(int choice) {
	switch (choice) {
		case 1: { friction = 0.00; break; } // zero friction
		case 2: { friction = 0.05; break; } // 1/4 friction
		case 3: { friction = 0.20; break; } // 1/1 friction
		case 4: { friction = 0.80; break; } // 4/1 friction
		case 5: { friction = 3.20; break; } // 16/1 friction
	}
}

/**
 * Particle Appearance menu
 */
void particleAppearanceMenu(int choice) {
	switch (choice) {
		case 1: { appType = 1; break; } // solid cube
		case 2: { appType = 2; break; } // wireframe cube
		case 3: { appType = 3; break; } // solid sphere
		case 4: { appType = 4; break; } // wireframe sphere
	}
}

/**
 * Particle Size Menu
 */
void particleSizeMenu(int choice) {
	switch (choice) {
		case 1: { scaleFactor = 0.025f; break; } // point
		case 2: { scaleFactor = 0.050f; break; } // 1/4 size
		case 3: { scaleFactor = 0.100f; break; } // 1/2 size
		case 4: { scaleFactor = 0.200f; break; } // 1/1 size
		case 5: { scaleFactor = 0.400f; break; } // 2/1 size
	}
}

/**
 * Particle Randomness Menu
 */
void particleRandomnessMenu(int choice) {
	switch (choice) {
		case 1: { spreadRandomness = 0.00; break; } // not random
		case 2: { spreadRandomness = 0.10; break; } // low randomness
		case 3: { spreadRandomness = 0.20; break; } // normal randomness
		case 4: { spreadRandomness = 0.30; break; } // very random
		case 5: { spreadRandomness = 0.40; break; } // extremely random
	}
}

/**
 * Firing Options menu
 */
void particleFiringMenu(int choice) {
	switch (choice) {
		case 1: { constantFire = !constantFire;	break; } // constant firing on/off
		case 2: { randSpeed = !randSpeed; break; } // random particle velocity on/off
	}
}

/**
 * Lighting and Shading Variables
 */
void lightShadeMenu(int choice) {
	switch (choice) {
		case 1: { shadeMode = !shadeMode;	break; } // smooth/flat
		case 2: { useLight = !useLight;	break; } // light
		case 3: { useCull = !useCull; break; }  // backface culling
	}
}

/**
 * Animation Speed options
 */
void animationSpeedMenu(int choice) {
	switch (choice) {
		case 1: { refreshRate = 100; break;	} // slow
		case 2: { refreshRate = 20;	break; } // default
		case 3: { refreshRate = 5; break; } // fast
		case 4: { animationPause = !animationPause; break; } // paused
	}
}

/**
 * Function for top-level GLUT menu items
 */
void topMenu(int choice) {
	switch (choice) {
		case 'r': {	reset(); break; } // reset
		case 'p': { animationPause = !animationPause; break; } // pause animation
		case 'q': { exit(0); break; } // quit program
		case 1: { removeParticles = !removeParticles; break; } // toggle immortality
		case 2: { particlePaths = !particlePaths; break; } // particle pathdrawing
	}
}

/**
 * Top level entry in particles menu
 */
void particleMenu(int choice) {
	switch (choice) {
		case 1: { particleBumping = !particleBumping; break; } // interpart. coll.
	}
}

/**
 * Switch complex to alter scene based on keyboard controls
 * This is strictly for environment variables or commands
 * performed with the keyboard.
 */
void menu(unsigned char key, int x, int y) {
	switch (key) {
		case 'f': { addParticle(); break; } // manual fire
		case 'g': { printVariables(); break; } // show diagnostic
		case '1': { yRotate = (yRotate == 359.9) ? 0.0 : yRotate + 0.1; break; } // y-rotate R
		case '2': {	yRotate = (yRotate == 0.0) ? 359.9 : yRotate - 0.1;	break; } // y-rotate L
		case '3': { xRotate += 1; break; } // x-rotate U
		case '4': { xRotate -= 1; break; } // x-rotate D
		case '5': { zoom++; break; } // zoom in
		case '6': {	zoom--;	break; } // zoom out
		case '7': { firePosition[0]++; break; } // move cannon x++
		case '8': { firePosition[0]--; break; } // move cannon x--
		case '9': { firePosition[2]++; break; } // move cannon z++
		case '0': { firePosition[2]--; break; } // move cannon z--
		case 'q': {
			listFloors.clear();
			addFloor(numFloors = (numFloors > 0) ? numFloors - 1 : 0);
			break;
		}
		case 'w': {
			listFloors.clear();
			addFloor(numFloors = (numFloors < 10) ? numFloors + 1 : 10);
			break;
		}
	}
}

/**
 * Function which creates the GLUT menu options
 * ties into switch complex in turn tied to algorithms
 */
void initMenu() {
	// menu for adjusting gravity settings
	int gravityMenu = glutCreateMenu(particleGravityMenu);
	glutAddMenuEntry("No Gravity (0x)", 1);
	glutAddMenuEntry("Low Gravity (1/4x)", 2);
	glutAddMenuEntry("Normal Gravity (1x)", 3);
	glutAddMenuEntry("High Gravity (4x)", 4);
	glutAddMenuEntry("V. High Gravity (16x)", 5);
	// menu for adjusting friction settings
	int frictionMenu = glutCreateMenu(particleFrictionMenu);
	glutAddMenuEntry("No Friction (0x)", 1);
	glutAddMenuEntry("Low Friction (1/4x)", 2);
	glutAddMenuEntry("Normal Friction (1x)", 3);
	glutAddMenuEntry("High Friction (4x)", 4);
	glutAddMenuEntry("V. High Friction (16x)", 5);
	// menu for adjusting particle size
	int sizeMenu = glutCreateMenu(particleSizeMenu);
	glutAddMenuEntry("Extremely Small/Point", 1);
	glutAddMenuEntry("Very Small (1/4x)", 2);
	glutAddMenuEntry("Small (1/2x)", 3);
	glutAddMenuEntry("Normal (1x)", 4);
	glutAddMenuEntry("Big (2x)", 5);
	// menu for adjusting particle appearance
	int appearMenu = glutCreateMenu(particleAppearanceMenu);
	glutAddMenuEntry("Solid Cubes", 1);
	glutAddMenuEntry("Wire Cubes", 2);
	glutAddMenuEntry("Solid Sphere", 3);
	glutAddMenuEntry("Wire Sphere", 4);
	// leads to two above menus
	int objectMenu = glutCreateMenu(particleMenu);
	glutAddSubMenu("Particle Appearance", appearMenu);
	glutAddSubMenu("Particle Size", sizeMenu);
	glutAddMenuEntry("Interparticle Collision toggle", 1);
	// menu for spread randomness
	int randomMenu = glutCreateMenu(particleRandomnessMenu);
	glutAddMenuEntry("Not Random (0x)", 1);
	glutAddMenuEntry("Low Randomness (1/2x)", 2);
	glutAddMenuEntry("Normal Randomness (1x)", 3);
	glutAddMenuEntry("High Randomness (3/2x)", 4);
	glutAddMenuEntry("V. High Randomness (2x)", 5);
	// menu for firing options
	int fireMenu = glutCreateMenu(particleFiringMenu);
	glutAddSubMenu("Spread Randomness", randomMenu);
	glutAddMenuEntry("Toggle Constant Firing", 1);
	glutAddMenuEntry("Toggle Random Initial Speed", 2);
	// menu for light/shading
	int shadeMenu = glutCreateMenu(lightShadeMenu);
	glutAddMenuEntry("Toggle Smooth/Flat Shading", 1);
	glutAddMenuEntry("Turn Light On/Off", 2);
	glutAddMenuEntry("Turn Culling On/Off", 3);
	// menu for animation speed
	int spdMenu = glutCreateMenu(animationSpeedMenu);
	glutAddMenuEntry("Slow (1/15x)", 1);
	glutAddMenuEntry("Normal (1x)", 2);
	glutAddMenuEntry("Fast (15x)", 3);
	// main menu to change settings
	int mainMenu = glutCreateMenu(topMenu);
	glutAddSubMenu("Gravity Settings", gravityMenu);
	glutAddSubMenu("Friction Settings", frictionMenu);
	glutAddSubMenu("Particle Settings", objectMenu);
	glutAddSubMenu("Firing Settings", fireMenu);
	glutAddSubMenu("Light/Shading Settings", shadeMenu);
	glutAddSubMenu("Animation Settings", spdMenu);
	glutAddMenuEntry("Toggle Immortality", 1);
	glutAddMenuEntry("Toggle Path Drawing", 2);
	glutAddMenuEntry("Pause Animation", 'p');
	glutAddMenuEntry("Reset", 'r');
	glutAddMenuEntry("Quit", 'q');
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

/**
 * Function which prints the GLUT menu
 */
void printMenu() {
	cout << "Hold right-click to open GLUT menus." << endl;
	cout << "Press or hold 'F' to fire one particles." << endl;
	cout << "Press or hold '1' or '2' to rotate along y-axis left/right." << endl;
	cout << "Press or hold '3' or '4' to rotate along x-axis up/down." << endl;
	cout << "Press or hold '5' or '6' to zoom scene in/out." << endl;
	cout << "Press or hold '7', '8', '9', '0' to move cannon along x/z planes." << endl;
	cout << "Press 'Q' to redesign environment with one less floor." << endl;
	cout << "Press 'W' to redesign environment with one more floor." << endl;
	cout << "Press 'G' to see diagnostic information on environment." << endl;
}

/**
 * Main Driver
 */
int main(int argc, char** argv) {
	listParticles = list<Particle>();
	listFloors = list<Floor>();
	printMenu();
	srand((unsigned int)time(NULL));
	glutInit(&argc, argv);
	addFloor(5);
	initGlut();
	glutDisplayFunc(drawScene);
	glutKeyboardFunc(menu);
	glutTimerFunc(0, repeater, 0);
	initMenu();
	glutMainLoop();
	return 0; 
}