#include "CX_Utilities.h"

#include "ofMain.h"
#include "ofConstants.h"
//#include "ofFileUtils.h"

#include "GLFW\glfw3.h"

GLFWwindow *CX::Private::glfwContext;

using namespace std;


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

std::vector<int> CX::intVector (int start, int end) {
	return sequence<int>(start, end, start < end ? 1 : -1);
}

std::vector<unsigned int> CX::uintVector (unsigned int start, unsigned int end) {
	return sequence<unsigned int>(start, end, start < end ? 1 : -1);
}

std::vector<int> CX::intVectorByCount (std::vector<int> counts) {
	std::vector<int> rval;

	for (int i = 0; i < counts.size(); i++) {
		for (int j = 0; j < counts[i]; j++) {
			rval.push_back(i);
		}
	}

	return rval;
}

std::vector<int> CX::intVectorByCountAndValue (std::vector<int> counts, std::vector<int> values) {
	std::vector<int> rval;

	if (counts.size() != values.size()) {
		return rval;
	}

	for (int i = 0; i < counts.size(); i++) {
		for (int j = 0; j < counts[i]; j++) {
			rval.push_back(values.at(i));
		}
	}

	return rval;
}

bool CX::checkOFVersion (int versionMajor, int versionMinor, int versionPatch) {
	if (versionMajor == OF_VERSION_MAJOR && versionMinor == OF_VERSION_MINOR && versionPatch == OF_VERSION_PATCH) {
		return true;
	}
	return false;
}

//Add various log data on failure: why failure?
bool CX::writeToFile (std::string filename, string data, bool append) {
	ofFile out( ofToDataPath(filename), (append ? ofFile::Append : ofFile::WriteOnly), false );
	if (out.is_open()) {
		out << data;
		out.close();
		return true;
	}
	return false;
}