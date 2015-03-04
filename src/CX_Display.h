#pragma once

/*! \defgroup video Video
This module is related to creating and presenting visual stimuli.

The CX::Draw namespace contains some more complex drawing functions. However, almost all of the drawing of
stimuli is done using openFrameworks functions. A lot of the common functions can be found in ofGraphics.h
(http://www.openframeworks.cc/documentation/graphics/ofGraphics.html), but there are a lot of other ways to
draw stimuli with openFrameworks: See the graphics and 3d sections of this page: http://www.openframeworks.cc/documentation/.
*/

#include <deque>

#include "ofThread.h"
#include "ofRectangle.h"
#include "ofFbo.h"
#include "ofGraphics.h"
#include "ofAppRunner.h"
#include "ofGLProgrammableRenderer.h"

#include "CX_Clock.h"
#include "CX_Logger.h"
#include "CX_VideoBufferSwappingThread.h"
#include "CX_DataFrame.h"

namespace CX {

	/*! This class represents an abstract visual display surface, which is my way of saying that it doesn't
	necessarily represent a monitor. The display surface can either be a window or, if full screen, the whole
	monitor. It is also a bit abstract in that it does not draw anything, but only creates an context in which
	things can be drawn.
	\ingroup video
	*/
	class CX_Display {
	public:
		CX_Display(void);
		~CX_Display(void);

		void setup(void);
		void configureFromFile(std::string filename, std::string delimiter = "=", bool trimWhitespace = true, std::string commentString = "//");

		void setFullscreen(bool fullscreen);
		bool isFullscreen(void);
		void useHardwareVSync(bool b);
		void useSoftwareVSync(bool b);

		void beginDrawingToBackBuffer(void);
		void endDrawingToBackBuffer(void);

		void swapBuffers(void);
		void swapBuffersInThread(void);
		void setAutomaticSwapping(bool autoSwap);
		bool isAutomaticallySwapping(void) const;

		bool hasSwappedSinceLastCheck(void);
		void waitForBufferSwap(void);
		CX_Millis getLastSwapTime(void) const;
		CX_Millis estimateNextSwapTime(void) const;
		uint64_t getFrameNumber(void) const;

		void estimateFramePeriod(CX_Millis estimationInterval, float minRefreshRate = 40, float maxRefreshRate = 160);
		CX_Millis getFramePeriod(void) const;
		CX_Millis getFramePeriodStandardDeviation(void) const;
		void setFramePeriod(CX_Millis knownPeriod);

		void setWindowResolution(int width, int height);
		ofRectangle getResolution(void) const;
		ofPoint getCenter(void) const;

		void waitForOpenGL(void);

		std::map<std::string, CX_DataFrame> testBufferSwapping(CX_Millis desiredTestDuration, bool testSecondaryThread);

		ofFbo makeFbo(void);
		void copyFboToBackBuffer(ofFbo &fbo);
		void copyFboToBackBuffer(ofFbo &fbo, ofPoint destination);
		void copyFboToBackBuffer(ofFbo &fbo, ofRectangle source, ofPoint destination);

		void setYIncreasesUpwards(bool upwards);
		bool getYIncreasesUpwards(void);

	private:

		ofPtr<ofGLProgrammableRenderer> _renderer;

		std::unique_ptr<Private::CX_VideoBufferSwappingThread> _swapThread;

		CX_Millis _framePeriod;
		CX_Millis _framePeriodStandardDeviation;

		uint64_t _manualBufferSwaps;
		uint64_t _frameNumberOnLastSwapCheck;

		bool _softVSyncWithGLFinish;

		void _blitFboToBackBuffer(ofFbo& fbo, ofRectangle sourceCoordinates, ofRectangle destinationCoordinates);

	};

    namespace Instances {
		extern CX_Display Disp;
	}

}
