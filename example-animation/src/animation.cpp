#include "CX_EntryPoint.h"

/*
It is not immediately obvious how to do animations using CX while not blocking
in updateExperiment(), so I've made an example showing how to do so.

There are really just four critical functions:
CX_Display::BLOCKING_setSwappingState(), CX_Display::hasSwappedSinceLastCheck(),
CX_Display::beginDrawingToBackBuffer(), and CX_Display::endDrawingToBackBuffer().

All you have to do to set up the animation is to call:
CX_Display::BLOCKING_setSwappingState(true);
This causes the contents of the back buffer to be automatically swapped to
the front buffer every monitor refresh.

Then, in updateExperiment(), check hasSwappedSinceLastCheck() to see if
a swap has just occured. If so, use beginDrawingToBackBuffer() and 
endDrawingToBackBuffer() to draw whatever the next frame of the animation
is into the back buffer.

That's it!
*/

int mouseX = 0;

double circleRadius = 30;

double angles[3] = { 0, 0, 0 };
double angleMultiplier[3] = { 1, 2, 3 };
int directions[3] = { 1, 1, 1 };
double distancesFromCenter[3] = { 75, 150, 225 };
double distanceMultiplier = 1;

void drawNextFrameOfAnimation (void);
ofPoint calculateCircleCenter(double angleDeg, double distanceFromCenter);

void setupExperiment (void) {
	//Use mouse, but not keyboard.
	Input.setup(false, true);

	//The window needs to be about this size in order to fit the circles.
	Display.setWindowResolution(600, 600);

	//See the main comment at the top of this file.
	Display.BLOCKING_setSwappingState(true);
}

void updateExperiment (void) {
	//See the main comment at the top of this file.
	if (Display.hasSwappedSinceLastCheck()) {
		Display.beginDrawingToBackBuffer(); //Prepare to draw the next frame of the animation.

		drawNextFrameOfAnimation();

		Display.endDrawingToBackBuffer(); //Make sure to call this to end the drawing.
	}

	//Do a little bit of stuff to get the state of the mouse.
	while (Input.Mouse.availableEvents() > 0) {
		CX_MouseEvent_t mev = Input.Mouse.getNextEvent();
		if (mev.eventType == CX_MouseEvent_t::MOVED) {
			mouseX = mev.x;
		}

		if (mev.eventType == CX_MouseEvent_t::PRESSED) {
			for (int i = 0; i < 3; i++) {
				ofPoint circleCenter = calculateCircleCenter(angles[i], distancesFromCenter[i]);
				if (circleCenter.distance(ofPoint(mev.x, mev.y)) <= circleRadius) {
					directions[i] *= -1;
				}
			}
		}

		if (mev.eventType == CX_MouseEvent_t::SCROLLED) {
			distanceMultiplier += mev.y * .02;
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
		ofCircle(calculateCircleCenter(angles[i], distancesFromCenter[i]), circleRadius);
	}
}

ofPoint calculateCircleCenter(double angleDeg, double distanceFromCenter) {
	return ofPoint(Display.getCenterOfDisplay().x + cos(angleDeg * PI / 180) * distanceFromCenter * distanceMultiplier,
				   Display.getCenterOfDisplay().y + sin(angleDeg * PI / 180) * distanceFromCenter * distanceMultiplier);
}