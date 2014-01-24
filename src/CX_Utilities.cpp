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


//When cfenv becomes availble, use that instead.
double CX::round (double d, int roundingPower, CX::CX_RoundingConfiguration c) {
	double loc = std::pow(10, roundingPower);
	double modulo = abs(fmod(d, loc));

	if (d >= 0) {
		d -= modulo;
	} else {
		d -= (loc - modulo);
	}

	switch (c) {
	case CX_RoundingConfiguration::ROUND_TO_NEAREST:
		d += (modulo >= (.5 * loc)) ? (loc) : 0;
		break;
	case CX_RoundingConfiguration::ROUND_UP:
		d += (modulo != 0) ? loc : 0;
		break;
	case CX_RoundingConfiguration::ROUND_DOWN:
		break;
	case CX_RoundingConfiguration::ROUND_TOWARD_ZERO:
		if (d < 0) {
			d += (modulo != 0) ? loc : 0;
		}
		break;
	}

	return d;
}