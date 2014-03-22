#include "CX_Display.h"

#include "CX_Private.h" //glfwContext

using namespace CX;
using namespace CX::Instances;

CX_Display::CX_Display (void) :
	_framePeriod(0),
	_swapThread(NULL),
	_manualBufferSwaps(0),
	_frameNumberOnLastSwapCheck(0),
	_softVSyncWithGLFinish(false)
{
}

CX_Display::~CX_Display (void) {
	_swapThread->stopThread();
	_swapThread->waitForThread(false);
	delete _swapThread;
}

/*! Set up the display. Must be called for the display to function correctly. */
void CX_Display::setup (void) {
	ofSetLogLevel("ofFbo", OF_LOG_WARNING); //It isn't clear that this should be here, but the fbos 
		//are really verbose when allocated and it is a lot of gibberish.

	//setFullScreen(false); //Default to windowed mode (for development). This has no effect, because it is in windowed mode already.

	_renderer = ofGetGLProgrammableRenderer();
	if (!_renderer) {
		//Log.warning("CX_Display") << "Programmable renderer not available. Standard renderer will be used instead.";
	}

	_swapThread = new CX_VideoBufferSwappingThread(); //This is a work-around for some stupidity in oF or Poco (can't tell which) where 
		//objects inheriting from ofThread cannot be constructed "too early" in program execution (where the quotes mean I have no idea 
		//what too early means) or else there will be a crash.

	//For some reason, frame period estimation gets screwed up because the first few swaps are way too fast.
	//So swap a few times to clear out the "bad" initial swaps.
	for (int i = 0; i < 5; i++) {
		glfwSwapBuffers(CX::Private::glfwContext);
	}

	BLOCKING_estimateFramePeriod( CX_Millis(500) );
}

