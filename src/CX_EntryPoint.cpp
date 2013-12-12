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

	

	cout << "Enter \"p\" for programmable renderer, anything else for standard renderer." << endl;
	string s;
	cin >> s;
	if (s == "p") {
		ofSetCurrentRenderer( (ofPtr<ofBaseRenderer>)(new ofGLProgrammableRenderer), true );
		ofSetupOpenGL(ofPtr<ofAppBaseWindow>(new ofAppGLFWWindow), 800, 600, OF_WINDOW);
	} else {
		ofSetCurrentRenderer( (ofPtr<ofBaseRenderer>)(new ofGLRenderer), true );
		ofSetupOpenGL(ofPtr<ofAppBaseWindow>(new ofAppGLFWWindow), 800, 600, OF_WINDOW);

		ofGetCurrentRenderer()->update(); //This function should be called "setup"

		ofDisableAlphaBlending();
		ofDisableAntiAliasing();

		//ofDisableArbTex(); //Test to see if this helps FBOs in some cases.
	}


	/*
	bool programmableRendererUsed = true;

	try {
		ofSetCurrentRenderer( (ofPtr<ofBaseRenderer>)(new ofGLProgrammableRenderer), true );
		ofSetupOpenGL(ofPtr<ofAppBaseWindow>(new ofAppGLFWWindow), 800, 600, OF_WINDOW);
		ofLogNotice("CX_EntryPoint") << "Sucessfully got an ofGLProgrammableRenderer";
	} catch (exception e) {
		ofLogError("CX_EntryPoint") << "Exception thrown while getting ofGLProgrammableRenderer: " << e.what();
		programmableRendererUsed = false;
	}

	if (!programmableRendererUsed) {
		try {
			ofSetCurrentRenderer( (ofPtr<ofBaseRenderer>)(new ofGLRenderer), true );
			ofSetupOpenGL(ofPtr<ofAppBaseWindow>(new ofAppGLFWWindow), 800, 600, OF_WINDOW);

			ofGetCurrentRenderer()->update(); //This function should be called "setup"
			ofLogNotice("CX_EntryPoint") << "Sucessfully got an ofGLRenderer";
		} catch (exception e) {
			ofLogFatalError("CX_EntryPoint") << "Exception thrown while getting ofGLRenderer: " << e.what();
		}
	}
	*/

	ofPtr<ofAppBaseWindow> window( ofGetWindowPtr() );
	window->initializeWindow();

	CX::Private::glfwContext = glfwGetCurrentContext();

	ofSetOrientation( ofOrientation::OF_ORIENTATION_DEFAULT, true );

	ofSeedRandom();
	ofResetElapsedTimeCounter();

	CX::Private::App A;
	A.setup();
	while(true){
		A.update();
	}
}