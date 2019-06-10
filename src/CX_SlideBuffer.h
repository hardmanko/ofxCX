#pragma once

#include "ofFbo.h"

#include "CX_Display.h"
#include "CX_Time_t.h"
#include "CX_Logger.h"
#include "CX_SynchronizationUtils.h"

namespace CX {

	class CX_SlideBuffer {
	public:

		/*! Contains information about the presentation timing of the slide. */
		struct SlideTimingInfo {

			SlideTimingInfo(void) :
				startTime(0),
				timeDuration(0),
				startFrame(0),
				frameDuration(0)
			{}

			CX_Millis startTime; /*!< \brief The time at which the slide was/should have been started. Can be compared with values from CX::CX_Clock::now(). */
			CX_Millis timeDuration; /*!< \brief The amount of time the slide was/should have been presented for. */
			FrameNumber startFrame; /*!< \brief The frame on which the slide started/should have started. Can be compared with the value given by CX_Display::getLastFrameNumber(). */
			FrameNumber frameDuration; /*!< \brief The number of frames the slide was/should have been presented for. */

		};

		struct SlidePresentationInfo {

			SlidePresentationInfo(void) :
				swappedBeforeRenderingComplete(false),
				renderStartTime(-1),
				renderCompleteTime(-1)
			{}

			bool swappedBeforeRenderingComplete;

			/*! \brief The time at which the drawing operations for this slide finished.
			This is pretty useful to determine if there was an error on the trial (e.g. framebuffer was copied late).
			If this is greater than actual.startTime, the slide may not have been fully drawn at the time the
			front and back buffers swapped. */
			//CX_Millis copyToBackBufferCompleteTime; //CX_Millis renderingCompleteTime; //?

			CX_Millis renderStartTime;
			CX_Millis renderCompleteTime;

			
		};
		
		/*! This struct contains information related to slides to present on screen. */
		struct Slide {

			Slide(void) :
				name(""),
				framebuffer(nullptr),
				drawingFunction(nullptr),
				slidePresentedCallback(nullptr),
				_status(PresentationStatus::NotStarted)
			{}

			std::string name; //!< The name of the slide. Set by the user during slide creation.

			/*! \brief A framebuffer containing image data that will be drawn to the screen during this slide's presentation.
			If drawingFunction points to a function, `framebuffer` will not be drawn and `drawingFunction` will be called instead. */
			std::shared_ptr<ofFbo> framebuffer;

			/*! \brief Pointer to a user function that will be called to draw the slide, rather than using the `framebuffer`.

			Pointer to a user function that will be called to draw the slide.
			If this points to a function, any data in `framebuffer` will be ignored.
			\note It is important to note that if you want to do something other than drawing in this function
			(e.g. examining responses to other stimuli), that the time at which this function is called is not
			the same time at which the slide's contents appear on screen. If you want a function to be called
			right after the contents of this slide appear on screen, use
			\ref CX::CX_SlideBuffer::Slide::slidePresentedCallback instead. */
			std::function<void(void)> drawingFunction;

			/*! \brief Pointer to a user function that will be called right after slide is presented,
			i.e. right after the back buffer containing the slide contents is swapped into the front buffer. */
			std::function<void(void)> slidePresentedCallback;

			SlideTimingInfo intended; //!< The intended timing parameters (i.e. what should have happened if there were no presentation errors).
			SlideTimingInfo actual; //!< The actual timing parameters. Set by whatever presents the slides.

			SlidePresentationInfo presInfo; //!< A struct that may be filled with additional information by 
											//!< the class that presents the slides.
			


			// this function shall be called whenever the caller wants to begin rendering this slide
			void renderSlide(CX_Display* disp);

			// once renderSlide() has been called, the caller shall call updateRenderStatus() as long as isRendering() continues to return true
			// once isRendering() returns false, updateRenderStatus() does not need to be called again
			// updateRenderStatus() shall not be called between a swap and slideSwappedIn() or slideSwappedOut()
			bool isRendering(void) const;
			void updateRenderStatus(void);
			
			// the caller shall call this function after
			// 1. renderSlide was called
			// 2. a swap happened
			void swappedIn(CX_Millis swapTime, FrameNumber swapFrame);

			// the caller shall call this function after
			// 1. renderSlide() was called for this slide
			// 2. a swap happened
			// 3. swappedIn() was called for this slide
			// 4. renderSlide() was called for a different slide
			// 5. a swap happened
			void swappedOut(CX_Millis swapTime, FrameNumber swapFrame);


