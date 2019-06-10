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

#include "CX_DisplayThread.h"
#include "CX_DisplaySwapper.h"

#include "CX_Clock.h"
#include "CX_Logger.h"
#include "CX_DataFrame.h"


namespace CX {


	/*! This class represents an abstract visual display surface, which is my way of saying that it doesn't
	necessarily represent a monitor. The display surface can either be a window or, if full screen, the whole
	monitor. It is also a bit abstract in that it does not draw anything, but only creates a context in which
	things can be drawn.

	An instance of this class is created for the user. It is called \ref CX::Instances::Disp. The user should not
	need another instance of this class.

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
		void setMinimized(bool minimize);

		// rename these?
		void useHardwareVSync(bool b);
		void useSoftwareVSync(bool b);
		//bool usingHardwareVSync(void);
		bool usingSoftwareVSync(void);

		void waitForOpenGL(void);
		

		void beginDrawingToBackBuffer(void);
		void endDrawingToBackBuffer(void);

		void swapBuffers(void);
		//void swapAt(CX_Millis time); // blocking function for really basic synchronization?

		// main class
		void estimateFramePeriod(CX_Millis estimationInterval, float minRefreshRate = 30, float maxRefreshRate = 150);
		void setFramePeriod(CX_Millis knownPeriod, bool setupSwapTracking = false);
		CX_Millis getFramePeriod(void) const;
		CX_Millis getFramePeriodStandardDeviation(void) const;


		void setWindowResolution(int width, int height);
		ofRectangle getResolution(void) const;
		ofPoint getCenter(void) const;


		// these functions stay
		FrameNumber getLastFrameNumber(void);
		CX_Millis getLastSwapTime(void);
		CX_Millis getNextSwapTime(void);


		// swapping in display thread, rendering in main thread
		bool hasSwappedSinceLastCheck(void);
		bool waitForBufferSwap(CX_Millis timeout, bool reset = true);

		
		// strange function. maybe non-member function?
		std::map<std::string, CX_DataFrame> testBufferSwapping(CX_Millis desiredTestDuration, bool testSecondaryThread);

		ofFbo makeFbo(void);

		void copyFboToBackBuffer(ofFbo &fbo);
		void copyFboToBackBuffer(ofFbo &fbo, ofPoint destination);
		void copyFboToBackBuffer(ofFbo &fbo, ofRectangle source, ofPoint destination);

		//void setYIncreasesUpwards(bool upwards);
		//bool getYIncreasesUpwards(void) const;

		std::shared_ptr<ofBaseRenderer> getRenderer(void);
		
		Sync::DataContainer swapData;
		Sync::DataClient swapClient;

		////////////////////
		// Display Thread //
		////////////////////

		CX_DisplayThread* getDisplayThread(void);

		// helpers
		bool renderingOnThisThread(void);
		bool renderingOnMainThread(void);

		// legacy interface. keep option to not swap in thread but keep thread running?
		bool setAutomaticSwapping(bool autoSwap);
		bool isAutomaticallySwapping(void);


	private:

		void _blitFboToBackBuffer(ofFbo& fbo, ofRectangle sourceCoordinates, ofRectangle destinationCoordinates);
		
		std::shared_ptr<ofBaseRenderer> _renderer; 

		// From estimateFramePeriod()
		CX_Millis _framePeriod;
		CX_Millis _framePeriodStandardDeviation;

		bool _softVSyncWithGLFinish;

		void _setupSwapTracking(CX_Millis nominalFramePeriod);

		// display thread stuff
		CX_DisplayThread _dispThread;

		std::unique_ptr<Sync::DataContainer::PolledSwapListener> _polledSwapListener;

		void _swapBuffers(void);

	};


    namespace Instances {
		extern CX_Display Disp;
	}

}
