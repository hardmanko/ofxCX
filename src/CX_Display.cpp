#include "CX_Display.h"

#include "ofAppGLFWWindow.h" //This is included here in order to avoid leaking symbols. 
//There are a lot of symbols in this file that should not leak into the user space.

#include "CX_Utilities.h" //glfwContext

using namespace CX;

CX_Display::CX_Display (void)
{
}

CX_Display::~CX_Display (void) {
	//_swapThread->stopThread();
	//_swapThread->waitForThread(false);
	delete _swapThread;
}


void CX_Display::setup (void) {
	ofSetLogLevel("ofFbo", OF_LOG_WARNING); //It isn't clear that this should be here, but the fbos are really verbose when allocated and it is all gibberish.

	setFullScreen(false); //Default to windowed mode (for development).

	_renderer = ofGetGLProgrammableRenderer();

	_swapThread = new CX_ConstantlySwappingThread(); //This is a work-around for some stupidity in OF or Poco (can't tell which) where 
		//objects inheriting from ofThread cannot be constructed "too early" in program execution (where the quotes mean I have no idea what too early means).
	BLOCKING_setSwappingState(true);
	BLOCKING_estimateFramePeriod( 200 * 1000 ); //Estimate for 200 ms.
	BLOCKING_setSwappingState(false);
}

void CX_Display::update (void) {
}

void CX_Display::exit (void) {
	_swapThread->stopThread();
	_swapThread->waitForThread(false);
}

void CX_Display::BLOCKING_setSwappingState (bool autoSwap) {
	if (autoSwap) {
		if (!_swapThread->isThreadRunning()) {
			_swapThread->startThread(true, false); //verbose is true only for testing.
		}
	} else {
		if (_swapThread->isThreadRunning()) {
			_swapThread->stopThread();
			_swapThread->waitForThread(false);
		}
	}
}

bool CX_Display::isAutomaticallySwapping (void) {
	return _swapThread->isThreadRunning();
}

uint64_t CX_Display::getLastSwapTime (void) {
	return _swapThread->getLastSwapTime();
}

bool CX_Display::hasSwappedSinceLastCheck (void) {
	return _swapThread->swappedSinceLastCheck();
}

uint64_t CX_Display::getFramePeriod (void) {
	return _swapThread->getTypicalSwapPeriod();
}

uint64_t CX_Display::getFrameNumber (void) {
	return _swapThread->getFrameNumber();
}

void CX_Display::drawFboToBackBuffer (ofFbo &fbo) {
	beginDrawingToBackBuffer();

	ofSetColor( 255 );
	fbo.draw(0, 0, ofGetWidth(), ofGetHeight()); //Maybe this should be the actual size of the FBO instead.

	endDrawingToBackBuffer();
}

void CX_Display::drawFboToBackBuffer (ofFbo &fbo, ofRectangle rect) {
	beginDrawingToBackBuffer();

	ofSetColor( 255 );
	fbo.draw( rect.x, rect.y, rect.width, rect.height );

	endDrawingToBackBuffer();
}

void CX_Display::beginDrawingToBackBuffer (void) {
	//_renderer = ofGetGLProgrammableRenderer();

	if(_renderer){
		_renderer->startRender();
	}

	ofViewport();
	ofSetupScreen();
}

void CX_Display::endDrawingToBackBuffer (void) {
	if(_renderer){
		_renderer->finishRender();
	}

	//glFinish(); //Makes sure openGL is completely done before returning. Takes forever.
	glFlush();
}

void CX_Display::BLOCKING_swapFrontAndBackBuffers (void) {
	glfwSwapBuffers( CX::Private::glfwContext );
}


ofRectangle CX_Display::getResolution (void) {
	return ofRectangle( ofGetWidth(), ofGetHeight(), ofGetWidth(), ofGetHeight() );
}

void CX_Display::setWindowResolution (int width, int height) {
	if (ofGetWindowMode() == OF_WINDOW) {
		ofSetWindowShape( width, height );
	}
}

/*!
This version of the function just does a spinloop while the swapping thread swaps in the background.

This function does nothing if there is some kind of introduction to the experiment where the swapping thread
will have time to learn what the period is.
*/
void CX_Display::BLOCKING_estimateFramePeriod (uint64_t estimationInterval) {
	uint64_t endTime = CX::Instances::Clock.getTime() + estimationInterval;

	while (endTime > CX::Instances::Clock.getTime())
		;
}

uint64_t CX_Display::estimateNextSwapTime (void) {
	return _swapThread->estimateNextSwapTime();
}

void CX_Display::setFullScreen (bool fullScreen) {
	ofSetFullscreen( fullScreen );

	if (fullScreen) {
		glfwSwapInterval(1);
	} else {
		glfwSwapInterval(0); //No v-sync in windowed mode
	}
}