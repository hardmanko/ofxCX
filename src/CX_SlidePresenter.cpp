#include "CX_SlidePresenter.h"

using namespace CX;
using namespace CX::Instances;

CX_SlidePresenter::CX_SlidePresenter (void) :
	_display(nullptr),
	_presentingSlides(false),
	_synchronizing(false),
	_currentSlide(0),
	_lastFramebufferActive(false),
	_awaitingFenceSync(false),
	_deallocateFramebuffersForCompletedSlides(true),
	_errorMode(CX_SP_ErrorMode::PROPAGATE_DELAYS)
{}

/*! Set up the slide presenter with the given CX_Display as the display.
\param display Pointer to the display to use.
\return False if there was an error during setup, in which case a message will be logged. */
bool CX_SlidePresenter::setup(CX_Display *display) {
	CX_SP_Configuration config;
	config.display = display;
	return setup(config);
}

/*! Set up the slide presenter using the given configuration.
\param config The configuration to use.
\return False if there was an error during setup, in which case a message will be logged.
*/
bool CX_SlidePresenter::setup(const CX_SP_Configuration &config) {
	if (config.display != nullptr) {
		_display = config.display;
	} else {
		Log.error("CX_SlidePresenter") << "setDisplay: display is NULL. Did you forget to set it to point to a CX_Display?";
		return false;
	}

	_userFunction = config.finalSlideCallback;
	_deallocateFramebuffersForCompletedSlides = config.deallocateCompletedSlides;
	_errorMode = config.errorMode;
	return true;
}

/*! Clears (deletes) all of the slides contained in the slide presenter and stops presentation,
if it was in progress. */
void CX_SlidePresenter::clearSlides (void) {
	stopSlidePresentation();
	_slides.clear();
	_currentSlide = 0;
}

/*! Start presenting the slides that are stored in the slide presenter.
After this function is called, calls to update() will advance the state of the slide presentation.
\return False if an error was encountered while starting presentation, in which case messages will
be logged, true otherwise.
*/
bool CX_SlidePresenter::startSlidePresentation (void) {
	if (_display == NULL) {
		Log.error("CX_SlidePresenter") << "Cannot start slide presentation without a valid monitor attached. Use setMonitor() to attach a monitor to the SlidePresenter";
		return false;
	}

	if (_slides.size() <= 0) {
		Log.warning("CX_SlidePresenter") << "Cannot start slide presentation without any slides to present.";
		return false;
	}

	if (!_display->isAutomaticallySwapping()) {
		_display->BLOCKING_setAutoSwapping(true); //This class requires that the monitor be swapping constantly while presenting slides.
		Log.notice("CX_SlidePresenter") << "Display was not set to automatically swap at start of presentation. It was set to swap automatically in order for the slide presentation to occur.";
	}

	if (_slides.size() == 0) {
		Log.warning("CX_SlidePresenter") << "startSlidePresentation was called without any slides to present.";
		return false;
	} else {

		if (_lastFramebufferActive) {
			Log.warning("CX_SlidePresenter") << "startSlidePresentation was called before last slide was finished. Call endDrawingCurrentSlide() before starting slide presentation.";
			endDrawingCurrentSlide();
		}

		//CX_Micros framePeriod = _display->getFramePeriod();

		for (unsigned int i = 0; i < _slides.size(); i++) {
			//This doesn't need to be done here any more, it's done as slides are added
			//_slides.at(i).intended.frameCount = _calculateFrameCount(_slides.at(i).intended.duration);

			_slides.at(i).slideStatus = CX_Slide_t::NOT_STARTED;
		}

		_synchronizing = true;
		_presentingSlides = false;

		//Wait for any ongoing operations to complete before starting frame presentation.
		_display->BLOCKING_waitForOpenGL();

		_display->hasSwappedSinceLastCheck(); //Throw away any very recent swaps.	
	}
	return true;
}

