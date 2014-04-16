#include "CX_EntryPoint.h"

//Because this example is in part a test of the rendering capabilities of your hardware, if you are
//experiencing crashes, you can try commenting out these defines in order to eliminate certain types
//of rendering in order to help localize the source of the problem.
#define CX_RT_USE_FBO //ofFbo
#define CX_RT_USE_PATH //ofPath
#define CX_RT_USE_TEXTURE //ofTexture
#define CX_RT_USE_IMAGE //ofImage
#define CX_RT_USE_TTF //ofTrueTypeFont

#ifdef CX_RT_USE_FBO
ofFbo mainFbo;
ofFbo transparency;
ofFbo trivialFbo;
#endif

#ifdef CX_RT_USE_PATH
ofPath squirclePath;
ofPath arrowPath;
#endif

#ifdef CX_RT_USE_IMAGE
ofImage birds;
#endif

#ifdef CX_RT_USE_TEXTURE
ofTexture mirroredBirds;
#endif

#ifdef CX_RT_USE_TTF
ofTrueTypeFont smallFont;
ofTrueTypeFont largeFont;
#endif

ofVbo rainbowVbo;

bool drawingToFboFirst = false;
float starSize = .8;

void updateDrawings(void);
void drawStuff (void);

void runExperiment(void) {

	Input.setup(true, true);

	Display.setWindowResolution(800, 600);
	Display.setWindowTitle("CX Rendering Test");

#ifdef CX_RT_USE_FBO
	//This is the most simple way to use an ofFbo.
	trivialFbo.allocate(100, 100, GL_RGB); //You must allocate the fbo before you use it. 
		//Make it 100 X 100 pixels. GL_RGB means that is should be in color (red, green, and blue channels).
	trivialFbo.begin(); //All drawing that happens between begin() and end() causes the drawing to be rendered to the ofFbo.
	ofBackground(0, 255, 0); //Draw a green background.
	trivialFbo.end(); //Finish drawing to the fbo.


	//Here is an example of both 1) storing drawn data in a framebuffer and then drawing that framebuffer at multiple places
	//and 2) drawing with transparency.
	//transparency is an ofFbo. You can allocate and draw to it once, then draw the contents of it later into other framebuffers.
	transparency.allocate(200, 200, GL_RGBA); //Allocate a 200x200 pixel framebuffer with an alpha channel. The default is GL_RGBA.

	transparency.begin(); //Begin rendering to the framebuffer. This means that all rendering commands that are used before
		//transparency.end() is called go into this framebuffer.

	//Fill the background of the framebuffer, making it opaque (with an alpha value of 255)
	ofBackground(ofColor(200,200,200,255)); 

	//Draw a rectangle in the framebuffer, but with an alpha value of less than 255 (in this case 50).
	ofSetColor(ofColor(255,255,255,50)); //The alpha determines how this will be blended with the things below it when it is drawn.
	ofRect(30, 30, 140, 140); //When you are drawing into a framebuffer, the drawing coordinates should be in the cooordinate system
		//of the framebuffer. Then when the framebuffer is drawn into something else (e.g. another framebuffer) you specify where in
		//that buffer to draw this buffer.

	ofSetColor(ofColor(0,255,0,100)); //Green with some transparency.
	ofSetCircleResolution(50);
	ofCircle(100, 100, 50);

	ofSetColor(0);
	ofDrawBitmapString( "ofFbo + transparency", 10, 10 );

	transparency.end(); //Stop drawing to the transparency fbo

	mainFbo.allocate(Display.getResolution().x, Display.getResolution().y, GL_RGB, CX::Util::getSampleCount());
#endif

#ifdef CX_RT_USE_PATH
	squirclePath = Draw::squircleToPath(50);
	squirclePath.setFilled(true);
	squirclePath.setStrokeColor(ofColor::white);

	arrowPath = Draw::arrowToPath(150, 45, 50, 10);
	arrowPath.setFillColor(ofColor::orange);
	arrowPath.rotate(60, ofVec3f(0, 0, 1));
#endif

#ifdef CX_RT_USE_IMAGE
	birds.loadImage("4birds.png"); //Example of loading an image file. This file should be put into projectDir/bin/data.
		//You can find this file in the directory for the renderingTest example.
#endif

#ifdef CX_RT_USE_TEXTURE
	//You can manipulate the data in the image if you read it out into an ofPixels:
	ofPixels mirroredPix;
	mirroredPix.allocate(birds.width, birds.height, birds.getPixelsRef().getImageType());
	birds.getPixelsRef().mirrorTo(mirroredPix, true, true);

	//Save the mirrored data into an ofTexture, which an be drawn directly
	mirroredBirds.allocate(mirroredPix);
	mirroredBirds.loadData(mirroredPix);
#endif

#ifdef CX_RT_USE_TTF
	smallFont.loadFont(OF_TTF_MONO, 12);
	largeFont.loadFont(OF_TTF_SERIF, 40);
#endif

	vector<ofFloatColor> rainbowColors = Draw::getRGBSpectrum<ofFloatColor>(90);
	rainbowVbo = Draw::colorArcToVbo(ofPoint(400, 550), rainbowColors, 100, 70, 30, 0, 180);

	while (true) {
		updateDrawings();
		Log.flush();
	}
}

