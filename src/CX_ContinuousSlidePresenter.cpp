#include "CX_ContinuousSlidePresenter.h"

using namespace CX;

void CX_ContinuousSlidePresenter::setUserFunction (std::function<void(CX_CSPInfo_t&)> userFunction) {
	_userFunction = userFunction;
}


void CX_ContinuousSlidePresenter::update (void) {

	if (_presentingSlides) {

		if (_display->hasSwappedSinceLastCheck()) {

			uint64_t currentFrameNumber = _display->getFrameNumber();

			//Was the current frame just swapped in? If so, store information about the swap time.
			if (_slides.at(_currentSlide).slideStatus == CX_Slide_t::SWAP_PENDING) {

				_slides.at(_currentSlide).slideStatus = CX_Slide_t::IN_PROGRESS;

				CX_Micros_t currentSlideOnset = _display->getLastSwapTime();

				_slides.at(_currentSlide).actualOnsetFrameNumber = currentFrameNumber;
				_slides.at(_currentSlide).actualSlideOnset = currentSlideOnset;

				if (_currentSlide == 0) {
					_slides.at(0).intendedOnsetFrameNumber = currentFrameNumber;
					_slides.at(0).intendedSlideOnset = currentSlideOnset; //This is sort of weird, but true.
				}

				//If there was a previous slide.
				if (_currentSlide > 0) {
					CX_Slide_t &previousSlide = _slides.at(_currentSlide - 1);
					previousSlide.slideStatus = CX_Slide_t::FINISHED;

					//"Deallocate" the framebuffer
					previousSlide.framebuffer.allocate(0, 0);

					//Now that the slide is finished, figure out its duration.
					previousSlide.actualSlideDuration = _slides.at(_currentSlide).actualSlideOnset - previousSlide.actualSlideOnset;
					previousSlide.actualFrameCount = _slides.at(_currentSlide).actualOnsetFrameNumber - previousSlide.actualOnsetFrameNumber;
				}

				//If this is the final slide, then we call the user function.
				if (_currentSlide == (_slides.size() - 1)) {
					_handleLastSlide();
				}

				//If there is a slide after the current one. This MUST come after _handleLastSlide(), because if new slides are added, this has to happen for them.
				if ((_currentSlide + 1) < _slides.size()) {
					CX_Slide_t &currentSlide = _slides.at(_currentSlide);
					CX_Slide_t &nextSlide = _slides.at(_currentSlide + 1);

					if (_errorMode == CX_CSP_ErrorMode::PROPAGATE_DELAYS) {
						nextSlide.intendedSlideOnset = currentSlide.actualSlideOnset + currentSlide.intendedSlideDuration;
						nextSlide.intendedOnsetFrameNumber = currentSlide.actualOnsetFrameNumber + currentSlide.intendedFrameCount;
					} else if (_errorMode == CX_CSP_ErrorMode::FIX_TIMING_FROM_FIRST_SLIDE) {
						nextSlide.intendedSlideOnset = currentSlide.intendedSlideOnset + currentSlide.intendedSlideDuration;
						nextSlide.intendedOnsetFrameNumber = currentSlide.intendedOnsetFrameNumber + currentSlide.intendedFrameCount;
					}
				}

			}

			//Is there is a slide after the current one?
			if ((_currentSlide + 1) < _slides.size()) {

				//Is that slide ready to swap in?
				if ( _slides.at(_currentSlide + 1).intendedOnsetFrameNumber <= (currentFrameNumber + 1)) {
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

void CX_ContinuousSlidePresenter::_handleLastSlide (void) {
	CX_CSPInfo_t info;
	info.currentSlideIndex = _currentSlide;
	info.instance = this;
	info.userStatus = CX_CSPInfo_t::CONTINUE_PRESENTATION;
	//info.lastSlide = &_slides.at(_currentSlide - 1);

	unsigned int previousSlideCount = _slides.size();
					
	_userFunction(info);

	//unsigned int newSlides = _slides.size() - previousSlideCount;

	//_deallocateCompletedSlides(); //Or you could just deallocate the last one (not right here).

	//Start from the first new slide and go to the last new slide
	for (unsigned int i = previousSlideCount; i < _slides.size(); i++) {
		double framesInDuration = (double)_slides.at(i).intendedSlideDuration / _display->getFramePeriod();
		_slides.at(i).intendedFrameCount = (uint32_t)ceil(framesInDuration); //Round up. This should be changed later to round-to-nearest.
		_slides.at(i).slideStatus = CX_Slide_t::NOT_STARTED;

		//_slides.at(i).intendedSlideOnset = _slides.at(i - 1).intendedSlideOnset + _slides.at(i - 1).intendedSlideDuration;
		//_slides.at(i).intendedOnsetFrameNumber = _slides.at(i - 1).intendedOnsetFrameNumber + _slides.at(i - 1).intendedFrameCount;
	}

	switch (info.userStatus) {
	case CX_CSPInfo_t::STOP_NOW:
		_presentingSlides = false;

		//The duration of the current slide is set to undefined (user may keep it on screen indefinitely).
		_slides.at(_currentSlide).actualSlideDuration = std::numeric_limits<CX_Micros_t>::max();
		_slides.at(_currentSlide).actualFrameCount = std::numeric_limits<uint32_t>::max();

		//The duration of following slides (if any) are set to 0 (never presented).
		for (unsigned int i = _currentSlide + 1; i < _slides.size(); i++) {
			_slides.at(i).actualSlideDuration = 0;
			_slides.at(i).actualFrameCount = 0;
		}

		//Deallocate all slides?
		for (unsigned int i = 0; i < _slides.size(); i++) {
			_slides.at(i).framebuffer.allocate(0, 0);
		}

		break;

	//case CX_CSPInfo_t::STOP_AFTER_NEXT_SLIDE_ONSET:
	//
	//	break;
	case CX_CSPInfo_t::CONTINUE_PRESENTATION:
		break;
	}
}


void CX_ContinuousSlidePresenter::_deallocateCompletedSlides (void) {
	for (unsigned int i = 0; i < _slides.size(); i++) {
		if (_slides.at(i).slideStatus == CX_Slide_t::FINISHED) {
			_slides.at(i).framebuffer.allocate(0, 0); //Effectively deallocate, but not exactly. Consider switching to pointer to fbo.
		}
	}
}