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

	BLOCKING_estimateFramePeriod( 200 * 1000 ); //Estimate for 200 ms.
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

/*!
Wait until all OpenGL instructions that were given before
this was called to complete. Any commands put into the pipeline
after this is called (from other threads) are not waited for. 
*/
void CX_Display::BLOCKING_waitForOpenGL (void) {
	GLsync fence = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
	glFlush();
	glWaitSync( fence, 0, GL_TIMEOUT_IGNORED );
}

/*!
Returns the resolution of the current window, not the resolution of the monitor
(unless you are in full screen mode). You can use either x and y or width and height.
*/
ofRectangle CX_Display::getResolution (void) {
	return ofRectangle( ofGetWidth(), ofGetHeight(), ofGetWidth(), ofGetHeight() );
}

/*!
Sets the resolution of the window. Has no effect if called while in full screen mode.
*/
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
	bool wasSwapping = isAutomaticallySwapping();
	BLOCKING_setSwappingState(true);

	uint64_t startTime = CX::Instances::Clock.getTime();
	while (CX::Instances::Clock.getTime() - startTime < estimationInterval)
		;

	BLOCKING_setSwappingState(wasSwapping);
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