/*! \brief Stops slide presentation. */
void CX_SlidePresenter::stopSlidePresentation(void) {
	_synchronizing = false;
	_presentingSlides = false;
	_awaitingFenceSync = false;
	//_currentSlide = 0; //It's useful to know what slide you were on when you stopped
}


/*! Prepares the framebuffer of the next slide for drawing so that any drawing
commands given between a call to beginDrawingNextSlide() and endDrawingCurrentSlide()
will cause stimuli to be drawn to the framebuffer of the slide.

\param slideDuration The amount of time to present the slide for. If this is less than or equal to 0, the slide will be ignored.
\param slideName The name of the slide. This can be anything and is purely for the user to use to help identify the slide.
*/
void CX_SlidePresenter::beginDrawingNextSlide (CX_Micros slideDuration, string slideName) {

	if (_lastFramebufferActive) {
		Log.verbose("CX_SlidePresenter") << "The previous frame was not finished before new frame started. Call endDrawingCurrentSlide() before starting slide presentation.";
		endDrawingCurrentSlide();
	}

	if (_display == NULL) {
		Log.error("CX_SlidePresenter") << "Cannot draw slides without a valid CX_Display attached. Call setup() before calling beginDrawingNextSlide.";
		return;
	}

	if (slideDuration <= 0) {
		Log.warning("CX_SlidePresenter") << "Slide named \"" << slideName << "\" with duration <= 0 ignored.";
		return;
	}

	_slides.push_back( CX_Slide_t() );

	_slides.back().slideName = slideName;
	Log.verbose("CX_SlidePresenter") << "Allocating framebuffer...";
	_slides.back().framebuffer.allocate(_display->getResolution().x, _display->getResolution().y, GL_RGBA, CX::Util::getSampleCount());
	Log.verbose("CX_SlidePresenter") << "Finished allocating.";
	
	_slides.back().intended.duration = slideDuration;
	_slides.back().intended.frameCount = _calculateFrameCount(slideDuration);

	Log.verbose("CX_SlidePresenter") << "Beginning to draw to framebuffer.";
	_slides.back().framebuffer.begin();
	_lastFramebufferActive = true;

	Log.verbose("CX_SlidePresenter") << "Slide #" << (_slides.size() - 1) << " (" << _slides.back().slideName << ") drawing begun. Frame count: " << _slides.back().intended.frameCount;
}

/*! Ends drawing to the framebuffer of the slide that is currently being drawn to. See beginDrawingNextSlide(). */
void CX_SlidePresenter::endDrawingCurrentSlide (void) {
	_slides.back().framebuffer.end();
	_lastFramebufferActive = false;
}

/*! Add a fully configured slide to the end of the list of slides. The user code
must configure several components of the slide:

+ If the framebuffer will be used, the framebuffer must be allocated and drawn to.
+ If the drawing function will be used, a valid function pointer must be given. A check is made that either the
drawing function is set or the framebuffer is allocated and an error is logged if neither is configured.
+ The intended duration must be set.
+ The name may be set (optional).

\param slide The slide to append.
*/
void CX_SlidePresenter::appendSlide (CX_Slide_t slide) {
	if (slide.intended.duration <= 0) {
		Log.warning("CX_SlidePresenter") << "Slide named \"" << slide.slideName << "\" with duration <= 0 ignored.";
		return;
	}

	if (_lastFramebufferActive) {
		Log.verbose("CX_SlidePresenter") << "appendSlide: The previous slide was not finished before new slide was appended."
			" Call endDrawingCurrentSlide() before appending a slide.";
		endDrawingCurrentSlide();
	}

	if (!slide.framebuffer.isAllocated() && slide.drawingFunction == nullptr) {
		Log.error("CX_SlidePresenter") << "appendSlide: The framebuffer was not allocated and the drawing function was a nullptr.";
		return;
	}

	_slides.push_back( slide );
	_slides.back().intended.frameCount = _calculateFrameCount(slide.intended.duration);

	Log.verbose("CX_SlidePresenter") << "Slide #" << (_slides.size() - 1) << " (" << _slides.back().slideName << ") appended. Frame count: " << _slides.back().intended.frameCount;
}

