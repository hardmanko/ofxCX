#pragma once

#include <stdint.h>
#include <functional>

#include "ofFbo.h"

#include "CX_Logger.h"
#include "CX_Utilities.h"
#include "CX_Display.h"
#include "CX_InputManager.h"

namespace CX {

	/*! This class is a useful abstraction that presents slides (i.e. a full display) of visual stimuli for fixed durations.
	See the changeDetection and nBack examples for the usage of this class.

	A brief example:

	\code{.cpp}
	CX_SlidePresenter slidePresenter;
	slidePresenter.setup(&Disp);

	slidePresenter.beginDrawingNextSlide(2000, "circle");
	ofBackground(50);
	ofSetColor(ofColor::red);
	ofCircle(Disp.getCenter(), 40);

	slidePresenter.beginDrawingNextSlide(1000, "rectangle");
	ofBackground(50);
	ofSetColor(ofColor::green);
	ofRect(Disp.getCenter() - ofPoint(100, 100), 200, 200);

	//The duration of the last slide, as long as it is greater than 0, is ignored.
	slidePresenter.beginDrawingNextSlide(1, "off");
	ofBackground(50);
	slidePresenter.endDrawingCurrentSlide(); //it is not necessary to call this, but the slide presenter will complain if you don't.

	slidePresenter.startSlidePresentation();

	//Update the slide presenter while waiting for slide presentation to complete
	while (slidePresenter.isPresentingSlides()) {
		slidePresenter.update(); //You must remember to call update() regularly while slides are being presented!
	}
	\endcode

	\ingroup video
	*/
	class CX_SlidePresenter {
	public:


		/*! The settings in this enum are related to what a CX_SlidePresenter does when it encounters a timing error.
		Timing errors are probably almost exclusively related to one slide being presented for too long.

		The PROPAGATE_DELAYS setting causes the slide presenter to handle these errors by moving the start time
		of all future stimuli back by the amount of extra time (or frames) used to the erroneous slide. This makes the
		durations of all future stimuli correct, so that there is only an error in the duration of one slide. If
		a slide's presentation start time is early, the intended start time is used (i.e. only delays, not early
		arrivals, are propogated).

		Other alternatizes are being developed.
		*/
		enum class ErrorMode {
			PROPAGATE_DELAYS
			//FIX_TIMING_FROM_FIRST_SLIDE
			/*
			An alternative option is to try to keep the onsets of all slides as constant as possible
			relative to each other. This means that if one slide is presented for an extra frame, the next slide
			will be presented for one frame less than it should have been. If one slide is presented for several
			extra frames (this should almost never happen), the next slide may be skipped altogether. However,
			this mode (FIX_TIMING_FROM_FIRST_SLIDE) does not completely work currently so it should not be used.
			*/
		};

		/*! The method used by the slide presenter to swap stimuli that have been drawn to the back buffer to the front buffer.
		MULTI_CORE is theoretically the best method, but only really works properly if you have at least a 2 core CPU. It uses 
		a secondary thread to constantly swap the front and back buffers, which allows each frame to be counted. This results 
		in really good synchronization between the copies of data to the back buffer and the swaps of the front and back buffers.
		In the SINGLE_CORE_BLOCKING_SWAPS mode, after a stimulus has been copied to the front buffer, the next stimulus is immediately
		drawn to the back buffer. After the correct amount of time minus \ref CX_SlidePresenter::Configuration::preSwapCPUHoggingDuration,
		the buffers are swapped. The main problem with this mode is that the buffer swapping in this mode \ref blockingCode "blocks" 
		in the main thread while waiting for the swap. However, it avoids thread synchronization issues, which is a huge plus.
		*/
		enum class SwappingMode {
			SINGLE_CORE_BLOCKING_SWAPS, //!< The slide presenter does bufer swapping in the main thread, blocking briefly during the buffer swap.
			//SINGLE_CORE_THREADED_SWAPS, //could be TIMED_THREADED In the SINGLE_CORE_THREADED_SWAPS mode, after a stimulus has been copied to the front buffer, the next stimulus is immediately drawn to the back buffer.After the correct amount of time minus preSwapCPUHoggingDuration, a swap of the front and back buffers is queued by launching a thread.
			
			/*! \brief The slide presenter does bufer swapping in a secondary thread, which means that 
			there is no blocking in the main thread when buffers are swapping. */
			MULTI_CORE
		};

