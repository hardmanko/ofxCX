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
}

void CX::Private::App::exit (ofEventArgs &a) {
	Instances::Display.exit();

	ofRemoveListener( ofEvents().exit, this, &Private::App::exit, OF_EVENT_ORDER_APP );
}


int main (void) {
	
	ofSetWorkingDirectoryToDefault();

	ofSetupOpenGL(1024, 768, OF_WINDOW);

	ofPtr<ofAppBaseWindow> window( ofGetWindowPtr() );
	window->initializeWindow();
	

	CX::Private::glfwContext = glfwGetCurrentContext();

	ofSetOrientation( ofOrientation::OF_ORIENTATION_DEFAULT, true ); //I have no idea why this has to be done now, but not before.

	ofSeedRandom();
	ofResetElapsedTimeCounter();

	CX::Private::App A;
	A.setup();
	while(true){
		A.update();
	}
}