void updateDrawings (void) {

	if (Input.pollEvents()) {
		while (Input.Keyboard.availableEvents()) {
			CX_Keyboard::Event ev = Input.Keyboard.getNextEvent();
			if (ev.eventType == CX_Keyboard::Event::PRESSED) {
				drawingToFboFirst = !drawingToFboFirst;
			}			
		}

		while (Input.Mouse.availableEvents()) {
			CX_Mouse::Event ev = Input.Mouse.getNextEvent();
			if (ev.eventType == CX_Mouse::Event::SCROLLED) {
				starSize += .05 * ev.y;
			}
		}
	}

#ifdef CX_RT_USE_FBO
	if (drawingToFboFirst) {
		mainFbo.begin();
		drawStuff();
		ofSetColor(255);
		ofDrawBitmapString("FBO", 20, 20);
		mainFbo.end();
				
		Display.copyFboToBackBuffer(mainFbo);
	} else 
#endif
	{
		Display.beginDrawingToBackBuffer();
		drawStuff();
		ofSetColor(255);
		ofDrawBitmapString("Back buffer", 20, 20);
		Display.endDrawingToBackBuffer();
	}
	Display.BLOCKING_swapFrontAndBackBuffers();

}

void drawStuff (void) {
	ofBackground( 50 ); //Fill the whole image with this color

	ofSetColor( 200, 100, 100 ); //Set the color of the next thing to be drawn
	Draw::line(ofPoint(180, 30), ofPoint(240, 70), 6); //Draw a line with the specified width from point to point.

	ofSetCircleResolution(6); //This sets the number of lines that will be used to draw the outer edge of the circle.
	ofCircle( 50, 50, 20 ); //This is really a hexagon.

	ofSetCircleResolution(50);
	ofCircle(100, 50, 20); //This looks much more like a circle.
	
	Draw::ring(ofPoint(150, 50), 20, 5, 40); //This can draw unfilled circles with variable thickness edges.

	Draw::arc(ofPoint(400, 50), 20, 30, 0, 135, 5, 40);

	ofSetColor(ofColor::blue);
	ofRect(20, 100, 60, 40); //Draw a rectangle
	ofSetColor(ofColor(0, 255, 0, 127)); //If the set the alpha channel to less than 255, you get transparency effects
	ofEllipse(40, 140, 40, 70); //Drawn over the rectangle

	ofSetColor(ofColor::red);
	ofTriangle(ofPoint(50, 250), ofPoint(150, 400), ofPoint(280, 350));
	

	ofSetColor(ofColor::darkorange);
	ofNoFill(); //Don't fill basic shapes
	ofTriangle( 100, 100, 150, 150, 100, 150 );
	ofFill(); //Fill them again

	vector<ofPoint> cps(4);
	cps[0] = ofPoint(240, 180);
	cps[1] = cps[0] + ofPoint(60, 0);
	cps[2] = cps[1] + ofPoint(0, 60);
	cps[3] = cps[2] + ofPoint(60, 0);
	ofSetColor(ofColor::green);
	Draw::bezier(cps, 10, 20);

#ifdef CX_RT_USE_FBO
	ofSetColor(255); //Before drawing an ofFbo that has transparent elements, if the color is not set to white, 
		//the output of the draw command looks wrong (merged with the current color).
	transparency.draw(30, 280);

	ofSetColor(255);
	trivialFbo.draw(30, 450);
#endif

#ifdef CX_RT_USE_TEXTURE
	//This section of code makes the strange bird picture effect. The image of the birds is mirrored in setup.
	ofSetColor(255);
	mirroredBirds.draw(500, 20);
#endif

#ifdef CX_RT_USE_IMAGE
	//Here we get a greyscale pattern
	Draw::CX_PatternProperties_t patternProps;
	patternProps.width = birds.width;
	patternProps.height = birds.height;
	patternProps.period = 40;
	patternProps.phase = 360.0 * fmod(Clock.now().seconds(), 1);
	patternProps.apertureType = Draw::CX_PatternProperties_t::AP_RECTANGLE;
	patternProps.angle = 15;
	ofPixels birdPattern = CX::Draw::greyscalePattern(patternProps);

	//The bird image may not have any transparency data, so set it to have an alpha channel
	birds.setImageType(ofImageType::OF_IMAGE_COLOR_ALPHA);
	birds.getPixelsRef().setChannel(3, birdPattern.getChannel(0)); //Set the alpha channel (channel 3) of the birds image to 
		//the single channel of the greyscale pattern.
	birds.draw(500, 20);
#endif

	rainbowVbo.draw(GL_TRIANGLE_STRIP, 0, rainbowVbo.getNumVertices());

#ifdef CX_RT_USE_TTF
	ofSetColor(255);
	smallFont.drawString("Some small text", 550, 500);
	ofSetColor(255, 0, 150);
	largeFont.drawString("Big text", 550, 540);
#endif

#ifdef CX_RT_USE_PATH
	//This squircle is rotated around all three axes at once. If you want to rotate ofPaths only around the Z axis (i.e. the normal 2D rotation),
	//use ofVec3f(0,0,1) for the axis argument (no x, no y, yes z).
	squirclePath.rotate(.5, ofVec3f(1, 1, 1)); //The current rotation is saved by the ofPath, so each time this called, it rotates a little more.
	squirclePath.draw(300, 70);

	arrowPath.draw(650, 400);

	//The size of this star can be changed with the mouse wheel
	Draw::star(ofPoint(500, 400), 5, 30 * starSize, 70 * starSize, ofColor::turquoise);
#endif

#ifdef CX_RT_USE_TEXTURE
	//Have a gabor follow the mouse around, pointing toward the center.
	ofPoint sides = Input.Mouse.getCursorPosition() - Display.getCenterOfDisplay();
	double theta = atan2(sides.y, sides.x) * 180 / PI;

	Draw::CX_GaborProperties_t prop;
	
	prop.color = ofColor::green;
	prop.pattern.angle = theta;
	prop.pattern.width = 150;
	prop.pattern.height = 100;
	prop.pattern.maskType = Draw::CX_PatternProperties_t::SINE_WAVE;
	prop.pattern.period = 20;
	prop.pattern.phase = starSize * 360;
	prop.pattern.apertureType = Draw::CX_PatternProperties_t::AP_CIRCLE;
	prop.pattern.fallOffPower = 6;
	CX::Draw::gabor(ofGetMouseX(), ofGetMouseY(), prop);
#endif

	

}

