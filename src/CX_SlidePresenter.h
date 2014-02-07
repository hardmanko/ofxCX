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

	/*! The final slide function takes a reference to a struct of this type.
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

	/*! Contains information about errors that were detected during slide presentation. 
	See CX_SlidePresenter::checkForPresentationErrors().
	\ingroup video */
	struct CX_SP_PresentationErrors_t {
		CX_SP_PresentationErrors_t(void) :
			presentationErrorsSuccessfullyChecked(false),
			incorrectFrameCounts(0),
			lateCopiesToBackBuffer(0)
		{}

		/*! \brief True if presentation errors were successfully checked for. This does not mean that there were 
		no presentation errors, but that there were no presentation error checking errors. */
		bool presentationErrorsSuccessfullyChecked; 

		unsigned int incorrectFrameCounts; //!< The number of slides for which the actual and intended frame counts did not match.

		/*! \brief The number of slides for which the time at which the slide finished being copied
		to the back buffer was after the actual start time of the slide. */
		unsigned int lateCopiesToBackBuffer;

		/*! \brief Sums up all of the different types of errors that are measured. */
		unsigned int totalErrors(void) {
			return incorrectFrameCounts + lateCopiesToBackBuffer;
		}

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
		bool deallocateCompletedSlides; //!< If true, once a slide has been presented, its framebuffer will be deallocated to conserve memory.

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

		CX_Slide_t() :
			slideName("unnamed"),
			drawingFunction(NULL),
			slideStatus(NOT_STARTED)
		{}

		std::string slideName; //!< The name of the slide. Set by the user during slide creation.

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

	/*! This class is a very useful abstraction that presents slides (a full display's worth) of visual stimuli for fixed durations.
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
		void appendSlideFunction (void (*drawingFunction)(void), CX_Micros slideDuration, std::string slideName = "");
		void beginDrawingNextSlide (CX_Micros slideDuration, std::string slideName = "");
		void endDrawingCurrentSlide (void);

		bool startSlidePresentation (void);
		void stopSlidePresentation (void);

		//! Returns true if slide presentation is in progress, even if the first slide has not yet been presented.
		bool isPresentingSlides (void) { return _presentingSlides || _synchronizing; };

		void clearSlides (void);
		
		// Returns the index of the slide that is currently being presented.
		//This is weird because the active slide becomes active before it is on screen.
		//unsigned int getActiveSlideIndex (void) { return _currentSlide; };
		//std::string getActiveSlideName (void); //This sucks for the same reason that getActiveSlideIndex sucks.
		//unsigned int getSlideCount(void) { return _slides.size(); }; //Just use getSlides().size()
		//CX_Slide_t& getSlide (unsigned int slideIndex);

		std::vector<CX_Slide_t>& getSlides(void);

		std::vector<CX_Micros> getActualPresentationDurations(void);
		std::vector<unsigned int> getActualFrameCounts(void);

		CX_SP_PresentationErrors_t checkForPresentationErrors(void);

	protected:

		CX_Display *_display;
		std::function<void(CX_FinalSlideFunctionInfo_t&)> _userFunction;

		bool _presentingSlides;
		bool _synchronizing;
		unsigned int _currentSlide;
		std::vector<CX_Slide_t> _slides;

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