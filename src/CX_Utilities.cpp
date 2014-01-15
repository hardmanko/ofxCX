#include "CX_Utilities.h"

#include "ofMain.h"

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

std::vector<int> CX::intVector (int rangeBottom, int rangeTop) {
	std::vector<int> rval;

	while (rangeBottom <= rangeTop) {
		rval.push_back(rangeBottom++);
	}

	return rval;
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