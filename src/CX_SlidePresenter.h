#ifndef _CX_SLIDE_PRESENTER_H_
#define _CX_SLIDE_PRESENTER_H_

#include <stdint.h>
#include <functional>

#include "ofFbo.h"

#include "CX_Logger.h"
#include "CX_Utilities.h"
#include "CX_Display.h"

namespace CX {

	struct CX_Slide_t {

		CX_Slide_t () :
			drawingFunction(NULL),
			slideStatus(NOT_STARTED)
		{}

		string slideName;

		ofFbo framebuffer;
		void (*drawingFunction) (void);

		enum {
			NOT_STARTED,
			COPY_TO_BACK_BUFFER_PENDING,
			SWAP_PENDING,
			IN_PROGRESS,
			FINISHED
		} slideStatus;

		uint32_t intendedFrameCount;
		uint32_t intendedOnsetFrameNumber;
		uint32_t actualFrameCount;
		uint32_t actualOnsetFrameNumber;

		CX_Micros_t intendedSlideDuration; //These durations (in microseconds) are good for about 600,000 years.
		CX_Micros_t actualSlideDuration;
		CX_Micros_t intendedSlideOnset; 
		CX_Micros_t actualSlideOnset;

		CX_Micros_t copyToBackBufferCompleteTime; //This is pretty useful to determine if there was an error on the trial (i.e. framebuffer copied late).
	
	};

	class CX_SlidePresenter {
	public:

		CX_SlidePresenter (void);

		void setup (CX_Display *display);
		void update (void);
		

	
		void appendSlide (CX_Slide_t slide); //This is kind of sucky because people have to manually allocate the FBOs
		void appendSlideFunction (void (*drawingFunction) (void), CX_Micros_t duration, string slideName = "");

		//Much easier way of doing things.
		void beginDrawingNextSlide (CX_Micros_t duration, string slideName = "");
		void endDrawingCurrentSlide (void);

		void clearSlides (void);

		void setThreadingMode (bool singleThreaded); //Flesh this out.
	

		void startSlidePresentation (void);
		bool isPresentingSlides (void) { return _presentingSlides || _synchronizing; };

		int getActiveSlideIndex (void) { return _currentSlide; };
		string getActiveSlideName (void);
		CX_Slide_t& getSlide (unsigned int slideIndex);

		vector<CX_Slide_t> getSlides (void);
		vector<CX_Micros_t> getActualPresentationDurations (void);
		vector<unsigned int> getActualFrameCounts (void);

		int checkForPresentationErrors (void);

	protected:

		CX_Display *_display;

		bool _presentingSlides;
		bool _synchronizing;
		int _currentSlide;
		vector<CX_Slide_t> _slides;

		bool _awaitingFenceSync;
		GLsync _fenceSyncObject;
	
		bool _lastFramebufferActive;

		void _renderCurrentSlide (void);
		void _waitSyncCheck (void);

	};

}

#endif //_CX_SLIDE_PRESENTER_H_