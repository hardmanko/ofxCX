/* This is a minimal exaple that shows:
1) How to display basic text on screen.
2) How to write text to the console that opens with each program.
3) How to wait for a keypress.
*/

#include "CX.h"

//runExperiment is called a single time. When it returns, the program closes.
void runExperiment (void) {

	//Disp is an instance of class CX_Display that is created for you. It is used for drawing on the screen.
	//Between calls to CX_Display::beginDrawingToBackBuffer() and CX_Display::endDrawingToBackBuffer(),
	//any functions that draw something will have the result of the drawing put into the back buffer of the video card.
	//This means that what has been drawn is not on screen now, but will be as soon as the front and back buffers 
	//are swapped. See the Visual Stimuli section in the manual for more information about offscreen buffers.
	Disp.beginDrawingToBackBuffer();

	//The "of" prefix on these functions means that they are from openFrameworks.
	ofBackground(ofColor::black); //Set the back buffer to black.

	//Set the drawing color to white. The next things that are drawn will be the drawing color.
	ofSetColor(ofColor::white); 

	ofDrawBitmapString("Hello, world!", 30, 40); //Draw "Hello, world!" at the specified pixel coordinates.
		//Pixel coordinates start at 0,0 in the upper left corner of the screen. Y values increase downwards, X-values increase to the right.

	//Draw some more messages.
	ofDrawBitmapString("See the console for other greetings.", 30, 80);
	ofDrawBitmapString("Press any key to exit", 30, 120);

	//Finish drawing to the back buffer, making it ready to be swapped to the front buffer.
	Disp.endDrawingToBackBuffer();

	//Swap the front and back buffers, which moves what you drew into the off-screen back buffer into the on-screen front buffer.
	Disp.swapBuffers();

	//Data given to cout will be displayed in the console window that opens with the main application.
	cout << "Hello, console!" << endl;

	//Wait for any key press before continuing (-1 means any key can be pressed).
	Input.Keyboard.waitForKeypress(-1);

	//Just past this point, the runExperiment function returns implicitly and the program exits.
}