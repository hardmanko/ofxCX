#include "CX_ContinuousSlidePresenter.h"

using namespace CX;

CX_ContinuousSlidePresenter::CX_ContinuousSlidePresenter(void) :
	_userFunction(nullptr)
{}

void CX_ContinuousSlidePresenter::setUserFunction(std::function<void(CX_UserFunctionInfo_t&)> userFunction) {
	_userFunction = userFunction;
}


void CX_ContinuousSlidePresenter::update (void) {

	if (_presentingSlides) {

		if (_display->hasSwappedSinceLastCheck()) {

			uint64_t currentFrameNumber = _display->getFrameNumber();

			//Was the current frame just swapped in? If so, store information about the swap time.
			if (_slides.at(_currentSlide).slideStatus == CX_Slide_t::SWAP_PENDING) {

				CX_Micros_t currentSlideOnset = _display->getLastSwapTime();

				_slides.at(_currentSlide).slideStatus = CX_Slide_t::IN_PROGRESS;
				_slides.at(_currentSlide).actualOnsetFrameNumber = currentFrameNumber;
				_slides.at(_currentSlide).actualSlideOnset = currentSlideOnset;

				if (_currentSlide == 0) {
					_slides.at(0).intendedOnsetFrameNumber = currentFrameNumber;
					_slides.at(0).intendedSlideOnset = currentSlideOnset; //This is sort of weird, but true.
				}

				if (_currentSlide > 0) {
					_finishPreviousSlide();
				}

				if (_currentSlide == (_slides.size() - 1)) {
					_handleLastSlide();
				}

				//If there is a slide after the current one. This MUST come after _handleLastSlide(), because if new slides are added, this has to happen for them.
				if ((_currentSlide + 1) < _slides.size()) {
					_prepareNextSlide();
				}
			}

			//Is there is a slide after the current one?
			if ((_currentSlide + 1) < _slides.size()) {
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

void CX_ContinuousSlidePresenter::_finishPreviousSlide (void) {
	CX_Slide_t &previousSlide = _slides.at(_currentSlide - 1);
	previousSlide.slideStatus = CX_Slide_t::FINISHED;

	if (_deallocateFramebuffersForCompletedSlides) {
		previousSlide.framebuffer.allocate(0, 0); //"Deallocate" the framebuffer
	}

	//Now that the slide is finished, figure out its duration.
	previousSlide.actualSlideDuration = _slides.at(_currentSlide).actualSlideOnset - previousSlide.actualSlideOnset;
	previousSlide.actualFrameCount = _slides.at(_currentSlide).actualOnsetFrameNumber - previousSlide.actualOnsetFrameNumber;
}

void CX_ContinuousSlidePresenter::_handleLastSlide (void) {
	CX_UserFunctionInfo_t info;
	info.currentSlideIndex = _currentSlide;
	info.instance = this;
	info.userStatus = CX_UserFunctionInfo_t::CONTINUE_PRESENTATION;
	//info.lastSlide = &_slides.at(_currentSlide - 1);

	unsigned int previousSlideCount = _slides.size();
	
	if (_userFunction != nullptr) {
		_userFunction(info);
	}

	//Start from the first new slide and go to the last new slide. This is really not neccessary.
	for (unsigned int i = previousSlideCount; i < _slides.size(); i++) {
		_slides.at(i).slideStatus = CX_Slide_t::NOT_STARTED;
	}

	//If the user requests a stop or if there is no user function, stop presenting.
	if ((info.userStatus == CX_UserFunctionInfo_t::STOP_NOW) || (_userFunction == nullptr)) {
		_presentingSlides = false;

		//The duration of the current slide is set to undefined (user may keep it on screen indefinitely).
		_slides.at(_currentSlide).actualSlideDuration = std::numeric_limits<CX_Micros_t>::max();
		_slides.at(_currentSlide).actualFrameCount = std::numeric_limits<uint32_t>::max();

		//The duration of following slides (if any) are set to 0 (never presented).
		for (unsigned int i = _currentSlide + 1; i < _slides.size(); i++) {
			_slides.at(i).actualSlideDuration = 0;
			_slides.at(i).actualFrameCount = 0;
		}

		//Deallocate all slides from here on
		for (unsigned int i = _currentSlide; i < _slides.size(); i++) {
			_slides.at(i).framebuffer.allocate(0, 0);
		}
	}

}

void CX_ContinuousSlidePresenter::_prepareNextSlide (void) {
	CX_Slide_t &currentSlide = _slides.at(_currentSlide);
	CX_Slide_t &nextSlide = _slides.at(_currentSlide + 1);

	if (_errorMode == CX_SP_ErrorMode::PROPAGATE_DELAYS) {
		nextSlide.intendedSlideOnset = currentSlide.actualSlideOnset + currentSlide.intendedSlideDuration;
		nextSlide.intendedOnsetFrameNumber = currentSlide.actualOnsetFrameNumber + currentSlide.intendedFrameCount;
	} else if (_errorMode == CX_SP_ErrorMode::FIX_TIMING_FROM_FIRST_SLIDE) {
		nextSlide.intendedSlideOnset = currentSlide.intendedSlideOnset + currentSlide.intendedSlideDuration;
		nextSlide.intendedOnsetFrameNumber = currentSlide.intendedOnsetFrameNumber + currentSlide.intendedFrameCount;
	}
}

/*
void CX_ContinuousSlidePresenter::_deallocateCompletedSlides (void) {
	for (unsigned int i = 0; i < _slides.size(); i++) {
		if (_slides.at(i).slideStatus == CX_Slide_t::FINISHED) {
			_slides.at(i).framebuffer.allocate(0, 0); //Effectively deallocate, but not exactly. Consider switching to pointer to fbo.
		}
	}
}
*/