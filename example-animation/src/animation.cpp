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

This example also uses the mouse a little to control the animation.
*/

double angle = 0;
double direction = 1;
int mouseX = 0;

void drawNextFrameOfAnimation (void);

void setupExperiment (void) {
	//Use mouse, but not keyboard.
	Input.setup(false, true);

	cout << "Move the mouse to the left or right to change speed. Click to change direction." << endl;

	//Display.setFullScreen(false);
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
			direction *= -1;
		}
	}
}

void drawNextFrameOfAnimation (void) {
	angle += .05 * (mouseX/600.0) * direction;
	if (angle > 2 * PI) {
		angle = 0;
	}

	int x1 = Display.getCenterOfDisplay().x + cos(angle) * 225;
	int y1 = Display.getCenterOfDisplay().y + sin(angle) * 225;

	int x2 = Display.getCenterOfDisplay().x + cos(angle * 2) * 150;
	int y2 = Display.getCenterOfDisplay().y + sin(angle * 2) * 150;

	int x3 = Display.getCenterOfDisplay().x + cos(angle * 3) * 75;
	int y3 = Display.getCenterOfDisplay().y + sin(angle * 3) * 75;

	ofBackground(0);

	ofSetColor(255, 0, 0);
	ofCircle(x1, y1, 30);

	ofSetColor(0, 255, 0);
	ofCircle(x2, y2, 30);

	ofSetColor(0, 0, 255);
	ofCircle(x3, y3, 30);
}