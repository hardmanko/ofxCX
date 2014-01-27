#ifndef _CX_SLIDE_PRESENTER_H_
#define _CX_SLIDE_PRESENTER_H_

#include <stdint.h>
#include <functional>

#include "ofFbo.h"

#include "CX_Logger.h"
#include "CX_Utilities.h"
#include "CX_Display.h"

namespace CX {

	class CX_SlidePresenter;

	enum class CX_SP_ErrorMode {
		PROPAGATE_DELAYS,
		FIX_TIMING_FROM_FIRST_SLIDE
	};

	struct CX_UserFunctionInfo_t {
		CX_UserFunctionInfo_t(void) :
			instance(nullptr),
			currentSlideIndex(0),
			userStatus(CX_UserFunctionInfo_t::CONTINUE_PRESENTATION)
		{}

		CX_SlidePresenter *instance;
		unsigned int currentSlideIndex;

		enum {
			CONTINUE_PRESENTATION,
			STOP_NOW
		} userStatus;
	};

	struct CX_SP_Configuration {
		CX_SP_Configuration(void) :
			display(nullptr),
			userFunction(nullptr),
			errorMode(CX_SP_ErrorMode::PROPAGATE_DELAYS),
			deallocateCompletedSlides(true)
		{}

		CX_Display *display;
		std::function<void(CX_UserFunctionInfo_t&)> userFunction;
		CX_SP_ErrorMode errorMode;
		bool deallocateCompletedSlides;
	};

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

		bool setup (CX_Display *display);
		bool setup (const CX_SP_Configuration &config);
		virtual void update (void);
		
		void appendSlide (CX_Slide_t slide);
		void appendSlideFunction (void (*drawingFunction) (void), CX_Micros_t duration, string slideName = "");
		void beginDrawingNextSlide (CX_Micros_t duration, string slideName = "");
		void endDrawingCurrentSlide (void);

		void startSlidePresentation(void);

		void clearSlides (void);

		//void setThreadingMode (bool singleThreaded); //Flesh this out. Maybe have it be a setup() parameter
	


		bool isPresentingSlides (void) { return _presentingSlides || _synchronizing; };

		unsigned int getActiveSlideIndex (void) { return _currentSlide; };
		unsigned int getSlideCount(void) { return _slides.size(); };
		string getActiveSlideName (void);
		CX_Slide_t& getSlide (unsigned int slideIndex);

		vector<CX_Slide_t> getSlides (void); //Return reference??
		vector<CX_Micros_t> getActualPresentationDurations (void);
		vector<unsigned int> getActualFrameCounts (void);

		int checkForPresentationErrors (void);

	protected:

		CX_Display *_display;
		std::function<void(CX_UserFunctionInfo_t&)> _userFunction;

		bool _presentingSlides;
		bool _synchronizing;
		unsigned int _currentSlide;
		vector<CX_Slide_t> _slides;

		bool _awaitingFenceSync;
		GLsync _fenceSyncObject;
	
		bool _lastFramebufferActive;

		void _renderCurrentSlide (void);
		void _waitSyncCheck (void);

		void _handleFinalSlide(void);
		void _finishPreviousSlide(void);
		void _prepareNextSlide(void);

		unsigned int _calculateFrameCount(CX_Micros_t duration);

		CX_SP_ErrorMode _errorMode;
		bool _deallocateFramebuffersForCompletedSlides;

	};

}

#endif //_CX_SLIDE_PRESENTER_H_