/*! Appends a slide to the slide presenter that will call the given drawing function when it comes time
to render the slide to the back buffer. Essentially, the drawing function will be called one frame
before the front and back buffers are swapped.
\param drawingFunction A pointer to a function that will draw the data to the slide. The contents of
the back buffer are not clear before this function is called, so the function must clear the background
to the desired color.
\param slideDuration The amount of time to present the slide for. If this is less than or equal to 0, the slide will be ignored.
\param slideName The name of the slide. This can be anything and is purely for the user to use to help identify the slide.
*/
void CX_SlidePresenter::appendSlideFunction (void (*drawingFunction) (void), CX_Micros slideDuration, string slideName) {

	if (slideDuration <= 0) {
		Log.warning("CX_SlidePresenter") << "Slide named \"" << slideName << "\" with duration <= 0 ignored.";
		return;
	}

	if (drawingFunction == NULL) {
		Log.error("CX_SlidePresenter") << "NULL pointer to drawing function given.";
		return;
	}

	if (_lastFramebufferActive) {
		Log.verbose("CX_SlidePresenter") << "appendSlideFunction: The previous slide was not finished before new slide function was appended."
			" Call endDrawingCurrentSlide() before appending a slide function.";
		endDrawingCurrentSlide();
	}

	_slides.push_back( CX_Slide_t() );

	_slides.back().drawingFunction = drawingFunction;
	_slides.back().intended.duration = slideDuration;
	_slides.back().intended.frameCount = _calculateFrameCount(slideDuration);
	_slides.back().slideName = slideName;

	Log.verbose("CX_SlidePresenter") << "Slide #" << (_slides.size() - 1) << " (" << slideName << ") function appended. Frame count: " << _slides.back().intended.frameCount;
}


/*! Get a reference to the vector of slides held by the slide presenter.
If you modify any of the memebers of any of the slides, you do so at your
own risk. This data is mostly useful in a read-only sort of way (when
was that slide presented?).
\return A reference to the vector of slides.
*/
std::vector<CX_Slide_t>& CX_SlidePresenter::getSlides (void) {
	return _slides;
}

/* Get a reference to the slide at a given index.
\param slideIndex The index of the slide to get. This function throws a std::exception of slideIndex is out of range.
\return A reference to the slide. */
/*
CX_Slide_t& CX_SlidePresenter::getSlide(unsigned int slideIndex) {
	if (slideIndex < _slides.size()) {
		return _slides.at(slideIndex);
	} else {
		stringstream m;
		m << "getSlide: slideIndex of " << slideIndex << " out of range. Stored slides " << this->getSlideCount();
		Log.error("CX_SlidePresenter") << m.str();
	std:exception e(m.str().c_str());
		throw(e);
	}
}
*/

/*! Gets a vector containing the durations of the slides from the last presentation of slides.
Note that these durations may be wrong. If checkForPresentationErrors() does not detect any errors,
the durations are likely to be right, but there is no guarantee.
\return A vector containing the durations. The duration corresponding to the first slide added 
to the slide presenter will be at index 0.
\note The duration of the last slide is meaningless. As far as the slide presenter is concerned,
as soon as the last slide is put on the screen, it is done presenting the slides. Because the
slide presenter is not responsible for removing the last slide from the screen, it has no idea
about the duration of that slide. */
std::vector<CX_Micros> CX_SlidePresenter::getActualPresentationDurations (void) {
	if (isPresentingSlides()) {
		Log.error("CX_SlidePresenter") << "getActualPresentationDurations called during slide presentation."
			" Wait until presentation is done to call this function.";
		return std::vector<CX_Micros>();
	}

	vector<CX_Micros> durations(_slides.size());
	for (unsigned int i = 0; i < _slides.size(); i++) {
		durations[i] = _slides[i].actual.duration;
	}
	return durations;
}

