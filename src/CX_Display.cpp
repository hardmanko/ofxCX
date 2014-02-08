#include "CX_Display.h"

#include "ofAppGLFWWindow.h" //This is included here in order to avoid leaking symbols. 
//There are a lot of symbols in this file that should not leak into the user space.

#include "CX_Private.h" //glfwContext

using namespace CX;
using namespace CX::Instances;

CX_Display::CX_Display (void) :
	_framePeriod(0),
	_swapThread(NULL),
	_manualBufferSwaps(0)
{
}

CX_Display::~CX_Display (void) {
	//_swapThread->stopThread();
	//_swapThread->waitForThread(false);
	delete _swapThread;
}

void CX_Display::exit(void) {
	_swapThread->stopThread();
	_swapThread->waitForThread(false);
}


/*! Set up the display. Must be called for the display to function correctly. */
void CX_Display::setup (void) {
	ofSetLogLevel("ofFbo", OF_LOG_WARNING); //It isn't clear that this should be here, but the fbos 
		//are really verbose when allocated and it is a lot of gibberish.

	//setFullScreen(false); //Default to windowed mode (for development). This has no effect, because it is in windowed mode already.

	_renderer = ofGetGLProgrammableRenderer();
	if (!_renderer) {
		Log.warning("CX_Display") << "Programmable renderer not available.";
	}

	_swapThread = new CX_ConstantlySwappingThread(); //This is a work-around for some stupidity in oF or Poco (can't tell which) where 
		//objects inheriting from ofThread cannot be constructed "too early" in program execution (where the quotes mean I have no idea 
		//what too early means) or else there will be a crash.

	BLOCKING_estimateFramePeriod( 300 * 1000 ); //Estimate for 300 ms.
}

/*! Set whether the front and buffers of the display will swap automatically every frame or not.
You can check to see if a swap has occured by calling hasSwappedSinceLastCheck(). You can
check to see if the display is automatically swapping by calling isAutomaticallySwapping().
\param autoSwap If true, the front and back buffer will swap automatically every frame.*/
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

/*! Determine whether the display is configured to automatically swap the front and back buffers
every frame.
See \ref BLOCKING_setSwappingState for more information.
*/
bool CX_Display::isAutomaticallySwapping (void) {
	return _swapThread->isThreadRunning();
}

/*! Get the last time at which the front and back buffers were swapped. 
\return A time value that can be compared with CX::Instances::Clock.getTime(). */
CX_Micros CX_Display::getLastSwapTime (void) {
	return _swapThread->getLastSwapTime();
}

/*! Get an estimate of the next time the front and back buffers will be swapped.
This function depends on the precision of the frame period as estimated using 
BLOCKING_estimateFramePeriod().
\return A time value that can be compared to CX::Instances::Clock.getTime(). */
CX_Micros CX_Display::estimateNextSwapTime(void) {
	return this->getLastSwapTime() + this->getFramePeriod();
	//return _swapThread->estimateNextSwapTime();
}

/*! Gets the estimate of the frame period calculated with BLOCKING_estimateFramePeriod(). */
CX_Micros CX_Display::getFramePeriod(void) {
	return _framePeriod;
}

/*! Check to see if the display has swapped the front and back buffers since the last call to this function.
If isAutomaticallySwapping() is false, the result of this function is meaningless.
\return True if a swap has been made since the last call to this function, false otherwise. */
bool CX_Display::hasSwappedSinceLastCheck (void) {
	return _swapThread->swappedSinceLastCheck();
}

/*! This function returns the number of the last frame presented, as determined by 
number of front and back buffer swaps. It tracks buffer swaps that result from 
1) the front and back buffer swapping automatically (as a result of BLOCKING_setSwappingState(true)) and 
2) manual swaps resulting from a call to BLOCKING_swapFrontAndBackBuffers().
\return The number of the last frame. This value can only be compared with other values 
returned by this function. */
uint64_t CX_Display::getFrameNumber (void) {
	return _swapThread->getFrameNumber() + _manualBufferSwaps;
}

