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
		FIX_TIMING_FROM_FIRST_SLIDE //!< This does not work currently.
	};

	/*! The final slide function takes a reference to this struct.
	\ingroup video
	*/
	struct CX_FinalSlideFunctionInfo_t {
		CX_FinalSlideFunctionInfo_t(void) :
			instance(nullptr),
			currentSlideIndex(0)
		{}

		CX_SlidePresenter *instance; //!< A pointer to the CX_SlidePresenter that called the user function.
		unsigned int currentSlideIndex; //!< Redundant: May be gotten using instance->getActiveSlideIndex().

	};

	/*! This struct is used for configuring a CX_SlidePresenter.
	\ingroup video
	*/
	struct CX_SP_Configuration {
		CX_SP_Configuration(void) :
			display(nullptr),
			finalSlideCallback(nullptr),
			errorMode(CX_SP_ErrorMode::PROPAGATE_DELAYS),
			deallocateCompletedSlides(true)
		{}

		CX_Display *display; //!< A pointer to the display to use.
		std::function<void(CX_FinalSlideFunctionInfo_t&)> finalSlideCallback; //!< A pointer to a user function that will be called as soon as the final slide is presented.
		CX_SP_ErrorMode errorMode;
		bool deallocateCompletedSlides; //<! If true, once a slide has been presented, its framebuffer will be deallocated to conserve memory.

		//bool singleThreadedMode;
		//CX_Micros preSwapCPUHoggingDuration;
	};

	/*! Contains information about the presentation timing of the slide. 
	\ingroup video */
	struct CX_SlideTimingInfo_t {
		uint32_t startFrame; /*!< The frame on which the slide started/should have started. Can be compared with the value given by Display.getFrameNumber(). */
		uint32_t frameCount; /*!< The number of frames the slide was/should be presented for. */
		CX_Micros startTime; /*!< The time at which the slide was/should have been started. Can be compared with values from Clock.getTime(). */
		CX_Micros duration; /*!< Time amount of time the slide was/should have been presented for. */
	};

	/*! This struct contains information related to slide presentation using CX_SlidePresenter.
	\ingroup video */
	struct CX_Slide_t {

		CX_Slide_t () :
			drawingFunction(NULL),
			slideStatus(NOT_STARTED)
		{}

		string slideName; //!< The name of the slide. Set by the user during slide creation.

		ofFbo framebuffer; /*!< \brief A framebuffer containing image data that will be drawn to the screen during this slide's presentation.
						   If drawingFunction points to a user function, framebuffer will not be drawn. */
		void (*drawingFunction) (void); /*!< \brief Pointer to a user function that will be called to draw the slide. 
										If this points to a user function, it overrides framebuffer. The drawing function does
										not need to call ofBackground() or otherwise clear the display before drawing, which 
										allow you to do what is essentially single-buffering using the back buffer as the framebuffer.*/

		/*! Status of the current slide vis a vis presentation. */
		enum {
			NOT_STARTED,
			COPY_TO_BACK_BUFFER_PENDING,
			SWAP_PENDING,
			IN_PROGRESS,
			FINISHED
		} slideStatus;

		CX_SlideTimingInfo_t intended; //!< The intended timing parameters (i.e. what should have happened if there were no presentation errors).
		CX_SlideTimingInfo_t actual; //!< The actual timing parameters.

		CX_Micros copyToBackBufferCompleteTime; /*!< \brief The time at which the drawing operations for this slide finished.
												This is pretty useful to determine if there was an error on the trial (e.g. framebuffer was copied late). 
												If this is greater than actual.startTime, the slide may not have been fully drawn at the time the
												front and back buffers swapped. */
	
	};

	/*! This class is a very useful abstraction that presents frames (slides) of visual stimuli at fixed time intervals.
	See the basicChangeDetectionTask.cpp, advancedChangeDetectionTask.cpp, and nBack.cpp examples for the usage of this class.
	\ingroup video
	*/
	class CX_SlidePresenter {
	public:

		CX_SlidePresenter (void);

		bool setup (CX_Display *display);
		bool setup (const CX_SP_Configuration &config);
		virtual void update (void);
		
		void appendSlide (CX_Slide_t slide);
		void appendSlideFunction (void (*drawingFunction) (void), CX_Micros duration, string slideName = "");
		void beginDrawingNextSlide (CX_Micros duration, string slideName = "");
		void endDrawingCurrentSlide (void);

		bool startSlidePresentation(void);
		void stopPresentation(void);

		void clearSlides (void);	
		bool isPresentingSlides (void) { return _presentingSlides || _synchronizing; };

		unsigned int getActiveSlideIndex (void) { return _currentSlide; };
		unsigned int getSlideCount(void) { return _slides.size(); };
		string getActiveSlideName (void);
		CX_Slide_t& getSlide (unsigned int slideIndex);

		vector<CX_Slide_t> getSlides (void); //Return reference??
		vector<CX_Micros> getActualPresentationDurations (void);
		vector<unsigned int> getActualFrameCounts (void);

		int checkForPresentationErrors (void); //Maybe return a struct with specifics about the errors?

	protected:

		CX_Display *_display;
		std::function<void(CX_FinalSlideFunctionInfo_t&)> _userFunction;

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

		unsigned int _calculateFrameCount(CX_Micros duration);

		CX_SP_ErrorMode _errorMode;
		bool _deallocateFramebuffersForCompletedSlides;

	};

}

#endif //_CX_SLIDE_PRESENTER_H_