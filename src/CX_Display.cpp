#include "CX_Display.h"

#include "CX_Private.h" //glfwContext

/*! An instance of CX::CX_Display that is lightly hooked into the CX backend. The only thing that happens outside of user code
is that during CX setup, before reaching user code in runExperiment(), CX_Display::setup() is called.
\ingroup entryPoint */
CX::CX_Display CX::Instances::Disp;

namespace CX {

CX_Display::CX_Display(void) :
	_framePeriod(0),
	_framePeriodStandardDeviation(0),
	_softVSyncWithGLFinish(false),
	_dispThread(this, &CX_Display::_swapBuffers)
{}

CX_Display::~CX_Display(void) {
}

/*! Set up the display. Must be called for the display to function correctly. 
This is called during CX setup; the user should not need to call it. */
void CX_Display::setup(void) {

	_renderer = CX::Private::appWindow->renderer();

	_dispThread.setup(CX_DisplayThread::Configuration(), false);

	// Use a plausible default frame period
	_setupSwapTracking(CX_Seconds(1.0 / 60.0));
}

void CX_Display::_setupSwapTracking(CX_Millis nominalFramePeriod) {

	// Set up DataContainer
	Sync::DataContainer::Configuration sdc;

	sdc.latency = 0;
	sdc.unitsPerSwap = 1;
	sdc.nominalSwapPeriod = nominalFramePeriod;
	sdc.sampleSize = 0; // let it be set by users

	swapData.setup(sdc);


	// Set up DataClient
	Sync::DataClient::Configuration dcc;

	dcc.dataContainer = &swapData;
	dcc.autoUpdate = false;
	dcc.swapPeriodTolerance = 0.2; // low-ish tolerance
	dcc.dataCollectionDuration = CX_Millis(250);

	swapClient.setup(dcc);

	// Initialize the polled swap listener
	_polledSwapListener = swapData.getPolledSwapListener();
}


/*! This function exists to serve a per-computer configuration function that is otherwise difficult to provide
due to the fact that C++ programs are compiled to binaries and cannot be easily edited on the computer on which
they are running. This function takes the file name of a specially constructed configuration file and reads the
key-value pairs in that file in order to configure the CX_Display. The format of the file is provided in the 
example below:

\code
display.windowWidth = 600
display.windowHeight = 300
display.fullscreen = false
display.hardwareVSync = true
//display.softwareVSync = false //Commented out: no change
//display.swapAutomatically = false //Commented out: no change
\endcode

All of the configuration keys are used in this example.
Configuration options can be omitted, in which case there is no change in the configuration of the CX_Display for that option.
Note that the "display" prefix allows this configuration to be embedded in a file that also performs other configuration functions.

Because this function uses CX::Util::readKeyValueFile() internally, it has the same arguments.
\param filename The name of the file containing configuration data.
\param delimiter The string that separates the key from the value. In the example, it is "=", but can be other values.
\param trimWhitespace If `true`, whitespace characters surrounding both the key and value will be removed. This is a good idea to do.
\param commentString If `commentString` is not the empty string (""), everything on a line
following the first instance of `commentString` will be ignored.
*/
void CX_Display::configureFromFile(std::string filename, std::string delimiter, bool trimWhitespace, std::string commentString) {

	// TODO: This function needs to be redone. Probably move it to be a function of Configuration.

	std::map<std::string, std::string> kv = Util::readKeyValueFile(filename, delimiter, trimWhitespace, commentString);

	if (kv.find("display.fullscreen") != kv.end()) {
		int result = Private::stringToBooleint(kv["display.fullscreen"]);
		if (result != -1) {
			this->setFullscreen(result == 1);
		}
	}

	if (kv.find("display.windowWidth") != kv.end()) {
		int width = ofFromString<int>(kv.at("display.windowWidth"));
		this->setWindowResolution(width, this->getResolution().height);
	}

	if (kv.find("display.windowHeight") != kv.end()) {
		int height = ofFromString<int>(kv.at("display.windowHeight"));
		this->setWindowResolution(this->getResolution().width, height);
	}

	if (kv.find("display.hardwareVSync") != kv.end()) {
		int result = Private::stringToBooleint(kv["display.hardwareVSync"]);
		if (result != -1) {
			this->useHardwareVSync(result == 1);
		}
	}

	if (kv.find("display.softwareVSync") != kv.end()) {
		int result = Private::stringToBooleint(kv["display.softwareVSync"]);
		if (result != -1) {
			this->useSoftwareVSync(result == 1);
		}
	}

	if (kv.find("display.swapAutomatically") != kv.end()) {
		int result = Private::stringToBooleint(kv["display.swapAutomatically"]);
		if (result != -1) {
			this->setAutomaticSwapping(result == 1);
		}
	}
}


CX_DisplayThread* CX_Display::getDisplayThread(void) {
	return &_dispThread;
}


/*! Set whether the front and buffers of the display will swap automatically every frame or not.
You can check to see if a swap has occured by calling hasSwappedSinceLastCheck(). You can
check to see if the display is automatically swapping by calling isAutomaticallySwapping().
\param autoSwap If true, the front and back buffer will swap automatically every frame.
\note This function may \ref blockingCode "block" for up to 1 frame to due the requirement that it synchronize with the thread. */
bool CX_Display::setAutomaticSwapping(bool autoSwap) {

	if (autoSwap == _dispThread.isThreadRunning()) {
		return true;
	}

	if (autoSwap) {
		_dispThread.startThread();
	} else {
		_dispThread.stopThread(true);
	}
	
	return true;

	//return enableDisplayThread(autoSwap);
}


/*! Determine whether the display is configured to automatically swap the front and back buffers
every frame.
See \ref setAutomaticSwapping for more information. */
bool CX_Display::isAutomaticallySwapping(void) {
	return _dispThread.isThreadRunning();
}


bool CX_Display::renderingOnThisThread(void) {
	return Private::glfwContextManager.isLockedByThisThread();
}

bool CX_Display::renderingOnMainThread(void) {
	return Private::glfwContextManager.isLockedByMainThread();
}


/*! This function returns the number of the last frame presented, as determined by
number of front and back buffer swaps. It tracks buffer swaps that result from
1) the front and back buffer swapping automatically (as a result of \ref CX_Display::setAutomaticSwapping "setAutomaticSwapping(true)") and
2) manual swaps resulting from a call to swapBuffers() or swapBuffersInThread().
\return The number of the last frame. This value can only be compared with other values
returned by this function. */
FrameNumber CX_Display::getLastFrameNumber(void) {
	return swapData.getLastSwapData().unit;
}


/*! Get the last time at which the front and back buffers were swapped.
\return A time value that can be compared with CX::Instances::Clock.now(). */
CX_Millis CX_Display::getLastSwapTime(void) {
	return swapData.getLastSwapData().time;
}

/*! Get an estimate of the next time the front and back buffers will be swapped.
This function depends on the precision of the frame period as estimated using
estimateFramePeriod(). 

If the front and back buffers are not swapped every frame (e.g. as a result of 
calling `setAutomaticSwapping(true)`), the result of this function is meaningless 
because it uses the last buffer swap time as a reference.

\return A time value that can be compared to CX::Instances::Clock.now(). */
CX_Millis CX_Display::getNextSwapTime(void) {
	return this->getLastSwapTime() + this->getFramePeriod();
}



/*! Check to see if the display has swapped the front and back buffers since the last call to this function.
This is generally used in conjuction with automatic swapping of the buffers (`setAutomaticSwapping()`)
or with an individual threaded swap of the buffers (`swapBuffersInThread()`). This technically works
with `swapBuffers()`, but given that that function only returns once the buffers have
swapped, using this function to check that the buffers have swapped is redundant.

If you want to be able to call this function from multiple different places and want each callsite
to get a result independent of the other callstites, see `Disp.swapData.getPolledEventListener()`.

\return `true` if a swap has been made since the last call to this function, `false` otherwise. */
bool CX_Display::hasSwappedSinceLastCheck(void) {
	return _polledSwapListener->hasSwappedSinceLastCheck();
}

/*! If the display is automatically swapping, this function blocks until a buffer swap has ocurred. If the
display is not automatically swapping, it returns immediately. 

\return `true` if the display swapped during the timeout period. `false` if there was a timeout or if an error.
*/
bool CX_Display::waitForBufferSwap(CX_Millis timeout, bool reset) {
	if (!isAutomaticallySwapping()) {
		CX::Instances::Log.warning("CX_Display") << "waitForBufferSwap(): Wait requested while not swapping in secondary thread. Returning immediately.";
		return false;
	}

	return _polledSwapListener->waitForSwap(timeout, reset);
}

/*! Prepares a rendering context for using drawing functions. Must be paired with
a call to endDrawingToBackBuffer().
\code{.cpp}
Disp.beginDrawingToBackBuffer();
//Draw stuff...
Disp.endDrawingToBackBuffer();
\endcode
*/
void CX_Display::beginDrawingToBackBuffer(void) {

	if (!Private::glfwContextManager.isLockedByThisThread()) {
		if (Private::glfwContextManager.isUnlocked()) {
			Instances::Log.warning("CX_Display") << "beginDrawingToBackBuffer() called on a thread in which the rendering context was not current"
				" while the rendering context was unlocked. The rendering context was made current and locked.";
			Private::glfwContextManager.lock();
		} else {
			Instances::Log.error("CX_Display") << "beginDrawingToBackBuffer() called on a thread in which the rendering context was not current"
				" while the rendering context was locked by another thread. Nothing will be rendered.";
			return;
		}
	}

	if (_renderer) {
		_renderer->startRender();
	}

	ofViewport();
	ofSetupScreen();
}

/*! Finish rendering to the back buffer. Must be paired with a call to beginDrawingToBackBuffer(). */
void CX_Display::endDrawingToBackBuffer(void) {

	if (!Private::glfwContextManager.isLockedByThisThread()) {
		return;
	}

	if (_renderer) {
		_renderer->finishRender();
	}

	glFlush(); // This is very important, because it seems like commands are buffered in a thread-local fashion initially.
		// As a result, if a swap is requested from a different thread than the rendering thread, the automatic flush
		// that supposedly happens when a swap is queued may not flush commands from the rendering thread. Calling glFlush
		// here helps guarantee that the rendering thread's commands will be executed before the swapping thread queues the swap.
}

/*! This function queues up a swap of the front and back buffers then
blocks until the swap occurs. This usually should not be used if
`isAutomaticallySwapping() == true`. If it is, a warning will be logged.
\see \ref blockingCode */
void CX_Display::swapBuffers(void) {
	
	if (isAutomaticallySwapping()) {
		Instances::Log.error("CX_Display") << "swapBuffers(): Manual buffer swap requested "
			"while automatic buffer swapping mode was in use. The manual swap has been ignored.";
		return;
	}
	
	_swapBuffers();
}

void CX_Display::_swapBuffers(void) {

	Private::CX_GlfwContextManager& cm = Private::glfwContextManager;
	if (!cm.isLockedByThisThread()) {
		Instances::Log.warning("CX_Display") << "swapBuffers(): Buffer swap requested in a thread that doesn't have a lock on the context.";
		return;
	}

	glfwSwapBuffers(cm.get());
	if (_softVSyncWithGLFinish) { // thread? std::atomic?
		glFinish();
	}

	swapData.storeSwap(Instances::Clock.now());
}


/*! This function cues `count` swaps of the front and back buffers. It avoids blocking
(like `swapBuffers()` does) by spawning a thread in which the swap is waited for. 
This does not make it obviously better than swapBuffers(),
because spawning a thread has a cost and may introduce synchronization problems. Also,
because this function does not block, in order to know when the buffer swap took place,
you need to check `hasSwappedSinceLastCheck()`. 

\param count The number of swaps that should be done.


void CX_Display::swapBuffersInThread(unsigned int count) {
	if (isAutomaticallySwapping()) {
		Instances::Log.error("CX_Display") << "swapBuffersInThread(): Queued buffer swap(s) requested "
			"while automatic buffer swapping mode was in use. The queued swaps have been ignored.";
		return;
	}

	if (!_dispThread->isRunning()) {

		_dispThread->setSwapContinuously(false);
		_dispThread->start();

	}

	_dispThread->queueSwaps(count);

}
*/

/*! This function blocks until all OpenGL instructions that were given before this was called to complete.
This can be useful if you are trying to determine how long a set of rendering commands takes
or need to make sure that all rendering is complete before moving on with other tasks.
To demystify things, this function simply calls glFinish().
\see \ref blockingCode */
void CX_Display::waitForOpenGL(void) {
	glFinish();
}

/*! Returns the resolution of the current display area. If in windowed mode, this will return the resolution
of the window. If in full screen mode, this will return the resolution of the monitor.
\return An ofRectangle containing the resolution. The width in pixels is stored in both the `width`
and `x` members and the height in pixles is stored in both the `height` and `y` members, so you can
use whichever makes the most sense to you. */
ofRectangle CX_Display::getResolution(void) const {
	return ofRectangle( ofGetWidth(), ofGetHeight(), ofGetWidth(), ofGetHeight() );
}

/*! Returns an ofPoint representing the center of the display. Works in both windowed and full screen mode. */
ofPoint CX_Display::getCenter(void) const {
	ofRectangle res = getResolution();
	return ofPoint(res.x / 2, res.y / 2);
}

/*!
Sets the resolution of the window. Has no effect if called while in full screen mode.
\param width The desired width of the window, in pixels.
\param height The desired height of the window, in pixels.
*/
void CX_Display::setWindowResolution(int width, int height) {
	if (width <= 0 || height <= 0) {
		CX::Instances::Log.error("CX_Display") << "setWindowResolution: width and height must be > 0. Given width == " <<
			width << " and height == " << height << ".";
		return;
	}

	if (ofGetWindowMode() == OF_WINDOW) {
		ofSetWindowShape(width, height);
	}
}


/*! Set whether the display is full screen or not. If the display is set to full screen,
the resolution may not be the same as the resolution of display in windowed mode, and vice
versa. */
void CX_Display::setFullscreen(bool fullscreen) {
	ofSetFullscreen(fullscreen);
}

/*! \brief Returns `true` if the display is in full screen mode, false otherwise. */
bool CX_Display::isFullscreen(void) {
	return (ofGetWindowMode() == OF_FULLSCREEN);
}

/*! Minimizes or restores the window, depending on the value of `minimize`. 
\param minimize If `true`, the window will be minimized. If `false`, the window will be restored.
*/
void CX_Display::setMinimized(bool minimize) {

	Private::CX_GlfwContextManager& cm = CX::Private::glfwContextManager;
	
	if (!cm.isMainThread() || (!cm.isLockedByThisThread() && cm.isLockedByAnyThread())) {
		return;
	}

	GLFWwindow* ctx = Private::glfwContextManager.get();

	if (minimize) {
		glfwIconifyWindow(ctx);
	} else {
		glfwRestoreWindow(ctx);
	}
}

/*! Sets whether the display is using hardware VSync to control frame presentation.
Without some form of Vsync, vertical tearing may occur.

\param use If `true`, hardware Vsync will be enabled in the video card driver. If `false`, it will be disabled.

\note This may not work, depending on your video card settings. Modern video card
drivers allow you to control whether Vsync is used for all applications or not,
or whether the applications are allowed to choose from themselves whether to use
Vsync. If your drivers are set to force Vsync to a particular setting, this function
is unlikely to have an effect. Even when the drivers allow applications to choose
a Vsync setting, it is still possible that this function will have not have the
expected effect. OpenGL seems to struggle with VSync.

\see See \ref visualStimuli for information on what Vsync is. */
void CX_Display::useHardwareVSync(bool use) {
	if (Private::glfwContextManager.isLockedByThisThread()) {
		glfwSwapInterval(use ? 1 : 0);
	} else if (_dispThread.threadOwnsRenderingContext()) {
		_dispThread.commandSetSwapInterval(use, true);
	} else {
		// fail
	}
}

/*! Sets whether the display is using software VSync to control frame presentation.
Without some form of Vsync, vertical tearing can occur. Hardware VSync, if available,
is generally preferable to software VSync, so see useHardwareVSync() as well. However,
software and hardware VSync are not mutally exclusive, sometimes using both together works
better than only using one.
\param use If `true`, the display will attempt to do VSync in software.
\see See \ref visualStimuli for information on what Vsync is. */
void CX_Display::useSoftwareVSync(bool use) {
	_softVSyncWithGLFinish = use;

	//_dispThread._setGLFinishAfterSwap(use);
}

//bool CX_Display::usingHardwareVSync(void) {
//	return false;
//}

bool CX_Display::usingSoftwareVSync(void) {
	return _softVSyncWithGLFinish;
}

/*! Makes an ofFbo with dimensions equal to the size of the current display with standard settings
and allocates memory for it. The FBO is configured to use RGB plus alpha for the color settings. 
The MSAA setting for the FBO is set to the current value returned by CX::Util::getMsaaSampleCount(). 
This is to help the FBO have the same settings as the front and back buffers so that rendering into 
the FBO will produce the same output as rendering into the back buffer.
\return The configured FBO.
*/
ofFbo CX_Display::makeFbo(void) {
	ofFbo fbo;
	ofRectangle dims = this->getResolution();
	fbo.allocate(dims.width, dims.height, GL_RGBA, CX::Util::getMsaaSampleCount());
	return fbo;
}

/*! Copies an `ofFbo` to the back buffer using a potentially very slow but pixel-perfect blitting operation.
The slowness of the operation is hardware-dependent, with older hardware often being faster at this operation.
Generally, you should just draw the `ofFbo` directly using its `draw()` function.

\note This function overwrites the contents of the back buffer, it does not draw over them. For this reason, transparaency
is ignored.

\param fbo The framebuffer to copy. It will be drawn starting from (0, 0) and will be drawn at
the full dimensions of the fbo (whatever size was chosen at allocation of the fbo).
*/
void CX_Display::copyFboToBackBuffer(ofFbo &fbo) {
	copyFboToBackBuffer(fbo, ofPoint(0, 0));
}

/*! Copies an `ofFbo` to the back buffer using a potentially very slow but pixel-perfect blitting operation.
The slowness of the operation is hardware-dependent, with older hardware often being faster at this operation.
Generally, you should just draw the `ofFbo` directly using its `draw()` function.

\note This function overwrites the contents of the back buffer, it does not draw over them. For this reason, transparaency
is ignored.

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

/*! Copies an `ofFbo` to the back buffer using a potentially very slow but pixel-perfect blitting operation.
The slowness of the operation is hardware-dependent, with older hardware often being faster at this operation.
Generally, you should just draw the `ofFbo` directly using its `draw()` function.

\note This function overwrites the contents of the back buffer, it does not draw over them. For this reason, transparaency
is ignored.

\param fbo The framebuffer to copy.
\param source A rectangle giving an area of the fbo to copy.
\param destination The point on the back buffer where the area of the fbo will be placed.

If this function does not provide enough flexibility, you can always draw ofFbo's using the following
technique, which allows for transparency:
\code{.cpp}
Disp.beginDrawingToBackBuffer();
ofSetColor( 255 ); //If the color is not set to white, the fbo will be drawn mixed with whatever the current color is.
fbo.draw( x, y, width, height ); //Replace these variables with the destination location (x,y) and dimensions of the FBO.
Disp.endDrawingToBackBuffer();
\endcode
*/
void CX_Display::copyFboToBackBuffer(ofFbo &fbo, ofRectangle source, ofPoint destination) {
	ofRectangle dest(destination.x, destination.y, source.width, source.height);

	_blitFboToBackBuffer(fbo, source, dest);
}

//It turns out that this is a very slow operation, in spite of the fact that it is just copying data.
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
		break;
	case OF_ORIENTATION_90_LEFT:
	case OF_ORIENTATION_90_RIGHT:
    case OF_ORIENTATION_UNKNOWN:
        CX::Instances::Log.error("CX_Display") << "drawFboToBackBuffer: FBO copy attempted while the orientation was in an unsupported mode."
			" Supported orientations are OF_ORIENTATION_DEFAULT and OF_ORIENTATION_180.";
		break;
	}

	glDrawBuffer(GL_BACK);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo.getFbo());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); //GL_BACK

	glBlitFramebuffer(sx0, sy0, sx1, sy1, dx0, dy0, dx1, dy1, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

/*! Set whether the y-axis vales should increase upwards.

Be careful when using this function because not all drawing functionality works correctly.

\note Another way to work within a modified display coordinate system is CX::Util::CX_CoordinateConverter.

\param upwards If `true`, y-values will increase upwards. If `false`, y-values will increase downwards (the default).

void CX_Display::setYIncreasesUpwards(bool upwards) {
	//when vFlip is true, y-values increase downwards.
	ofSetOrientation(ofGetOrientation(), !upwards);
}
*/

/*! \brief Do y-axis values increase upwards? 
bool CX_Display::getYIncreasesUpwards(void) const {
	//when vFlip is true, y-values increase downwards.
	return !ofIsVFlipped();
}
*/

/*! \brief Get a `shared_ptr` to the renderer used by the CX_Display. */
std::shared_ptr<ofBaseRenderer> CX_Display::getRenderer(void) {
	return _renderer;
}

/*! This function estimates the typical period of the display refresh.
This function blocks for estimationInterval while the swapping thread swaps in the background (see \ref blockingCode).
This function is called during setup of this class, so
there will always be some information about the frame period. If more precision of the estimate
is desired, this function can be called again with a longer wait duration.

\param estimationInterval The length of time to spend estimating the frame period.
\param minRefreshRate The minimum allowed refresh rate, in Hz. If an observed duration is less than 1/minRefreshRate seconds,
it will be ignored for purposes of estimating the frame period.
\param maxRefreshRate The maximum allowed refresh rate, in Hz. If an observed duration is greater than 1/minRefreshRate seconds,
it will be ignored for purposes of estimating the frame period.
*/
void CX_Display::estimateFramePeriod(CX_Millis estimationInterval, float minRefreshRate, float maxRefreshRate) {
	bool wasSwapping = isAutomaticallySwapping();
	setAutomaticSwapping(false);

	std::vector<CX_Millis> swapTimes;

	CX_Millis minFramePeriod = CX_Seconds((1.0 / maxRefreshRate));
	CX_Millis maxFramePeriod = CX_Seconds((1.0 / minRefreshRate));

	//For some reason, frame period estimation gets screwed up because the first few swaps are way too fast
	//if the buffers haven't been swapping for some time, so swap a few times to clear out the "bad" initial swaps.
	for (int i = 0; i < 3; i++) {
		swapBuffers();
	}

	CX_Millis startTime = CX::Instances::Clock.now();
	while (CX::Instances::Clock.now() - startTime < estimationInterval) {
		swapBuffers();
		swapTimes.push_back(CX::Instances::Clock.now());
	}

	if (swapTimes.size() >= 2) {

		std::vector<CX_Millis> cleanedDurations;

		std::vector<CX_Millis> excludedDurations;

		for (unsigned int i = 1; i < swapTimes.size(); i++) {
			CX_Millis dur = swapTimes[i] - swapTimes[i - 1];

			if (dur >= minFramePeriod && dur <= maxFramePeriod) {
				cleanedDurations.push_back(dur);
			} else {
				excludedDurations.push_back(dur);
			}
		}

		if (cleanedDurations.size() >= 2) {

			// Save the results
			setFramePeriod(Util::mean(cleanedDurations), false);
			_framePeriodStandardDeviation = CX_Millis::standardDeviation(cleanedDurations);

		} else {
			CX::Instances::Log.error("CX_Display") << "estimateFramePeriod(): Not enough valid swaps occurred during the " <<
				estimationInterval << " ms estimation interval. If the estimation interval was very short (less than 50 ms), you "
				"could try making it longer. If the estimation interval was longer, this is an indication that there is something "
				"wrong with the video card configuration. Try using CX_Display::testBufferSwapping() to narrow down the source "
				"of the problems.";
		}

		if (excludedDurations.size() > 0) {
			unsigned int totalExcludedDurations = excludedDurations.size();

			if (excludedDurations.size() > 20) {
				excludedDurations.resize(20);
			}
			CX::Instances::Log.warning("CX_Display") << "estimateFramePeriod(): " << totalExcludedDurations << " buffer swap durations were " <<
				"outside of the allowed range of " << minFramePeriod << " ms to " << maxFramePeriod << " ms. The" <<
				((totalExcludedDurations == excludedDurations.size()) ? "" : " first 20") << " excluded durations were: " <<
				CX::Util::vectorToString(excludedDurations, ", ", 5);
		}


	} else {
		CX::Instances::Log.error("CX_Display") << "estimateFramePeriod(): Not enough buffer swaps occurred during the " <<
			estimationInterval << " ms estimation interval. If the estimation interval was very short (less than 50 ms), "
			"you should try making it longer.";
	}

	setAutomaticSwapping(wasSwapping);
}

/*! Gets the estimate of the frame period estimated with CX_Display::estimateFramePeriod(). */
CX_Millis CX_Display::getFramePeriod(void) const {
	return _framePeriod;
}

/*! Gets the sample standard deviation of the frame period estimated with CX_Display::estimateFramePeriod(). */
CX_Millis CX_Display::getFramePeriodStandardDeviation(void) const {
	return _framePeriodStandardDeviation;
}

/*! During setup, CX tries to estimate the frame period of the display using CX::CX_Display::estimateFramePeriod().
However, this does not always work, and the estimated value is wrong.
If you know that this is happening, you can use this function to set the correct frame period. A typical call might be
\code{.cpp}
Disp.setFramePeriod(CX_Seconds(1.0/60.0));
\endcode
to set the frame period for a 60 Hz refresh cycle. However, note that this will not fix the underlying problem
that prevented the frame period from being estimated correctly, which usually has to do with problems with
the video card doing vertical synchronization incorrectly. Thus, this may not fix anything.
\param knownPeriod The known refresh period of the monitor.
\note This function sets the standard deviation of the frame period to 0.
*/
void CX_Display::setFramePeriod(CX_Millis knownPeriod, bool setupSwapTracking) {
	_framePeriod = knownPeriod;
	_framePeriodStandardDeviation = 0;

	if (setupSwapTracking) {
		_setupSwapTracking(knownPeriod);
	}
}

/*! Epilepsy warning: This function causes your display to rapidly flash with high-contrast patterns.

This function tests buffer swapping under various combinations of Vsync setting and whether the swaps
are requested in the main thread or in a secondary thread. The tests combine visual inspection and automated
time measurement. The visual inspection is important because what the computer is told to put on the screen
and what is actually drawn on the screen are not always the same. It is best to run the tests in full screen
mode, although that is not enforced. At the end of the tests, the results of the tests are provided to
you to interpret based on the guidelines described here. The outcome of the test will usually be that there
are some modes that work better than others for the tested computer.

In the resulting data, there are three test conditions. "thread" indicates whether the main thread or a 
secondary thread was used. "hardVSync" and "softVSync" indicate whether hardware or software Vsync were 
enabled for the test (see CX_Display::useHardwareVSync() and CX_Display::useSoftwareVSync()).
Other columns, giving data from the tests, are explained below. Whatever combination of Vsync works
best can be set up for use in experiments using CX_Display::useHardwareVSync() and CX_Display::useSoftwareVSync() 
to set the Vsync mode in code or with CX_Display::configureFromFile() to set the values based on a configuration 
file. 

The threading mode that is used in stimulus presentation is primarily determined by CX_SlidePresenter with 
the CX::CX_SlidePresenter::Configuration::SwappingMode setting, although some experiments might want to use 
threaded swaps directly. If you are not using a multi-threaded swapping mode with a CX_SlidePresenter, you
probably don't need to do these tests with a secondary thread, which you can do by setting the argument 
`testSecondaryThread` to false when you call this function.

Continuous swapping test
--------------------------------
This test examines the case of constantly swapping the front and back buffers. It measures the amount of time
between swaps, which should always approximately equal the frame period. The raw data from this test can be found
in the "continuousSwapping" CX_DataFrame in the returned map. The raw data are in flat field format, with the 
duration data in the "duration" column and the test conditions in the "hardVSync", "softVSync", and "thread" columns.
A summary of this test can be found in the "summary" data frame in the returned map. In the summary, columns related
to this test are prefixed with "cs" and give the mean, standard deviation, minimum, and maximum swap duration in each 
of the conditions that were tested.

If the swapping durations are not very consistent, which can be determined by visual examination and by looking at 
the standard deviation, min, and max, then there is a problem with the configuration. If the mean duration 
is different from the monitor's actual refresh period, then there is a serious problem with the configuration.

During this test, you should see the screen very rapidly flickering between black and white, so that it might nearly 
appear to be a shade of grey. If you see slow flickering or solid black or white, that is an error. If there are 
horizontal lines that alternate black and white, that is a signature of vertical tearing, which is an error (except
for when both kinds of Vsync are turned off, in which case it is allowable and a good demonstration of the value of Vsync). 

Wait swap test
--------------------------------
One case that this function checks for is what happens if a swap is requested after a long period of
no swaps being requested. In particular, this function swaps, waits for 2.5 swap periods and then swaps twice in a row.
The idea is that there is a long delay between the first swap (the "long" swap) and the second swap (the "short" swap),
followed by a standard delay before the third swap (the "normal" swap). The raw swap durations for this test can be 
found in the "waitSwap" data frame in the returned map, with the test conditions given in the "hardVSync", "softVSync", 
and "thread" columns. The "type" column indicates whether a given swap duration was long, short, or normal and the 
"duration" column gives the durations of the swaps. Summary data from this test can be found in the "summary" data 
frame in the returned map. The columns in the summary data that correspond to this test are prefixed "ws".

There are graded levels of success in this test. Complete success is when the duration of the first swap is 3P,
where P is the standard swap period (i.e. the length of one frame), and the duration of both of the second two swaps is 1P.
Partial success is if the duration of the long swap is ~2.5P, the duration of the short swap is ~.5P, and the duration
of the normal swap is 1P. In this case, the short swap at least gets things back on the right track.
Failure occurs if the short swap duration is ~0P. Mega-failure occurs if the normal swap duration is ~0P. In this
case, it is taking multiple repeated swaps in order to regain vertical synchronization, which is unacceptable behavior.

You can visually check these results. During this test, an attempt is made to draw three bars on the left,
middle, and right of the screen. The left bar is drawn for the long duration, the middle bar for the short duration,
and the right bar for the normal duration.
Complete success results in all three bars flickering on and off (although you still need to check the timing data).
Partial success results in only the left and right bars flickering with the middle bar location flat black.
For the partial success case, the middle bar is never visible because at the time at which it is swapped in,
the screen is in the middle of a refresh cycle. When the next refresh cycle starts, then the middle bar can
start to be drawn to the screen. However, before it has a chance to be drawn, the right rectangle is drawn
to the back buffer, overwriting the middle bar.

If there are horizontal lines that alternate between black and white, that is a sign of vertical tearing, which is
an error unless both type of Vsync are disabled, in which case tearing is to be expected.

Note: The wait swap test is not performed for the secondary thread, because the assumption is that if the secondary
thread is used, in that thread the front and back buffers will be swapped constantly in the secondary thread so 
there will be no wait swaps. You can enable constant swapping in the secondary thread with CX_Display::setAutomaticSwapping().

Remedial measures
-----------------

If all of the tests fail, there are a number of possible reasons. 

One of the primary reasons for failure is that the video card driver is not honoring the requested vertical 
synchronization settings that CX tries during the test. A workaround for this issue is to force vertical 
synchronization on in the video driver settings, which can be done through the GUIs for the drivers. In my 
experience, this is a good first thing to try and often improves things substantially.

It should not be assumed that using both hardware and software Vsync is better than using only one of the two.
The failure case I typically observe if both are enabled is that each buffer swap will take twice the nominal
frame period. If this error occurrs, try using just one type of Vsync.

If none of the wait swap test configurations result in acceptable behavior, the implication is that there is an
error in the implementation of Vsync for your computer. If this is the case, you should be careful about using
stimulus presentation code that requests two or more swaps in a row (i.e. to swap in two different stimuli on 
two consecutive frames) following a multi-frame interval in which there were no buffer swaps. What may happen
is that the first stimulus may never be presented (especially if the "short" duration on the test is ~0).
If the short duration is not 0, then that stimulus should be presented, but if the long duration is less than 3P,
the preceding stimulus may be cut short. In cases like this, you may want to configure the CX_Display to swap
buffers automatically in a secondary thread all the time (see CX_Display::setAutomaticSwapping()), so that 
there are never swaps after several frames without swaps. The "animation" example shows how to use 
CX_Display::hasSwappedSinceLastCheck() to synchronize rendering in the main thread with buffer swaps in the 
secondary thread. Note that if your computer does not have at least a 2 core CPU, using a secondary thread to
constantly swap buffers is not a good solution, because the secondary thread will peg 1 CPU at 100% usage.

If none of these remedial measures corrects you problems, you may want to try another psychology experiment
package. However, many of them use OpenGL and so if the problem is your OpenGL configuration (hardware and 
software), switching to another package that uses OpenGL is unlikely to fix your problem (if it does,
let me know because that could point to an issue in CX or openFrameworks). 


\param desiredTestDuration An approximate amount of time to spend performing all of the tests. The total 
amount of time is divided among all of the tests equally.
\param testSecondaryThread If true, buffer swapping from within a secondary thread will be tested. If false, only
swapping from within the main thread will be tested.
\return A map containing CX_DataFrames. One data frame, named "summary" in the map, contains summary statistics. Another
data frame, named "constantSwapping", contains raw data from the constant swapping test. Another data frame, named "waitSwap",
contains raw data from the wait swap test.

\note This function blocks for approximately `desiredTestDuration` or more. See \ref blockingCode.
*/
std::map<std::string, CX_DataFrame> CX_Display::testBufferSwapping(CX_Millis desiredTestDuration, bool testSecondaryThread) {

	auto drawScreenData = [](CX_Display* display, ofColor color, std::string information) {
		display->beginDrawingToBackBuffer();
		ofBackground(color);
		ofDrawBitmapStringHighlight(information, ofPoint(100, 50), ofColor::black, ofColor::white);
		display->endDrawingToBackBuffer();
	};

	auto drawWaitSwapScreenData = [](CX_Display* display, ofColor background, ofColor rectColor, ofRectangle rect, std::string information) {
		display->beginDrawingToBackBuffer();
		ofBackground(background);
		ofSetColor(rectColor);
		ofDrawRectangle(rect);
		ofDrawBitmapStringHighlight(information, ofPoint(100, 50), ofColor::black, ofColor::white);
		display->endDrawingToBackBuffer();
	};

	bool wasSwapping = isAutomaticallySwapping();

	CX_Millis testSegmentDuration = desiredTestDuration / 12; //8 continuous swapping tests, but only 4 wait swap tests
	testSegmentDuration *= testSecondaryThread ? 1 : 1.5; //If not doing the secondary thread, make everything else go longer.


	CX_DataFrame summary;
	CX_DataFrame waitSwap;
	CX_DataFrame constantSwapping;

	for (int thread = (testSecondaryThread ? 0 : 1); thread < 2; thread++) {

		bool mainThread = (thread == 1);
		setAutomaticSwapping(!mainThread);

		for (int hardV = 0; hardV < 2; hardV++) {
			for (int softV = 0; softV < 2; softV++) {
				CX_DataFrameRow summaryRow;

				std::string threadName = mainThread ? "main" : "secondary";

				summaryRow["thread"] = threadName;
				summaryRow["hardVSync"] = hardV;
				summaryRow["softVSync"] = softV;

				//Configure V-Sync for the current test.
				useHardwareVSync(hardV == 1);
				useSoftwareVSync(softV == 1);

				std::stringstream ss;
				ss << "Thread: " << threadName << "\nHardV: " << hardV << "\nSoftV: " << softV;
				std::string conditionString = ss.str();

				std::vector<CX_Millis> swapTimes;

				//////////////////////////////
				// Continuous swapping test //
				//////////////////////////////
				if (mainThread) {
					//In order to give a fair test, each main thread test should start with some swaps.
					for (unsigned int i = 0; i < 3; i++) {
						swapBuffers();
					}

					CX_Millis startTime = CX::Instances::Clock.now();
					while ((CX::Instances::Clock.now() - startTime) < testSegmentDuration) {
						swapBuffers();
						swapTimes.push_back(CX::Instances::Clock.now());

						drawScreenData(this, (swapTimes.size() % 2) ? ofColor(255) : ofColor(0), "Continuous swapping test\n" + conditionString);
					}

				} else {

					CX::Instances::Clock.delay(CX_Millis(200));

					CX_Millis startTime = CX::Instances::Clock.now();
					while ((CX::Instances::Clock.now() - startTime) < testSegmentDuration) {
						if (this->hasSwappedSinceLastCheck()) {
							swapTimes.push_back(this->getLastSwapTime());

							drawScreenData(this, (swapTimes.size() % 2) ? ofColor(255) : ofColor(0), "Continuous swapping test\n" + conditionString);
						}
					}
				}

				std::vector<CX_Millis> durations(swapTimes.size() - 1);
				for (unsigned int i = 0; i < swapTimes.size() - 1; i++) {
					durations[i] = swapTimes[i + 1] - swapTimes[i];

					CX_DataFrame::RowIndex row = constantSwapping.getRowCount();

					constantSwapping(row, "thread") = threadName;
					constantSwapping(row, "hardVSync") = hardV;
					constantSwapping(row, "softVSync") = softV;
					constantSwapping(row, "duration") = durations[i];
				}

				summaryRow["csDurationMean"] = Util::mean(durations);
				summaryRow["csDurationStdDev"] = CX_Millis::standardDeviation(durations);
				summaryRow["csDurationMin"] = Util::min(durations);
				summaryRow["csDurationMax"] = Util::max(durations);

				////////////////////
				// Wait swap test //
				////////////////////
				if (!mainThread) {
					//Wait swap test is not performed for secondary thread.
					summaryRow["wsLongMean"] = "NULL";
					summaryRow["wsShortMean"] = "NULL";
					summaryRow["wsNormalMean"] = "NULL";
					summaryRow["wsTotalMean"] = "NULL";
				} else {
					swapTimes.clear();
					std::vector<std::string> durationType;

					ofRectangle resolution = this->getResolution();

					CX_Millis startTime = CX::Instances::Clock.now();
					CX_Millis period = Util::mean(durations);
					while ((CX::Instances::Clock.now() - startTime) < testSegmentDuration) {

						drawWaitSwapScreenData(this, ofColor::black, ofColor::white,
											   ofRectangle(0, 0, resolution.width / 3, resolution.height),
											   "Wait swap test\n" + conditionString);
						swapBuffers();
						swapTimes.push_back(CX::Instances::Clock.now());
						durationType.push_back("long");


						drawWaitSwapScreenData(this, ofColor::black, ofColor::white,
											   ofRectangle(resolution.width / 3, 0, resolution.width / 3, resolution.height),
											   "Wait swap test\n" + conditionString);

						CX::Instances::Clock.delay(period * 2.5);

						swapBuffers();
						swapTimes.push_back(CX::Instances::Clock.now());
						durationType.push_back("short");

						drawWaitSwapScreenData(this, ofColor::black, ofColor::white,
											   ofRectangle(resolution.width * 2 / 3, 0, resolution.width / 3, resolution.height),
											   "Wait swap test\n" + conditionString);
						swapBuffers();
						swapTimes.push_back(CX::Instances::Clock.now());
						durationType.push_back("normal");
					}

					durationType.pop_back(); //Make it so that durations lines up with swapType in terms of length.

					durations.resize(swapTimes.size() - 1);
					for (unsigned int i = 0; i < swapTimes.size() - 1; i++) {
						durations[i] = swapTimes[i + 1] - swapTimes[i];

						CX_DataFrame::RowIndex row = waitSwap.getRowCount();

						waitSwap(row, "thread") = threadName;
						waitSwap(row, "hardVSync") = hardV;
						waitSwap(row, "softVSync") = softV;
						waitSwap(row, "type") = durationType[i];
						waitSwap(row, "duration") = durations[i];

					}

					CX_Millis longSwapSum = 0;
					unsigned int longSwapCount = 0;
					CX_Millis shortSwapSum = 0;
					unsigned int shortSwapCount = 0;
					CX_Millis normalSwapSum = 0;
					unsigned int normalSwapCount = 0;

					for (unsigned int i = 0; i < durationType.size(); i++) {
						if (durationType[i] == "long") {
							longSwapSum += durations[i];
							longSwapCount++;
						} else if (durationType[i] == "short") {
							shortSwapSum += durations[i];
							shortSwapCount++;
						} else if (durationType[i] == "normal") {
							normalSwapSum += durations[i];
							normalSwapCount++;
						}
					}

					summaryRow["wsLongMean"] = longSwapSum / longSwapCount;
					summaryRow["wsShortMean"] = shortSwapSum / shortSwapCount;
					summaryRow["wsNormalMean"] = normalSwapSum / normalSwapCount;
					summaryRow["wsTotalMean"] = summaryRow["wsLongMean"].toDouble() + 
						summaryRow["wsShortMean"].toDouble() + 
						summaryRow["wsNormalMean"].toDouble();
				}

				summary.appendRow(summaryRow);

			}
		}
	}

	setAutomaticSwapping(wasSwapping);

	std::map<std::string, CX_DataFrame> data;
	data["summary"] = summary;
	data["constantSwapping"] = constantSwapping;
	data["waitSwap"] = waitSwap;

	return data;
}


} //namespace CX