/*! Draw the given ofFbo to the back buffer. It will be drawn starting from 0, 0 and will be drawn at
the full dimensions of the ofFbo (whatever size was chosen at allocation of the fbo). */
void CX_Display::drawFboToBackBuffer (ofFbo &fbo) {
	drawFboToBackBuffer(fbo, ofRectangle(0, 0, fbo.getWidth(), fbo.getHeight() )); //ofGetWidth(), ofGetHeight()?
}

/*! Draw the given ofFbo to the back buffer at the coordinates given by rect.
\param fbo The fbo to draw.
\param rect The rectangle in which to place to fbo. The x and y components specify location. 
The width and height components specify the output width and height of the fbo. If these are
not equal to the width and height of the fbo, the fbo will be scaled up or down to fit the
width and height.
*/
void CX_Display::drawFboToBackBuffer (ofFbo &fbo, ofRectangle rect) {
	beginDrawingToBackBuffer();

	ofSetColor( 255 );
	fbo.draw( rect.x, rect.y, rect.width, rect.height );

	endDrawingToBackBuffer();
}

/*! Prepares a rendering context for using drawing functions. Must be paired with
a call to endDrawingToBackBuffer(). */
void CX_Display::beginDrawingToBackBuffer (void) {
	//_renderer = ofGetGLProgrammableRenderer();

	if(_renderer){
		_renderer->startRender();
	}

	ofViewport();
	ofSetupScreen();
}

/*! Finish rendering to the back buffer. Must be paired with a call to beginDrawingToBackBuffer(). */
void CX_Display::endDrawingToBackBuffer (void) {
	if(_renderer){
		_renderer->finishRender();
	}

	//glFinish(); //Makes sure openGL is completely done before returning. Takes forever.
	glFlush();
}

/*! This function queues up a swap of the front and back buffers then
blocks until the swap occurs. It does nothing if isAutomaticallySwapping() == true. */
void CX_Display::BLOCKING_swapFrontAndBackBuffers (void) {
	if (!isAutomaticallySwapping()) {
		glfwSwapBuffers( CX::Private::glfwContext );
		_manualBufferSwaps++;
	}
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

/*! Returns an ofPoint representing the center of the display. Works in either windowed or full screen mode. */
ofPoint CX_Display::getCenterOfDisplay (void) {
	ofPoint rval;
	rval.x = getResolution().x/2;
	rval.y = getResolution().y/2;
	return rval;
}

/*!
Sets the resolution of the window. Has no effect if called while in full screen mode.
\param width The desired width of the window, in pixels.
\param height The desired height of the window, in pixels.
*/
void CX_Display::setWindowResolution (int width, int height) {
	if (ofGetWindowMode() == OF_WINDOW) {
		ofSetWindowShape( width, height );
	}
}

/*!
This function estimates the typical period of the display refresh.
This function blocks for estimationInterval while the swapping thread swaps in the background.
This function is called with an argument of 300 ms during construction of this class, so
there will always be some information about the frame period. If more precision of the estimate
is desired, this function can be called again with a longer wait duration.
\param estimationInterval The length of time to spend estimating the frame period.
*/
void CX_Display::BLOCKING_estimateFramePeriod (CX_Micros estimationInterval) {
	bool wasSwapping = isAutomaticallySwapping();
	BLOCKING_setSwappingState(false);

	vector<CX_Micros> swapTimes;

	CX_Micros startTime = CX::Instances::Clock.getTime();
	while (CX::Instances::Clock.getTime() - startTime < estimationInterval) {
		BLOCKING_swapFrontAndBackBuffers();
		swapTimes.push_back( CX::Instances::Clock.getTime() );
	}

	if (swapTimes.size() > 1) {
		CX_Micros swapSum = 0;
		for (unsigned int i = 1; i < swapTimes.size(); i++) {
			swapSum += swapTimes[i] - swapTimes[i - 1];
		}

		_framePeriod = swapSum/(swapTimes.size() - 1);
	}
	
	BLOCKING_setSwappingState(wasSwapping);
}

/*! Set whether the display is full screen or not. If the display is set to full screen, 
the resolution may not be the same as the resolution of display in windowed mode, and vice
versa.
*/
void CX_Display::setFullScreen (bool fullScreen) {
	ofSetFullscreen( fullScreen );

	if (fullScreen) {
		glfwSwapInterval(1);
	} else {
		glfwSwapInterval(0); //No v-sync in windowed mode
	}
}