		/*! The final slide user function takes a reference to a struct of this type. 
		See CX_SlidePresenter::Configuration::finalSlideCallback for more information. */
		struct FinalSlideFunctionArgs {
			FinalSlideFunctionArgs(void) :
				instance(nullptr),
				currentSlideIndex(0)
			{}

			CX_SlidePresenter *instance; //!< A pointer to the CX_SlidePresenter that called the user function.
			unsigned int currentSlideIndex; //!< The index of the slide that is currently being presented.
		};

		/*! This struct contains information about errors that were detected during slide presentation.
		See CX_SlidePresenter::checkForPresentationErrors(). */
		struct PresentationErrorInfo {
			PresentationErrorInfo(void) :
				presentationErrorsSuccessfullyChecked(false),
				incorrectFrameCounts(0),
				lateCopiesToBackBuffer(0)
			{}

			//how about indices for the frames with errors?

			/*! \brief True if presentation errors were successfully checked for. This does not mean that there were
			no presentation errors, but that there were no presentation error checking errors. */
			bool presentationErrorsSuccessfullyChecked;

			/*! The number of slides for which the actual and intended frame counts did not match,
			indicating that the slide was presented for too many or too few frames.	*/
			unsigned int incorrectFrameCounts;

			/*! \brief The number of slides for which the time at which the slide finished being copied
			to the back buffer was after the actual start time of the slide. */
			unsigned int lateCopiesToBackBuffer;

			/*! \brief Returns the sum of the different types of errors that are measured. */
			unsigned int totalErrors(void) {
				return incorrectFrameCounts + lateCopiesToBackBuffer;
			}

		};

		/*! This struct is used for configuring a CX_SlidePresenter. See CX_SlidePresenter::setup(const CX_SlidePresenter::Configuration&). */
		struct Configuration {
			Configuration(void) :
				display(nullptr),
				finalSlideCallback(nullptr),
				errorMode(CX_SlidePresenter::ErrorMode::PROPAGATE_DELAYS),
				deallocateCompletedSlides(false),
				swappingMode(CX_SlidePresenter::SwappingMode::SINGLE_CORE_BLOCKING_SWAPS),
				preSwapCPUHoggingDuration(2),
				useFenceSync(true),
				waitUntilFenceSyncComplete(false)
			{}

			CX_Display *display; //!< A pointer to the display to use.

			/*! \brief A pointer to a user function that will be called as soon as the final slide is presented. In this function,
			you can add additional slides to the slide presenter and do other tasks, like process input. */
			std::function<void(CX_SlidePresenter::FinalSlideFunctionArgs&)> finalSlideCallback;

			/*! \brief This sets how errors in slide presentation should be handled.
			Currently, the only available mode is the default, so this should not be changed. */
			CX_SlidePresenter::ErrorMode errorMode;

			/*! \brief If `true`, once a slide has been presented, its framebuffer will be deallocated to conserve video memory. 
			This only matters if you are using a large number of slides at once and add slides during slide presentation. */
			bool deallocateCompletedSlides;

			/*! \brief The mode used for swapping slides. See the SwappingMode enum for the possible settings. 
			Defaults to `SINGLE_CORE_BLOCKING_SWAPS`. */
			SwappingMode swappingMode;

			/*! \brief Only used if swappingMode is a single core mode. The amount of time, before a slide is swapped from
			the back buffer to the front buffer, that the CPU is put into a spinloop waiting for the buffers to swap. */
			CX_Millis preSwapCPUHoggingDuration;

			/*! \brief Hint that fence sync should be used to check that slides are fully copied to the back buffer
			before they are swapped in. This can help prevent vertical tearing. */
			bool useFenceSync;

			/*! \brief If useFenceSync is false, this is also forced to false. If this is true, new slides will not be swapped in until
			there is confirmation that the slide has been fully copied into the back buffer. This prevents vertical tearing, but
			may cause slides to be swapped in late if the copy confirmation is delayed but the copy has actually occurred.
			Does nothing if `swappingMode` is `MULTI_CORE`. */
			bool waitUntilFenceSyncComplete;
		};

