#include "CX_EntryPoint.h"

#include "ofAppGLFWWindow.h"

using namespace CX;

CX_Display Instances::Display;
CX_InputManager Instances::Input;
CX_SlidePresenter Instances::SlidePresenter;
CX_RandomNumberGenerator Instances::RNG;

void CX::Private::App::setup (void) {
	ofAddListener( ofEvents().exit, this, &CX::Private::App::exit, OF_EVENT_ORDER_APP );

	Instances::Display.setup();
	Instances::SlidePresenter.setDisplay( &Instances::Display );

	//Call the user setup function
	setupExperiment();
}

void CX::Private::App::update (void) {
	Instances::Display.update();
	Instances::SlidePresenter.update();
	
	Instances::Input.pollEvents();

	//Call the user update function
	updateExperiment();

	ofSleepMillis(0); //sleep(0) is similar to yield()
}

void CX::Private::App::exit (ofEventArgs &a) {
	Instances::Display.exit();

	ofRemoveListener( ofEvents().exit, this, &Private::App::exit, OF_EVENT_ORDER_APP );
}

void glfwErrorCallback (int code, const char *message) {
	ofLogError("ofAppGLFWWindow") << "GLFW error code: " << code << " " << message;
}


int main (void) {

	ofSetLogLevel(OF_LOG_VERBOSE);
	
	ofSetWorkingDirectoryToDefault();

	glfwSetErrorCallback( &glfwErrorCallback );

	
	//It seems like all of this should happen in CX_Display::setup.
	//cout << "Enter \"p\" for programmable renderer, anything else for standard renderer." << endl;
	//string s;
	//cin >> s;
	//if (s == "p") {
	//	ofSetCurrentRenderer( (ofPtr<ofBaseRenderer>)(new ofGLProgrammableRenderer), true );
	//	ofSetupOpenGL(ofPtr<ofAppBaseWindow>(new ofAppGLFWWindow), 800, 600, OF_WINDOW);
	//} else {

	ofPtr<ofAppGLFWWindow> window( new ofAppGLFWWindow );
	window->setNumSamples(4);

		ofSetCurrentRenderer( (ofPtr<ofBaseRenderer>)(new ofGLRenderer), true );
		ofSetupOpenGL(ofPtr<ofAppBaseWindow>(window), 800, 600, OF_WINDOW);

		ofGetCurrentRenderer()->update(); //This function should be called "setup". This only needs to be called for

		//ofDisableAlphaBlending(); //This would be more consistent with ofFbo, which does not do alpha blending properly.
		//ofDisableArbTex(); //Test to see if this helps FBOs in some cases.
	//}

	window->initializeWindow();

	CX::Private::glfwContext = glfwGetCurrentContext();

	ofSetOrientation( ofOrientation::OF_ORIENTATION_DEFAULT, true );

	//Why use these? CX has RNG and Clock.
	ofSeedRandom();
	ofResetElapsedTimeCounter();

	CX::Private::App A;
	A.setup();
	while(true){
		A.update();
	}
}