/*! Set whether the front and buffers of the display will swap automatically every frame or not.
You can check to see if a swap has occured by calling hasSwappedSinceLastCheck(). You can
check to see if the display is automatically swapping by calling isAutomaticallySwapping().
\param autoSwap If true, the front and back buffer will swap automatically every frame. 
\see \ref blockingCode */
void CX_Display::BLOCKING_setAutoSwapping (bool autoSwap) {
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
See \ref BLOCKING_setAutoSwapping for more information.
*/
bool CX_Display::isAutomaticallySwapping (void) {
	return _swapThread->isThreadRunning();
}

/*! Get the last time at which the front and back buffers were swapped. 
\return A time value that can be compared with CX::Instances::Clock.now(). */
CX_Millis CX_Display::getLastSwapTime(void) {
	return _swapThread->getLastSwapTime();
}

/*! Get an estimate of the next time the front and back buffers will be swapped.
This function depends on the precision of the frame period as estimated using 
BLOCKING_estimateFramePeriod().
\return A time value that can be compared to CX::Instances::Clock.now(). */
CX_Millis CX_Display::estimateNextSwapTime(void) {
	return this->getLastSwapTime() + this->getFramePeriod();
	//return _swapThread->estimateNextSwapTime();
}

/*! Gets the estimate of the frame period calculated with BLOCKING_estimateFramePeriod(). */
CX_Millis CX_Display::getFramePeriod(void) {
	return _framePeriod;
}

/*! Check to see if the display has swapped the front and back buffers since the last call to this function.
This is generally used in conjuction with automatic swapping of the buffers (BLOCKING_setAutoSwapping())
or with an individual threaded swap of the buffers (swapFrontAndBackBuffers()). This technically works
with BLOCKING_swapFrontAndBackBuffers(), but given that that function only returns once the buffers have
swapped, checking that the buffers have swapped is redundant.
\return True if a swap has been made since the last call to this function, false otherwise. */
bool CX_Display::hasSwappedSinceLastCheck (void) {
	bool hasSwapped = false;
	uint64_t currentFrameNumber = this->getFrameNumber();
	if (currentFrameNumber != _frameNumberOnLastSwapCheck) {
		_frameNumberOnLastSwapCheck = currentFrameNumber;
		hasSwapped = true;
	}
	return hasSwapped;
}

/*! This function returns the number of the last frame presented, as determined by 
number of front and back buffer swaps. It tracks buffer swaps that result from 
1) the front and back buffer swapping automatically (as a result of BLOCKING_setAutoSwapping() with true as the argument) and 
2) manual swaps resulting from a call to BLOCKING_swapFrontAndBackBuffers() or swapFrontAndBackBuffers().
\return The number of the last frame. This value can only be compared with other values 
returned by this function. */
uint64_t CX_Display::getFrameNumber (void) {
	return _swapThread->getFrameNumber() + _manualBufferSwaps;
}

/*! Copies an ofFbo to the back buffer using a highly efficient blitting operation. This copies
over the contents of the back buffer, it does not draw over them. For this reason, transparaency
is ignored.
\param fbo The framebuffer to copy. It will be drawn starting from (0, 0) and will be drawn at
the full dimensions of the fbo (whatever size was chosen at allocation of the fbo). 
*/
void CX_Display::copyFboToBackBuffer(ofFbo &fbo) {
	copyFboToBackBuffer(fbo, ofPoint(0, 0));
}

/*! Copies an ofFbo to the back buffer using a highly efficient blitting operation.
\param fbo The framebuffer to copy.
\param destination The point on the back buffer where the fbo will be placed.
*/
void CX_Display::copyFboToBackBuffer(ofFbo &fbo, ofPoint destination) {

	ofRectangle res = this->getResolution();

	GLint copyWidth = std::min(fbo.getWidth(), res.width); //Dimensions must be the same
	GLint copyHeight = std::min(fbo.getHeight(), res.height);

	ofRectangle source(0, 0, copyWidth, copyHeight);
	ofRectangle dest(destination.x, destination.y, copyWidth, copyHeight);

	_blitFboToBackBuffer(fbo, source, dest);
}

/*! Copies an ofFbo to the back buffer using a highly efficient blitting operation.
\param fbo The framebuffer to copy.
\param source A rectangle giving an area of the fbo to copy.
\param destination The point on the back buffer where the area of the fbo will be placed.

If this function does not provide enough flexibility, you can always draw ofFbo's using the following
technique:
\code{.cpp}
Display.beginDrawingToBackBuffer();
ofSetColor( 255 );
fbo.draw( x, y, width, height ); //Replace these variables with the destination location (x,y) and dimensions of the 
Display.endDrawingToBackBuffer();
\endcode
*/
void CX_Display::copyFboToBackBuffer(ofFbo &fbo, ofRectangle source, ofPoint destination) {
	ofRectangle dest(destination.x, destination.y, source.width, source.height);

	_blitFboToBackBuffer(fbo, source, dest);
}



void CX_Display::_blitFboToBackBuffer(ofFbo& fbo, ofRectangle sourceCoordinates, ofRectangle destinationCoordinates) {

	ofRectangle res = this->getResolution();

	GLint sx0 = sourceCoordinates.x;
	GLint sy0 = fbo.getHeight() - sourceCoordinates.y;
	GLint sx1 = sourceCoordinates.x + sourceCoordinates.width;
	GLint sy1 = fbo.getHeight() - sourceCoordinates.y - sourceCoordinates.height;

	GLint dx0 = destinationCoordinates.x;
	GLint dy0 = res.height - destinationCoordinates.y;
	GLint dx1 = destinationCoordinates.x + destinationCoordinates.width;
	GLint dy1 = res.height - destinationCoordinates.y - destinationCoordinates.height;

	ofOrientation orient = ofGetOrientation();

	switch (orient) {
	case OF_ORIENTATION_DEFAULT:
		std::swap(sy0, sy1);
		break;
	case OF_ORIENTATION_180:
		std::swap(sx0, sx1);
		//std::swap(sy0, sy1);
		break;
	case OF_ORIENTATION_90_LEFT:
		//std::swap(y0, y1);
		//break;
	case OF_ORIENTATION_90_RIGHT:
		//std::swap(y0, y1);
		Log.error("CX_Display") << "drawFboToBackBuffer: FBO copy attempted while the orientation was in an unsupported mode."
			" Supported orientations are OF_ORIENTATION_DEFAULT and OF_ORIENTATION_180.";
		break;
	}

	
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo.getFbo());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_BACK);

	glBlitFramebuffer(sx0, sy0, sx1, sy1, dx0, dy0, dx1, dy1, GL_COLOR_BUFFER_BIT, GL_LINEAR);
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
	glFlush();
}