/*! Gets a vector containing the number of frames that each of the slides from the last presentation 
of slides was presented for. Note that these frame counts may be wrong. If checkForPresentationErrors()
not detect any errors, the frame counts are likely to be right, but there is no guarantee.
\return A vector containing the frame counts. The frame count corresponding to the first slide added
to the slide presenter will be at index 0.
\note The frame count of the last slide is meaningless. As far as the slide presenter is concerned,
as soon as the last slide is put on the screen, it is done presenting the slides. Because the
slide presenter is not responsible for removing the last slide from the screen, it has no idea
about the duration of that slide. */
std::vector<unsigned int> CX_SlidePresenter::getActualFrameCounts (void) {

	if (isPresentingSlides()) {
		Log.error("CX_SlidePresenter") << "getActualFrameCounts called during slide presentation."
			" Wait until presentation is done to call this function.";
		return std::vector<unsigned int>();
	}

	vector<unsigned int> frameCount(_slides.size());
	for (unsigned int i = 0; i < _slides.size(); i++) {
		frameCount[i] = _slides[i].actual.frameCount;
	}
	return frameCount;
}

/*! Checks the timing data from the last presentation of slides for presentation errors. Currently it checks to
see if the intended frame count matches the actual frame count of each slide, which indicates if the furation was
correct. It also checks to make sure that the framebuffer was copied to the back buffer before the onset of the
slide, which would indicate the potential for vertical tearing.
\return A struct with information about the errors that occurred on the last presentation of slides.
\note If clearSlides() has been called since the end of the presentation, this does nothing as its data has been cleared.
\note If this function is called during slide presentation, the returned struct will have the presentationErrorsSuccessfullyChecked
member set to false and an error will be logged.
*/
CX_SP_PresentationErrors_t CX_SlidePresenter::checkForPresentationErrors(void) {

	CX_SP_PresentationErrors_t errors;

	if (isPresentingSlides()) {
		Log.error("CX_SlidePresenter") << "checkForPresentationErrors called during slide presentation."
			" Wait until presentation is done to call this function.";
		errors.presentationErrorsSuccessfullyChecked = false;
		return errors;
	}

	for (unsigned int i = 0; i < _slides.size(); i++) {
		CX_Slide_t &sl = _slides.at(i);

		if (sl.intended.frameCount != sl.actual.frameCount) {
			//This error does not apply to the last slide because the duration of the last slide is undefined.
			if (i != _slides.size() - 1) {
				errors.incorrectFrameCounts++;
			}
		}

		if (sl.copyToBackBufferCompleteTime > sl.actual.startTime) {
			errors.lateCopiesToBackBuffer++;
		}
	}

	errors.presentationErrorsSuccessfullyChecked = true;
	return errors;
}

/* \brief Get the name of slide that is currently being presented. */
/*
std::string CX_SlidePresenter::getActiveSlideName (void) {
	if (_currentSlide < _slides.size()) {
		return _slides.at(_currentSlide).slideName;
	}
	return "No active slide.";
}
*/

