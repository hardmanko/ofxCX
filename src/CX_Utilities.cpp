#include "CX_Utilities.h"

#include "ofMain.h"
#include "ofConstants.h"
//#include "ofFileUtils.h"

//These two go together
#include "GLFW\glfw3.h"
GLFWwindow *CX::Private::glfwContext;

using namespace std;
using namespace CX;

//#include "ofFbo.h"
//#include "ofAppGLFWWindow.h"

/*
void CX::exit (void) {
	ofExit();
}
*/

//void CX::pollEvents (void) {
//	glfwPollEvents();
//}

/*
void CX::drawFboToBackBuffer (ofFbo &fbo) {
	ofPtr<ofGLProgrammableRenderer> renderer = ofGetGLProgrammableRenderer();

	if(renderer){
		renderer->startRender();
	}

	ofViewport();
	ofSetupScreen();

	ofSetColor( 255 ); //ofFbo.draw() eventually calls a ofTexture.draw(), which means that is ofColor != 255, there are strange color problems.
	fbo.draw(0, 0, ofGetWidth(), ofGetHeight());

	if(renderer){
		renderer->finishRender();
	}

	glFlush(); //Make sure that all openGL commands complete in "finite time" by flushing all command buffers to the video card(s).
	//glFinish();
}
*/

int CX::getSampleCount (void) { 
	return 4; 
};

bool CX::checkOFVersion (int versionMajor, int versionMinor, int versionPatch) {
	if (versionMajor == OF_VERSION_MAJOR && versionMinor == OF_VERSION_MINOR && versionPatch == OF_VERSION_PATCH) {
		return true;
	}
	return false;
}

//Add various log data on failure: why failed?
bool CX::writeToFile (std::string filename, string data, bool append) {
	ofFile out( ofToDataPath(filename), (append ? ofFile::Append : ofFile::WriteOnly), false );
	if (out.is_open()) {
		out << data;
		out.close();
		return true;
	}
	return false;
}

void CX::drawCenteredString (string s, ofTrueTypeFont &font, int x, int y) {
	ofRectangle bb = font.getStringBoundingBox(s, 0, 0);
	x -= bb.width/2;
	y -= bb.height/2;
	font.drawString(s, x, y);
}

void CX::drawCenteredString (string s, ofTrueTypeFont &font, ofPoint location) {
	drawCenteredString(s, font, location.x, location.y);
}