#include "CX_EntryPoint.h"

/*
It is not immediately obvious how to do animations using CX while not blocking, 
so I've made an example showing how to do so.

There are really just four critical functions:
CX_Display::BLOCKING_setAutoSwapping(), CX_Display::hasSwappedSinceLastCheck(),
CX_Display::beginDrawingToBackBuffer(), and CX_Display::endDrawingToBackBuffer().

All you have to do to set up the animation is to call:
CX_Display::BLOCKING_setAutoSwapping(true);
This causes the contents of the back buffer to be automatically swapped to
the front buffer every monitor refresh.

Then, in updateAnimation(), check hasSwappedSinceLastCheck() to see if
a swap has just occured. If so, use beginDrawingToBackBuffer() and 
endDrawingToBackBuffer() to draw whatever the next frame of the animation
is into the back buffer.

That's it!
*/

//These variables have to do with the contents of the animation
int mouseX = 0;
double circleRadius = 30;
double angles[3] = { 0, 0, 0 };
double angleMultiplier[3] = { 1, 2, 3 };
int directions[3] = { 1, 1, 1 };
double distancesFromCenter[3] = { 75, 150, 225 };
double distanceMultiplier = 1;

void updateAnimation (void);
void drawNextFrameOfAnimation (void);
ofPoint calculateObjectCenter(double angleDeg, double distanceFromCenter);


void runExperiment (void) {
	//Use mouse, but not keyboard.
	Input.setup(false, true);

	//The window needs to be about this size in order to fit the circles.
	Display.setWindowResolution(600, 600);

	//See the main comment at the top of this file.
	Display.BLOCKING_setAutoSwapping(true);

	while (true) {
		updateAnimation();
	}
}

void updateAnimation (void) {

	//See the main comment at the top of this file.
	if (Display.hasSwappedSinceLastCheck()) {
		Display.beginDrawingToBackBuffer(); //Prepare to draw the next frame of the animation.

		drawNextFrameOfAnimation();

		Display.endDrawingToBackBuffer(); //Make sure to call this to end the drawing.
			//Because the front and back buffers are automatically swapping, you don't
			//need to do anything else here: the new frame will be swapped to the front
			//at some point in the near future.
	}

	//Do a little bit of stuff to get the state of the mouse.
	Input.pollEvents();
	while (Input.Mouse.availableEvents() > 0) {
		CX_Mouse::Event mev = Input.Mouse.getNextEvent();
		if (mev.eventType == CX_Mouse::Event::MOVED) {
			mouseX = mev.x;
		}

		//Check to see if a circle was clicked on
		if (mev.eventType == CX_Mouse::Event::PRESSED) {
			for (int i = 0; i < 3; i++) {
				ofPoint circleCenter = calculateObjectCenter(angles[i], distancesFromCenter[i]);
				if (circleCenter.distance(ofPoint(mev.x, mev.y)) <= circleRadius) {
					directions[i] *= -1;
				}
			}
		}

		if (mev.eventType == CX_Mouse::Event::SCROLLED) {
			distanceMultiplier += mev.y * .02; //The y component of the scroll wheel is the typical scroll wheel on most mice
			if (distanceMultiplier > 1.5) {
				distanceMultiplier = 1.5;
			}
			if (distanceMultiplier < -1.5) {
				distanceMultiplier = -1.5;
			}
		}
	}
}

void drawNextFrameOfAnimation (void) {
	ofColor colors[3] = { ofColor::red, ofColor::green, ofColor::blue };

	ofBackground(0);

	ofSetColor(255);
	ofDrawBitmapString("Move the mouse to the left or right to change speed.\n"
					   "Click on a circle to change its direction.\n"
					   "Use the mouse wheel to change the orbit size.", ofPoint(30, 30));

	for (int i = 0; i < 3; i++) {
		angles[i] += 0.005 * mouseX * directions[i] * angleMultiplier[i];
		ofSetColor(colors[i]);
		ofCircle(calculateObjectCenter(angles[i], distancesFromCenter[i]), circleRadius);
	}
}

ofPoint calculateObjectCenter(double angleDeg, double distanceFromCenter) {
	return ofPoint(Display.getCenterOfDisplay().x + cos(angleDeg * PI / 180) * distanceFromCenter * distanceMultiplier,
				   Display.getCenterOfDisplay().y + sin(angleDeg * PI / 180) * distanceFromCenter * distanceMultiplier);
}