			bool isInactive(void) const;
			bool isActive(void) const;
			
			bool isPreparingToSwap(void) const;
			bool isPreparedToSwap(void) const;
			bool isOnScreen(void) const;

			void deallocateFramebuffer(void);
			void resetPresentationInfo(void);

		private:

			enum class PresentationStatus : int {
				NotStarted = 0,
				RenderStarted = 1,
				RenderComplete = 2,
				OnScreen = 3,
				Finished = 4
			};

			Sync::GLFenceSync _fenceSync;
			PresentationStatus _status;

		};


		
		struct Configuration {
			CX_Display* display;
			//ofRectangle fboResolution; // all you need from the display...
		};

		CX_SlideBuffer(void);
		CX_SlideBuffer(CX_Display* disp);

		void setup(CX_Display* disp);
		void setup(Configuration config);
		const Configuration& getConfiguration(void) const;

		void beginDrawingNextSlide(CX_Millis timeDuration, std::string slideName = "", FrameNumber frameDuration = 0);
		void endDrawingCurrentSlide(void);

		void appendSlideFunction(CX_Millis timeDuration, std::function<void(void)> drawingFunction, std::string slideName = "", FrameNumber frameDuration = 0);
		void appendSlide(CX_SlideBuffer::Slide slide);

		// modifiers to last slide
		void setLastSlideFrameDuration(FrameNumber frameDuration);


		void clear(void);

		
		

		bool slideExists(std::string name) const;
		Slide* getSlide(std::string name);
		Slide* operator[](std::string name);
		bool deleteSlide(std::string name);

		bool slideExists(size_t index) const;
		Slide* getSlide(size_t index);
		Slide* operator[](size_t index);
		bool deleteSlide(size_t index);

		size_t size(void) const;
		std::vector<Slide>& getSlides(void);


		// ??
		//void lock(void);
		//void unlock(void);
		//bool isLocked(void);

		/*! This struct contains information about errors that were detected during slide presentation.
		See CX_SlideBuffer::checkForPresentationErrors() for how to get this information.

		Note that false positives are possible. For example, when considering late starts, it is possible
		that a slide was actually presented on time, but CX did not learn that the presentation was started
		until after the intended start time.

		It is possible for errors to be counted multiple times. For example, one slide might be copied to
		the back buffer late (1 error) and, as a result, presented late (2 errors), which also means that
		it has an incorrect frame count (3 errors).
		*/
		struct PresentationErrorInfo {
			PresentationErrorInfo(void) :
				presentationErrorsSuccessfullyChecked(false),
				incorrectFrameCounts(0),
				lateCopiesToBackBuffer(0),
				lateStarts(0)
			{}

			/*! \brief The names of all of the slides that had any errors. */
			std::vector< std::string > namesOfSlidesWithErrors;

			/*! \brief `true` if presentation errors were successfully checked for. This does not mean that there were
			no presentation errors, but that there were no presentation error checking errors. */
			bool presentationErrorsSuccessfullyChecked;

			/*! \brief The number of slides for which the actual and intended frame counts did not match,
			indicating that the slide was presented for too many or too few frames.	*/
			unsigned int incorrectFrameCounts;

			/*! \brief The number of slides for which the time at which the slide finished being copied
			to the back buffer was after the actual start time of the slide. */
			unsigned int lateCopiesToBackBuffer;

			/*! \brief The number of slides for which the start time was later than the intended
			start time. */
			unsigned int lateStarts;

			/*! \brief Returns the sum of the different types of errors that are measured. */
			unsigned int totalErrors(void) {
				return incorrectFrameCounts + lateCopiesToBackBuffer + lateStarts;
			}

		};

		CX_SlideBuffer::PresentationErrorInfo checkForPresentationErrors(void) const;
		std::string printLastPresentationInformation(void) const;
		CX_DataFrame getLastPresentationInformation(void) const;

		std::vector<CX_Millis> getActualTimeDurations(void);
		std::vector<uint64_t> getActualFrameDurations(void);

	private:

		bool _appendSlide(Slide&& slide);

		Configuration _config;

		std::vector<Slide> _slides;

		bool _renderingToCurrentSlide;
		std::shared_ptr<Slide> _currentSlide;

