#include "CX_SlidePresenter.h"

using namespace CX;

CX_SlidePresenter::CX_SlidePresenter (void) :
	_presentingSlides(false),
	_synchronizing(false),
	_lastFramebufferActive(false),
	_currentSlide(0),
	_awaitingFenceSync(false)
{
}

void CX_SlidePresenter::startSlidePresentation (void) {
	if (_display == NULL) {
		ofLogError("CX_SlidePresenter") << "Cannot start slide presentation without a valid monitor attached. Use setMonitor() to attach a monitor to the SlidePresenter";
		return;
	}

	if (_slides.size() <= 0) {
		ofLogWarning("CX_SlidePresenter") << "Cannot start slide presentation without any slides to present.";
		return;
	}

	if (!_display->isAutomaticallySwapping()) {
		ofLogNotice("CX_SlidePresenter") << "Display was not set to automatically swap at start of presentation. It was set to swap automatically in order for the slide presentation to occur.";
		_display->BLOCKING_setSwappingState(true); //This class requires that the monitor be swapping constantly while presenting slides.
	}

	if (_slides.size() > 0) {

		if (_lastFramebufferActive) {
			ofLogWarning("CX_SlidePresenter") << "startSlidePresentation was called before last slide was finished. Call finishCurrentSlide() before starting slide presentation.";
			finishCurrentSlide();
		}

		//Wait until all openGL operations are complete.
		GLsync fence = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
		glFlush();
		glWaitSync( fence, 0, GL_TIMEOUT_IGNORED );

		uint64_t framePeriod = _display->getFramePeriod();

		for (int i = 0; i < _slides.size(); i++) {

			double framesInDuration = (double)_slides.at(i).intendedSlideDuration / framePeriod;
			framesInDuration = ceil(framesInDuration); //Round up. This should be changed later to round-to-closest.

			_slides.at(i).intendedFrameCount = (uint32_t)framesInDuration;

			_slides.at(i).slideStatus = CX_Slide_t::NOT_STARTED;
		}

		_synchronizing = true;
		_presentingSlides = false;

		_display->hasSwappedSinceLastCheck(); //Throw away any very recent swaps.

	}
}

void CX_SlidePresenter::update (void) {

	if (_presentingSlides) {

		if (_display->hasSwappedSinceLastCheck()) {

			uint64_t currentFrameNumber = _display->getFrameNumber();

			//Was the current frame just swapped in? If so, store information about the swap time.
			if (_slides.at(_currentSlide).slideStatus == CX_Slide_t::SWAP_PENDING) {

				uint64_t currentFrameOnset = _display->getLastSwapTime();

				_slides.at(_currentSlide).actualOnsetFrameNumber = currentFrameNumber;
				_slides.at(_currentSlide).actualSlideOnset = currentFrameOnset;
				_slides.at(_currentSlide).slideStatus = CX_Slide_t::IN_PROGRESS;

				//If on the first frame, some setup must be done for the rest of the frames. The first frame has just swapped in.
				if (_currentSlide == 0) {

					_slides.at(0).intendedSlideOnset = currentFrameOnset; //This is sort of weird, but also sort of true.
					_slides.at(0).intendedOnsetFrameNumber = currentFrameNumber;
					
					for (int i = 0; i < _slides.size() - 1; i++) {
						_slides.at(i + 1).intendedSlideOnset = _slides.at(i).intendedSlideOnset + _slides.at(i).intendedSlideDuration;
						_slides.at(i + 1).intendedOnsetFrameNumber = _slides.at(i).intendedOnsetFrameNumber + _slides.at(i).intendedFrameCount;
					}

				}

				//If there was a previous frame, mark it as finished.
				if (_currentSlide > 0) {
					_slides.at(_currentSlide - 1).slideStatus = CX_Slide_t::FINISHED;
				}

				//If this is the last frame, then we are done presenting frames.
				if (_currentSlide == (_slides.size() - 1)) {
					_presentingSlides = false;

					for (int i = 0; i < _slides.size() - 1; i++) {
						_slides.at(i).actualSlideDuration = _slides.at(i + 1).actualSlideOnset - _slides.at(i).actualSlideOnset;
						_slides.at(i).actualFrameCount = _slides.at(i + 1).actualOnsetFrameNumber - _slides.at(i).actualOnsetFrameNumber;
					}

					//The duration of the final frame is undefined.
					_slides.back().actualSlideDuration = std::numeric_limits<uint64_t>::max();
					_slides.back().actualFrameCount = std::numeric_limits<uint32_t>::max();
				}

			}

			//Is there is a frame after the current one?
			if ((_currentSlide + 1) < _slides.size()) {

				//Is that frame ready to swap in?
				if ( _slides.at(_currentSlide + 1).intendedOnsetFrameNumber <= (currentFrameNumber + 1)) {

					_currentSlide++;
					
					//This code is duplicated below
					_display->drawFboToBackBuffer( _slides.at(_currentSlide).framebuffer );
					_fenceSyncObject = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
					glFlush();
					_awaitingFenceSync = true;
					_slides.at(_currentSlide).slideStatus = CX_Slide_t::COPY_TO_BACK_BUFFER_PENDING;
					//End dupe
				}
			}
		}
	} else if (_synchronizing) {
		if (_display->hasSwappedSinceLastCheck()) {
			_currentSlide = 0;

			//This code is duplicated above
			_display->drawFboToBackBuffer( _slides.at(_currentSlide).framebuffer );
			_fenceSyncObject = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
			glFlush();
			_awaitingFenceSync = true;
			_slides.at(_currentSlide).slideStatus = CX_Slide_t::COPY_TO_BACK_BUFFER_PENDING;
			//End dupe

			_synchronizing = false;
			_presentingSlides = true;
		}
	}


	//By forcing the fence sync to complete before the slide is marked as ready to swap, you run into the potential
	//for a late fence sync paired with an on time write to the back buffer to result in false positives for an error
	//condition. Better a false positive than a miss, I guess.
	if (_awaitingFenceSync) {
		GLenum result = glClientWaitSync(_fenceSyncObject, 0, 100); //GL_SYNC_FLUSH_COMMANDS_BIT can be second parameter. Wait for 100 ns for sync.
		if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
			if (_slides.at(_currentSlide).slideStatus == CX_Slide_t::COPY_TO_BACK_BUFFER_PENDING) {
				
				_slides.at( _currentSlide ).copyToBackBufferCompleteTime = CX::Instances::Clock.getTime();
				_awaitingFenceSync = false;

				_slides.at(_currentSlide).slideStatus = CX_Slide_t::SWAP_PENDING;

				ofLogVerbose("CX_SlidePresenter") << "Fence sync done for slide #" << _currentSlide;
			} else {
				ofLogError("CX_SlidePresenter") << "Fence sync completed when active slide was not waiting for copy to back buffer.";
			}

		}
	}


}

