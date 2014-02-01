#ifndef _CX_DISPLAY_H_
#define _CX_DISPLAY_H_



#include <deque>

#include "ofThread.h"
#include "ofRectangle.h"
#include "ofFbo.h"
#include "ofGraphics.h"
#include "ofAppRunner.h"
#include "ofGlProgrammableRenderer.h"

#include "CX_Clock.h"
#include "CX_Logger.h"
#include "CX_SwappingThread.h"

namespace CX {

	class CX_Display {
	public:

		CX_Display (void);
		~CX_Display (void);

		void setup (void);
		void update (void);
		void exit (void);

		void setFullScreen (bool fullScreen);
		
		void drawFboToBackBuffer (ofFbo &fbo);
		void drawFboToBackBuffer (ofFbo &fbo, ofRectangle placement);
		void beginDrawingToBackBuffer (void);
		void endDrawingToBackBuffer (void);
		void BLOCKING_swapFrontAndBackBuffers (void);

		void BLOCKING_setSwappingState (bool autoSwap);
		bool isAutomaticallySwapping (void);
		bool hasSwappedSinceLastCheck (void);
		CX_Micros getLastSwapTime (void);

		CX_Micros getFramePeriod (void);
		void setWindowResolution (int width, int height);
		ofRectangle getResolution (void);
		ofPoint getCenterOfDisplay (void);
		uint64_t getFrameNumber (void);

		void BLOCKING_estimateFramePeriod (CX_Micros estimationInterval); //Also estimate standard deviation. Return a struct with this info?
		CX_Micros estimateNextSwapTime (void); //Maybe, given the range of observed swaps, this could give an upper and lower bound?

		void BLOCKING_waitForOpenGL (void);

	private:
		ofPtr<ofGLProgrammableRenderer> _renderer;

		CX_ConstantlySwappingThread *_swapThread;

		CX_Micros _framePeriod;

	};

}

#endif //_CX_DISPLAY_H_