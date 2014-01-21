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

				CX_Micros_t currentFrameOnset = _display->getLastSwapTime();

				_slides.at(_currentSlide).actualOnsetFrameNumber = currentFrameNumber;
				_slides.at(_currentSlide).actualSlideOnset = currentFrameOnset;
				_slides.at(_currentSlide).slideStatus = CX_Slide_t::IN_PROGRESS;

				//If on the first frame, some setup must be done for the rest of the frames. The first frame has just swapped in.
				if (_currentSlide == 0) {

					_slides.at(0).intendedSlideOnset = currentFrameOnset; //This is sort of weird, but true.
					_slides.at(0).intendedOnsetFrameNumber = currentFrameNumber;
					
					for (int i = 0; i < _slides.size() - 1; i++) {
						_slides.at(i + 1).intendedSlideOnset = _slides.at(i).intendedSlideOnset + _slides.at(i).intendedSlideDuration;
						_slides.at(i + 1).intendedOnsetFrameNumber = _slides.at(i).intendedOnsetFrameNumber + _slides.at(i).intendedFrameCount;
					}

				}

				//If there was a previous frame, mark it as finished.
				if (_currentSlide > 0) {
					CX_Slide_t &lastSlide = _slides.at(_currentSlide - 1);
					lastSlide.slideStatus = CX_Slide_t::FINISHED;

					//Now that the slide is finished, figure out its duration.
					lastSlide.actualSlideDuration = _slides.at(_currentSlide).actualSlideOnset - lastSlide.actualSlideOnset;
					lastSlide.actualFrameCount = _slides.at(_currentSlide).actualOnsetFrameNumber - lastSlide.actualOnsetFrameNumber;
				}

				//If this is the last frame, then we call the user function.
				if (_currentSlide == (_slides.size() - 1)) {
					CX_CSPInfo_t info;
					info.currentSlideIndex = _currentSlide;
					info.lastSlide = _slides.at(_currentSlide - 1);

					

					//begin rendering to something?
					beginDrawingNextSlide(1,"null");

					_userFunction(info);

					endDrawingCurrentSlide();

					CX_Slide_t &newSlide = _slides.back();

					newSlide.slideName = info.nextSlideInfo.name;
					newSlide.intendedSlideDuration = info.nextSlideInfo.duration;

					double framesInDuration = (double)newSlide.intendedSlideDuration / _display->getFramePeriod();
					newSlide.intendedFrameCount = (uint32_t)ceil(framesInDuration); //Round up. TODO: This should be changed later to round-to-nearest.
					newSlide.slideStatus = CX_Slide_t::NOT_STARTED;

					CX_Slide_t &currentSlide = _slides.at(_currentSlide - 1);

					newSlide.intendedSlideOnset = currentSlide.intendedSlideOnset + currentSlide.intendedSlideDuration;
					newSlide.intendedOnsetFrameNumber = currentSlide.intendedOnsetFrameNumber + currentSlide.intendedFrameCount;
					

					switch (info.userStatus) {
					case CX_CSPInfo_t::STOP_NOW:
						_presentingSlides = false;

						//The duration of the final frame is undefined. These could be 0.
						_slides.back().actualSlideDuration = std::numeric_limits<CX_Micros_t>::max();
						_slides.back().actualFrameCount = std::numeric_limits<uint32_t>::max();
						break;

					//case CX_CSPInfo_t::STOP_AFTER_NEXT_SLIDE_ONSET:
					//
					//	break;
					case CX_CSPInfo_t::CONTINUE_PRESENTATION:
						if (info.nextSlideInfo.nextSlideRendered) {
							//yay
						}

						break;
					}

					
				}

			}

			//Is there is a slide after the current one?
			if ((_currentSlide + 1) < _slides.size()) {

				//Is that slide ready to swap in?
				if ( _slides.at(_currentSlide + 1).intendedOnsetFrameNumber <= (currentFrameNumber + 1)) {
					_currentSlide++;
					_renderNextFrame();
				}
			}
		}
	} else if (_synchronizing) {
		if (_display->hasSwappedSinceLastCheck()) {
			_currentSlide = 0;

			_renderNextFrame();

			_synchronizing = false;
			_presentingSlides = true;
		}
	}

	_waitSyncCheck();
}