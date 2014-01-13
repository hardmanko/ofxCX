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

	//ofSleepMillis(0); //sleep(0) is similar to yield()
}

void CX::Private::App::exit (ofEventArgs &a) {
	Instances::Display.exit();

	ofRemoveListener( ofEvents().exit, this, &Private::App::exit, OF_EVENT_ORDER_APP );
}

void glfwErrorCallback (int code, const char *message) {
	//ofLogError("ofAppGLFWWindow") << "GLFW error code: " << code << " " << message;
	Log.error("ofAppGLFWWindow") << "GLFW error code: " << code << " " << message;
}

int CX::setupWindow (CX_WindowConfiguration_t config) {

	glfwSetErrorCallback( &glfwErrorCallback );

	ofPtr<ofAppGLFWWindow> window( new ofAppGLFWWindow );
	window->setNumSamples( CX::getSampleCount() );

	ofSetCurrentRenderer( (ofPtr<ofBaseRenderer>)(new ofGLRenderer), true );
	ofSetupOpenGL(ofPtr<ofAppBaseWindow>(window), 800, 600, OF_WINDOW);

	ofGetCurrentRenderer()->update(); //Only needed for ofGLRenderer, not for ofGLProgrammableRenderer

	window->initializeWindow();

	CX::Private::glfwContext = glfwGetCurrentContext();

	ofSetOrientation( ofOrientation::OF_ORIENTATION_DEFAULT, true );

	return 0;
}


int main (void) {
	Log.levelForConsole(LogLevel::LOG_ALL);
	Log.levelForFile(LogLevel::LOG_ALL);
	Log.levelForFile(LogLevel::LOG_ALL, "Log for last run.txt");

	ofLogToConsole();
	ofSetLogLevel(OF_LOG_VERBOSE);
	
	ofSetWorkingDirectoryToDefault();

	CX::setupWindow( CX_WindowConfiguration_t() );

	//Why use these? CX has RNG and Clock.
	ofSeedRandom();
	ofResetElapsedTimeCounter();

	CX::Private::App A;
	A.setup();
	Log.flush();
	while(true){
		A.update();
	}
}