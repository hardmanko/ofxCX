#ifndef _CX_DISPLAY_H_
#define _CX_DISPLAY_H_

/*! \defgroup video Video
This module is related to creating and presenting visual stimuli.

The CX::Draw namespace contains some more complex drawing functions. However, almost all of the drawing of 
stimuli is done using openFrameworks functions. A lot of the common functions can be found in ofGraphics.h 
(http://www.openframeworks.cc/documentation/graphics/ofGraphics.html), but there are a lot of other ways to 
draw stimuli: see the graphics and 3d sections if this page: http://www.openframeworks.cc/documentation/.
*/

#include <deque>

#include "ofThread.h"
#include "ofRectangle.h"
#include "ofFbo.h"
#include "ofGraphics.h"
#include "ofAppRunner.h"
#include "ofGlProgrammableRenderer.h"

#include "CX_Clock.h"
#include "CX_Logger.h"
#include "CX_VideoBufferSwappingThread.h"

namespace CX {

	/*! This class represents an abstract visual display surface, which is my way of saying that it doesn't 
	necessarily represent a monitor. The display surface can either be a window or, if full screen, the whole 
	monitor. It is also a bit abstract in that it does not draw anything, but only creates an context in which
	things can be drawn.
	\ingroup video
	*/
	class CX_Display {
	public:

		CX_Display (void);
		~CX_Display (void);

		void setup (void);

		void setFullScreen (bool fullScreen);
		
		void drawFboToBackBuffer (ofFbo &fbo);
		void drawFboToBackBuffer (ofFbo &fbo, ofRectangle placement);
		void beginDrawingToBackBuffer (void);
		void endDrawingToBackBuffer (void);
		void BLOCKING_swapFrontAndBackBuffers (void);
		void swapFrontAndBackBuffers (void);

		void BLOCKING_setAutoSwapping (bool autoSwap);
		bool isAutomaticallySwapping (void);
		bool hasSwappedSinceLastCheck (void);
		CX_Micros getLastSwapTime (void);

		CX_Micros getFramePeriod (void);
		void setWindowResolution (int width, int height);
		void setWindowTitle(std::string title);
		ofRectangle getResolution (void);
		ofPoint getCenterOfDisplay (void);
		uint64_t getFrameNumber (void);

		void BLOCKING_estimateFramePeriod (CX_Micros estimationInterval); //Also estimate standard deviation. Return a struct with this info?
		CX_Micros estimateNextSwapTime (void); //Maybe, given the range of observed swaps, this could give an upper and lower bound?

		void BLOCKING_waitForOpenGL (void);

	private:
		ofPtr<ofGLProgrammableRenderer> _renderer;

		CX_VideoBufferSwappingThread *_swapThread;

		CX_Micros _framePeriod;

		uint64_t _manualBufferSwaps;
		uint64_t _frameNumberOnLastSwapCheck;

		void _exitHandler(void);

	};

}

#endif //_CX_DISPLAY_H_