		int _namedSlideIndex(std::string name) const;
		
	};


	// This class is not thread safe
	class CX_SlideBufferPlaybackHelper {
	public:

		struct Configuration {

			Configuration(void) :
				slideBuffer(nullptr),
				display(nullptr)
			{}

			CX_SlideBuffer* slideBuffer;
			CX_Display* display;

		};

		bool setup(const Configuration& config);
		const Configuration& getConfiguration(void) const;

		
		bool startPlaying(void);
		void updatePlayback(void);
		bool isPlaying(void);
		void stopPlaying(void);


		void renderNextSlide(void);
		void reRenderCurrentSlide(void);
		void bufferSwap(CX_Millis swapTime, FrameNumber frameNumber);

		bool slideAdvancedOnLastSwap(void);

		bool currentSlideIsFirstSlide(void) const;
		bool currentSlideIsLastSlide(void) const;

		void setIntendedStartFramesUsingTimeDurations(FrameNumber startFrame, CX_Millis nominalFramePeriod);
		void setIntendedStartFramesUsingFrameDurations(FrameNumber startFrame);
		void setIntendedStartTimesUsingTimeDurations(CX_Millis startTime);
		void setIntendedStartTimesUsingFrameDurations(CX_Millis startTime, CX_Millis nominalFramePeriod);

		void setIntendedStartOfRemainingSlidesFromCurrentSlide(bool setTime = true, bool setFrames = true);
		void setIntendedStartTimesOfRemainingSlidesFromCurrentSlide(void);
		void setIntendedStartFramesOfRemainingSlidesFromCurrentSlide(void);

		void resetPresentationInfo(void);

		CX_SlideBuffer::Slide* getPreviousSlide(void);
		CX_SlideBuffer::Slide* getCurrentSlide(void); // on screen
		CX_SlideBuffer::Slide* getNextSlide(void);

		/*
		int getOnScreenSlideIndex(void) {
			if (!_playing) {
				return -1;
			}
			return _currentSlide;
		}
		*/



	private:

		Configuration _config;


		bool _playing;
		int _currentSlide;

		bool _slideAdvancedOnLastSwap;

	};

	// This class is thread-safe
	class CX_SlideBufferPredicatePlayback {
	public:

		struct PredicateArgs {
			bool hasSwapped;
		};

		struct Configuration {

			CX_SlideBuffer* slideBuffer;
			CX_Display* display;

			//CX_SlideBufferPlaybackHelper* helper;

			bool useTimeDurations; // if false, use frame durations
			bool deallocateCompletedSlides;
			bool propagateDelays;

			// choose one
			std::function<bool(void)> shouldSwapPredicate;
			std::function<bool(void)> hasSwappedPredicate;

			// must supply
			std::function<bool(const PredicateArgs&)> renderNextPredicate;

			// may supply
			std::function<bool(const PredicateArgs&)> reRenderCurrentPredicate;

		};

		bool setup(const Configuration& config);
		Configuration getConfiguration(void);

		struct StartConfig {

			StartConfig(void) :
				intendedStartTime(CX_Millis::max()),
				intendedStartFrame(std::numeric_limits<FrameNumber>::max())
			{}

			//bool setStartTimes; // if false, set startFrames (why not set both?)

			//bool useTimeDurations; // if false, use frame durations

			// provide at least one. If both are provided, both will be used
			CX_Millis intendedStartTime;
			FrameNumber intendedStartFrame;
		};

		//bool startPlaying(CX_Millis intendedStartTime, FrameNumber intendedStartFrame);
		bool startPlaying(StartConfig sc);

		//bool startPlaying(void);
		bool isPlaying(void);
		void updatePlayback(void);

		void updatePlaybackSwapping(void);
		void updatePlaybackRendering(void);
		

		void stopPlaying(void);

		//bool play(void);

		typedef Util::LockedPointer<CX_SlideBufferPlaybackHelper, std::recursive_mutex> SlideHelperLP;
		SlideHelperLP getLockedHelperPointer(void);

		typedef Util::LockedPointer<CX_SlideBuffer, std::recursive_mutex> SlideBufferLP;
		SlideBufferLP getSlideBufferLP(void);

	private:

		std::recursive_mutex _mutex;

		Configuration _config;

		CX_SlideBufferPlaybackHelper _helper;

		PredicateArgs _predArgs;

	};

} // namespace CX