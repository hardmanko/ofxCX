#include "CX_EntryPoint.h"

//setupExperiment is called a single time.
void setupExperiment (void) {

	//Easy Hello, world! Data given to cout will be displayed in the console window that opens with the main application. Very useful for debugging.
	cout << "Hello, console!" << endl;

	//CX uses many oF types directly. ofTrueTypeFont is one of those types.
	ofTrueTypeFont font;
	font.loadFont(OF_TTF_SANS, 20); //Use the system-standard sans-serif font at size 20.
	
	//Display is an instance of class CX_Display that is created for you. It is used for drawing on the screen.
	//Between calls to CX_Display::beginDrawingToBackBuffer and CX_Display::endDrawingToBackBuffer,
	//any functions that draw something will have the result of the drawing put into the back buffer of the video card.
	Display.beginDrawingToBackBuffer();

	ofBackground(0); //Set the back buffer to black.

	ofSetColor(255); //Set the drawing color to white
	font.drawString( "Hello, world!", 30, 40 ); //Draw the "Hello, world!" at the specified coordinates.

	//Finish drawing to the back buffer, making to ready to be swapped to the front buffer.
	Display.endDrawingToBackBuffer();

	//Swap the front and back buffers.
	Display.BLOCKING_swapFrontAndBackBuffers();
}

//updateExperiment is called continuously.
void updateExperiment (void) {
	//We don't need to update anything for this example.
}