/*! Updates the state of the slide presenter. If the slide presenter is presenting stimuli,
update() must be called very regularly (at least once per millisecond) in order for the slide 
presenter to function. */
void CX_SlidePresenter::update(void) {

	if (_presentingSlides) {

		if (_display->hasSwappedSinceLastCheck()) {

			uint64_t currentFrameNumber = _display->getFrameNumber();

			//Was the current frame just swapped in? If so, store information about the swap time.
			if (_slides.at(_currentSlide).slideStatus == CX_Slide_t::SWAP_PENDING) {

				CX_Micros currentSlideOnset = _display->getLastSwapTime();

				Log.verbose("CX_SlidePresenter") << "Slide #" << _currentSlide << " in progress";

				_slides.at(_currentSlide).slideStatus = CX_Slide_t::IN_PROGRESS;
				_slides.at(_currentSlide).actual.startFrame = currentFrameNumber;
				_slides.at(_currentSlide).actual.startTime = currentSlideOnset;

				if (_currentSlide == 0) {
					_slides.at(0).intended.startFrame = currentFrameNumber;
					_slides.at(0).intended.startTime = currentSlideOnset; //This is sort of weird, but true.
				}

				if (_currentSlide > 0) {
					_finishPreviousSlide();
				}

				if (_currentSlide == (_slides.size() - 1)) {
					_handleFinalSlide();
					if (!_presentingSlides) {
						return;
					}
				}

				//If there is a slide after the current one, prepare it. This MUST come after _handleFinalSlide(), 
				//because if new slides are added, this has to happen for them.
				if ((_currentSlide + 1) < _slides.size()) {
					_prepareNextSlide();
				}
			}

			//Is there is a slide after the current one?
			if ((_currentSlide + 1) < _slides.size()) {
				if (_slides.at(_currentSlide + 1).intended.startFrame <= (currentFrameNumber + 1)) {
					_currentSlide++; //This must happen before the next slide is rendered.
					_renderCurrentSlide();
				}
			}
		}
	} else if (_synchronizing) {
		if (_display->hasSwappedSinceLastCheck()) {
			_currentSlide = 0;
			_renderCurrentSlide();
			_synchronizing = false;
			_presentingSlides = true;
		}
	}

	_waitSyncCheck();
}

void CX_SlidePresenter::_finishPreviousSlide(void) {
	CX_Slide_t &previousSlide = _slides.at(_currentSlide - 1);
	previousSlide.slideStatus = CX_Slide_t::FINISHED;

	Log.verbose("CX_SlidePresenter") << "Slide #" << (_currentSlide - 1) << " marked as finished.";

	if (_deallocateFramebuffersForCompletedSlides) {
		previousSlide.framebuffer.allocate(0, 0); //"Deallocate" the framebuffer
	}

	//Now that the slide is finished, figure out its duration.
	previousSlide.actual.duration = _slides.at(_currentSlide).actual.startTime - previousSlide.actual.startTime;
	previousSlide.actual.frameCount = _slides.at(_currentSlide).actual.startFrame - previousSlide.actual.startFrame;
}

void CX_SlidePresenter::_handleFinalSlide(void) {
	CX_FinalSlideFunctionInfo_t info;
	info.instance = this;
	info.currentSlideIndex = _currentSlide;

	unsigned int previousSlideCount = _slides.size();

	if (_userFunction != nullptr) {
		_userFunction(info);
	}

	//Start from the first new slide and go to the last new slide. This is not strictly neccessary.
	for (unsigned int i = previousSlideCount; i < _slides.size(); i++) {
		_slides.at(i).slideStatus = CX_Slide_t::NOT_STARTED;
	}

	//If there are no new slides, or if the user requested a stop, or if there is no user function, 
	//stop the presentation and fill in info for the final slides.
	if ((previousSlideCount == _slides.size()) || (!_presentingSlides) || (_userFunction == nullptr)) {
		_presentingSlides = false;

		//The duration of the current slide is set to undefined (user may keep it on screen indefinitely).
		_slides.at(_currentSlide).actual.duration = std::numeric_limits<CX_Micros>::max();
		_slides.at(_currentSlide).actual.frameCount = std::numeric_limits<uint32_t>::max();

		//The durations of following slides (if any) are set to 0 (never presented).
		for (unsigned int i = _currentSlide + 1; i < _slides.size(); i++) {
			_slides.at(i).actual.duration = 0;
			_slides.at(i).actual.frameCount = 0;
		}

		//Deallocate all slides from here on
		if (_deallocateFramebuffersForCompletedSlides) {
			for (unsigned int i = _currentSlide; i < _slides.size(); i++) {
				_slides.at(i).framebuffer.allocate(0, 0);
			}
		}
	}
}