void CX_SlidePresenter::setDisplay (CX_Display *display) {
	if (display != NULL) {
		_display = display;
	} else {
		ofLogError("CX_SlidePresenter") << "Monitor is NULL.";
	}
	
}



void CX_SlidePresenter::clearSlides (void) {
	_slides.clear();
	_currentSlide = 0;
	_presentingSlides = false;
	_synchronizing = false;
}



void CX_SlidePresenter::startNextSlide (string slideName, uint64_t slideDuration) {

	if (_lastFramebufferActive) {
		ofLogVerbose("CX_SlidePresenter") << "The previous frame was not finished before new frame started. Call finishCurrentSlide() before starting slide presentation.";
		finishCurrentSlide();
	}

	if (_display == NULL) {
		ofLogError("CX_SlidePresenter") << "Cannot start slide presentation without a valid monitor attached. Use setMonitor() to attach a monitor to the SlidePresenter";
		return;
	}

	if (slideDuration == 0) {
		ofLogWarning("CX_SlidePresenter") << "Slide named \"" << slideName << "\" with duration 0 ignored.";
		return;
	}

	_slides.push_back( CX_Slide_t() );

	_slides.back().slideName = slideName;
	_slides.back().framebuffer.allocate( _display->getResolution().x, _display->getResolution().y );
	
	_slides.back().intendedSlideDuration = slideDuration;

	_slides.back().framebuffer.begin();
	_lastFramebufferActive = true;

}

void CX_SlidePresenter::finishCurrentSlide (void) {
	_slides.back().framebuffer.end();
	_lastFramebufferActive = false;
}

void CX_SlidePresenter::appendSlide (CX_Slide_t slide) {
	if (slide.intendedSlideDuration == 0) {
		ofLogWarning("CX_SlidePresenter") << "Slide named \"" << slide.slideName << "\" with duration 0 ignored.";
		return;
	}
	_slides.push_back( slide );
}

vector<CX_Slide_t> CX_SlidePresenter::getSlides (void) {
	return _slides;
}

int CX_SlidePresenter::checkForPresentationErrors (void) {

	int presentationErrors = 0;

	for (int i = 0; i < _slides.size(); i++) {
		CX_Slide_t &sl = _slides.at(i);

		if (sl.intendedFrameCount != sl.actualFrameCount) {
			//This error does not apply to the last slide because the duration of the last slide is undefined.
			if (i != _slides.size() - 1) {
				presentationErrors++;
			}
		}

		if (sl.copyToBackBufferCompleteTime > sl.actualSlideOnset) {
			presentationErrors++;
		}
	}

	return presentationErrors;
}