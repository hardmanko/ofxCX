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

/*! Checks that the version of oF that is used during compilation matches the requested version. If the desired version
was 0.7.1, simply input (0, 7, 1) as the arguments. A warning will be logged if the versions don't match.
\return True if the versions match, false otherwise. */
bool CX::checkOFVersion (int versionMajor, int versionMinor, int versionPatch) {
	if (versionMajor == OF_VERSION_MAJOR && versionMinor == OF_VERSION_MINOR && versionPatch == OF_VERSION_PATCH) {
		return true;
	}
	Instances::Log.warning("CX::checkOFVersion") << "oF version does not match. Current version: " << ofGetVersionInfo();
	return false;
}

//Add various log data on failure: why failed?
bool CX::writeToFile (std::string filename, string data, bool append) {
	ofFile out( ofToDataPath(filename), ofFile::Reference );
	if (out.exists() && !append) {
		Instances::Log.warning("CX::writeToFile") << "File " << filename << " already exists. I will be overwritten.";
	}
	out.close();
	out.open( ofToDataPath(filename), (append ? ofFile::Append : ofFile::WriteOnly), false );
	if (out.is_open()) {
		out << data;
		out.close();
		return true;
	} else {
		Instances::Log.error("CX::writeToFile") << "File " << filename << " could not be opened.";
	}
	return false;
}



/*! 
Rounds the given double to the given power of 10.
\param d The number to be rounded.
\param roundingPower The power of 10 to round d to. For example, if roundingPower is 0, d is rounded to the one's place (10^0 == 1).
If roundingPower is -3, d is rounded to the thousandth's place (10^-3 = .001). If roundingPower is 1, d is rounded to the ten's place.
\param c The type of rounding to do, from the CX::CX_RoundingConfiguration enum. You can round up, down, to nearest, and toward zero.
\return The rounded value.
*/
double CX::round (double d, int roundingPower, CX::CX_RoundingConfiguration c) {
	//When <cfenv> becomes availble, use that instead of this stuff.
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