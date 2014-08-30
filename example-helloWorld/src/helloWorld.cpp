#include "CX_EntryPoint.h"

//runExperiment is called a single time. When it returns, the program closes.
void runExperiment (void) {

	//Data given to cout will be displayed in the console window that opens with the main application.
	cout << "Hello, console!" << endl;

	//CX uses many openFrameworks types directly. ofTrueTypeFont is one of those types and it is used for presenting text.
	ofTrueTypeFont font;
	font.loadFont(OF_TTF_SANS, 20); //Use the system-standard sans-serif font at size 20.
	
	//Display is an instance of class CX_Display that is created for you. It is used for drawing on the screen.
	//Between calls to CX_Display::beginDrawingToBackBuffer and CX_Display::endDrawingToBackBuffer,
	//any functions that draw something will have the result of the drawing put into the back buffer of the video card.
	//See Framebuffers and Buffer Swapping in the manual for more information.
	Display.beginDrawingToBackBuffer();

	ofBackground(0); //Set screen to black.

	ofSetColor(255); //Set the drawing color to white
	font.drawString( "Hello, world!", 30, 40 ); //Draw the "Hello, world!" at the specified pixel coordinates.
		//Pixel coordinates start at 0,0 in the upper left corner of the screen. Y values increase downwards, X-values increase to the right.
	font.drawString( "See the console for other greetings.", 30, 80 ); 
	font.drawString( "Press any key to exit", 30, 120 ); 

	//Finish drawing to the back buffer, making it ready to be swapped to the front buffer.
	Display.endDrawingToBackBuffer();

	//Swap the front and back buffers, which moves what you drew into the offscreen back buffer into the onscreen front buffer.
	Display.swapBuffers();

	//Wait for any key press before continuing.
	Input.Keyboard.waitForKeypress(-1);

	//Just past this point, the runExperiment function returns implicitly and the program exits.
}