/*! This function queues up a swap of the front and back buffers then
blocks until the swap occurs. It does nothing if isAutomaticallySwapping() == true.
\see \ref blockingCode */
void CX_Display::BLOCKING_swapFrontAndBackBuffers (void) {
	if (!isAutomaticallySwapping()) {
		glfwSwapBuffers(CX::Private::glfwContext);
		if (_softVSyncWithGLFinish) {
			glFinish();
		}
		_manualBufferSwaps++;
	} else {
		Instances::Log.error("CX_Display") << "Manual buffer swap requested with BLOCKING_swapFrontAndBackBuffers while auto swapping mode was in use.";
	}
}

/*! This function cues a swap of the front and back buffers. It avoids blocking
(like BLOCKING_swapFrontAndBackBuffers()) by spawning a thread in which the 
swap is waited for. */
void CX_Display::swapFrontAndBackBuffers (void) {
	_swapThread->swapNFrames(1);
}

/*!
Wait until all OpenGL instructions that were given before this was called to complete. 
This can be useful if you are trying to determine how long a set of rendering commands takes
or need to make sure that all rendering is complete before moving on. 
\see \ref blockingCode
*/
void CX_Display::BLOCKING_waitForOpenGL (void) {
	glFinish();
	/*
	if (CX::Private::glFenceSyncSupported()) {
		GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		glFlush();
		glWaitSync(fence, 0, GL_TIMEOUT_IGNORED);
	} else {
		glFinish(); //I don't see why not to just use glFinish()
	}
	*/
}

/*! Returns the resolution of the current window, not the resolution of the monitor (unless you are in full screen mode).
\return An ofRectangle containing the resolution. The width in pixels is stored in both the width
and x members and the height in pixles is stored in both the height and y members, so you can
use whichever makes the most sense to you. */
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

/*! Sets the title of the experiment window.
\param title The new window title.
*/
void CX_Display::setWindowTitle(std::string title) {
	CX::Private::window->setWindowTitle(title);
}

/*!
This function estimates the typical period of the display refresh.
This function blocks for estimationInterval while the swapping thread swaps in the background.
This function is called with an argument of 300 ms during construction of this class, so
there will always be some information about the frame period. If more precision of the estimate
is desired, this function can be called again with a longer wait duration.
\param estimationInterval The length of time to spend estimating the frame period.
\see \ref blockingCode
*/
void CX_Display::BLOCKING_estimateFramePeriod(CX_Millis estimationInterval) {
	bool wasSwapping = isAutomaticallySwapping();
	BLOCKING_setAutoSwapping(false);

	vector<CX_Millis> swapTimes;

	CX_Millis startTime = CX::Instances::Clock.now();
	while (CX::Instances::Clock.now() - startTime < estimationInterval) {
		BLOCKING_swapFrontAndBackBuffers();
		swapTimes.push_back(CX::Instances::Clock.now());
	}
	
	if (swapTimes.size() > 1) {
		CX_Millis swapSum = 0;
		for (unsigned int i = 1; i < swapTimes.size(); i++) {
			swapSum += swapTimes[i] - swapTimes[i - 1];
		}
		_framePeriod = swapSum / (swapTimes.size() - 1);
	} else {
		Log.warning("CX_Display") << "BLOCKING_estimateFramePeriod: Not enough swaps occured during the " << estimationInterval << " ms estimation interval.";
	}
	
	BLOCKING_setAutoSwapping(wasSwapping);
}

/*! Set whether the display is full screen or not. If the display is set to full screen, 
the resolution may not be the same as the resolution of display in windowed mode, and vice
versa.
*/
void CX_Display::setFullScreen (bool fullScreen) {
	ofSetFullscreen( fullScreen );

	if (fullScreen) {
		setVSync(true);
	} else {
		setVSync(false); //No v-sync in windowed mode
	}
}

/*! Sets whether the display is using V-Sync to control frame presentation.
Without V-Sync, vertical tearing can occur.
\param vSync If true, V-Sync will be used.
\param useSoftwareVSync If true, instead of trying to use hardware to control V-Sync,
the display will attempt to do V-Sync in software.
\note This may not work, depending on your video card settings. Modern video card
drivers allow you to contorl whether V-Sync is used for all applications or not,
or whether the applications are allowed to choose from themselves whether to use
V-Sync. If your drivers are not properly adjusted, this function may have no effect.
Even when the drivers appear to be properly adjusted, it is still possible that this
function will have no effect.
*/
void CX_Display::setVSync(bool vSync, bool useSoftwareVSync) {
	_softVSyncWithGLFinish = useSoftwareVSync;
	if (vSync) {
		glfwSwapInterval(1);
	} else {
		glfwSwapInterval(0);
	}
}