void CX_SlidePresenter::_prepareNextSlide(void) {
	CX_Slide_t &currentSlide = _slides.at(_currentSlide);
	CX_Slide_t &nextSlide = _slides.at(_currentSlide + 1);

	if (_errorMode == CX_SP_ErrorMode::PROPAGATE_DELAYS) {
		nextSlide.intended.startTime = currentSlide.actual.startTime + currentSlide.intended.duration;
		nextSlide.intended.startFrame = currentSlide.actual.startFrame + currentSlide.intended.frameCount;
	} else if (_errorMode == CX_SP_ErrorMode::FIX_TIMING_FROM_FIRST_SLIDE) {

		nextSlide.intended.startTime = currentSlide.intended.startTime + currentSlide.intended.duration;
		nextSlide.intended.startFrame = currentSlide.intended.startFrame + currentSlide.intended.frameCount;

		uint64_t endFrameNumber = nextSlide.intended.startFrame + nextSlide.intended.frameCount;

		if (endFrameNumber <= _display->getFrameNumber()) {

			if ((_currentSlide + 2) < _slides.size()) {
				//If the next slide is not the last slide, it may be skipped

				_currentSlide++;

				_finishPreviousSlide();
				nextSlide.actual.duration = 0;
				nextSlide.actual.frameCount = 0;

				Log.error("CX_SlidePresenter") << "Slide #" << _currentSlide << " skipped.";

				_prepareNextSlide(); //Keep skipping slides

				//return;

			} else {
				//The next slide is the last slide and may not be skipped
				Log.error("CX_SlidePresenter") << "The next slide is the last slide and may not be skipped: " << _currentSlide;
			}
		}
	}
}


void CX_SlidePresenter::_waitSyncCheck(void) {
	//By forcing the fence sync to complete before the slide is marked as ready to swap, you run into the potential
	//for a late fence sync paired with an on time write to the back buffer to result in false positives for an error
	//condition. Better a false positive than a miss, I guess.
	if (_awaitingFenceSync) {
		GLenum result = glClientWaitSync(_fenceSyncObject, 0, 10); //GL_SYNC_FLUSH_COMMANDS_BIT can be second parameter. Wait for 10 ns for sync.
		if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
			if (_slides.at(_currentSlide).slideStatus == CX_Slide_t::COPY_TO_BACK_BUFFER_PENDING) {

				_slides.at(_currentSlide).copyToBackBufferCompleteTime = CX::Instances::Clock.getTime();
				_awaitingFenceSync = false;

				_slides.at(_currentSlide).slideStatus = CX_Slide_t::SWAP_PENDING;

				Log.verbose("CX_SlidePresenter") << "Slide #" << _currentSlide << " copied to back buffer";
			} else {
				Log.error("CX_SlidePresenter") << "Slide #" << _currentSlide << " fence sync completed when active slide was not waiting for copy to back buffer.";
				_awaitingFenceSync = false;
			}
		}
	}
}

void CX_SlidePresenter::_renderCurrentSlide(void) {
	if (_slides.at(_currentSlide).drawingFunction != NULL) {
		_display->beginDrawingToBackBuffer();
		_slides.at(_currentSlide).drawingFunction();
		_display->endDrawingToBackBuffer();
	} else {
		_display->drawFboToBackBuffer(_slides.at(_currentSlide).framebuffer);
	}

	Log.verbose("CX_SlidePresenter") << "Slide #" << _currentSlide << " rendering started";

	_fenceSyncObject = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glFlush();
	_awaitingFenceSync = true;
	_slides.at(_currentSlide).slideStatus = CX_Slide_t::COPY_TO_BACK_BUFFER_PENDING;
}


unsigned int CX_SlidePresenter::_calculateFrameCount(CX_Micros duration) {
	double framesInDuration = (double)duration / _display->getFramePeriod();
	framesInDuration = CX::Util::round(framesInDuration, 0, CX::Util::CX_RoundingConfiguration::ROUND_TO_NEAREST);
	return (uint32_t)framesInDuration;
}