		/*! Contains information about the presentation timing of the slide. */
		struct SlideTimingInfo {
			uint32_t startFrame; /*!< \brief The frame on which the slide started/should have started. Can be compared with the value given by Disp.getFrameNumber(). */
			uint32_t frameCount; /*!< \brief The number of frames the slide was/should have been presented for. */
			CX_Millis startTime; /*!< \brief The time at which the slide was/should have been started. Can be compared with values from CX::CX_Clock::now(). */
			CX_Millis duration; /*!< \brief The amount of time the slide was/should have been presented for. */
		};

		/*! This struct contains information related to slide presentation using CX_SlidePresenter. */
		struct Slide {

			Slide() :
				slideName("unnamed"),
				drawingFunction(nullptr),
				slideStatus(NOT_STARTED)
			{}

			std::string slideName; //!< The name of the slide. Set by the user during slide creation.

			ofFbo framebuffer; /*!< \brief A framebuffer containing image data that will be drawn to the screen during this slide's presentation.
							   If drawingFunction points to a user function, framebuffer will not be drawn. */
			std::function<void(void)> drawingFunction; /*!< \brief Pointer to a user function that will be called to draw the slide.
										   If this points to a user function, it overrides `framebuffer`. The drawing function is
										   not required to call ofBackground() or otherwise clear the display before drawing, which
										   allows you to do what is essentially single-buffering using the back buffer as the framebuffer.
										   However, if you want a blank framebuffer, you will have to clear it manually. */

			/*! \brief Status of the current slide vis a vis presentation. This should not be modified by the user. */
			enum {
				NOT_STARTED,
				COPY_TO_BACK_BUFFER_PENDING,
				SWAP_PENDING,
				IN_PROGRESS,
				FINISHED
			} slideStatus;

			SlideTimingInfo intended; //!< The intended timing parameters (i.e. what should have happened if there were no presentation errors).
			SlideTimingInfo actual; //!< The actual timing parameters.

			CX_Millis copyToBackBufferCompleteTime; /*!< \brief The time at which the drawing operations for this slide finished.
													This is pretty useful to determine if there was an error on the trial (e.g. framebuffer was copied late).
													If this is greater than actual.startTime, the slide may not have been fully drawn at the time the
													front and back buffers swapped. */
		};


		CX_SlidePresenter (void);

		bool setup (CX_Display *display);
		bool setup (const CX_SlidePresenter::Configuration &config);
		void update (void);

		void appendSlide (CX_SlidePresenter::Slide slide);
		void appendSlideFunction (std::function<void(void)> drawingFunction, CX_Millis slideDuration, std::string slideName = "");
		void beginDrawingNextSlide(CX_Millis slideDuration, std::string slideName = "");
		void endDrawingCurrentSlide (void);

		bool startSlidePresentation (void);
		void stopSlidePresentation (void);
		bool isPresentingSlides (void) const;
		bool presentSlides(void);

		void clearSlides (void);

		std::vector<CX_SlidePresenter::Slide>& getSlides(void);
		CX_SlidePresenter::Slide& getSlideByName(std::string name);

		std::vector<CX_Millis> getActualPresentationDurations(void);
		std::vector<unsigned int> getActualFrameCounts(void);

		CX_SlidePresenter::PresentationErrorInfo checkForPresentationErrors(void) const;
		std::string printLastPresentationInformation(void) const;

	private:

		struct ExtraSlideInfo {
			ExtraSlideInfo(void) :
				awaitingFenceSync(false)
			{}

			bool awaitingFenceSync;
			GLsync fenceSyncObject;
		};

		CX_SlidePresenter::Configuration _config;

		CX_Millis _hoggingStartTime;

		bool _presentingSlides;
		bool _synchronizing;
		unsigned int _currentSlide;
		std::vector<CX_SlidePresenter::Slide> _slides;
		std::vector<ExtraSlideInfo> _slideInfo;

		bool _lastFramebufferActive;

		unsigned int _calculateFrameCount(CX_Millis duration);

		void _singleCoreBlockingUpdate (void);
		void _singleCoreThreadedUpdate (void);
		void _multiCoreUpdate (void);

		void _renderCurrentSlide(void);

		bool _useFenceSync;
		void _waitSyncCheck(void);

		void _finishPreviousSlide(void);
		void _handleFinalSlide(void);
		void _prepareNextSlide(void);

		bool _hasSwappedSinceLastCheck(void);
		uint64_t _frameNumberOnLastSwapCheck;

	};
}
