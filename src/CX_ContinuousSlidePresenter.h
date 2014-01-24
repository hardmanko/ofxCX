#ifndef _CX_CONTINUOUS_SLIDE_PRESENTER_H_
#define _CX_CONTINUOUS_SLIDE_PRESENTER_H_

#include "CX_SlidePresenter.h"


namespace CX {

	class CX_ContinuousSlidePresenter;

	struct CX_CSPInfo_t {

		CX_CSPInfo_t (void) :
			instance(nullptr),
			currentSlideIndex(0),
			userStatus(CX_CSPInfo_t::CONTINUE_PRESENTATION)
		{}

		CX_ContinuousSlidePresenter *instance;
		//CX_Slide_t *lastSlide;

		unsigned int currentSlideIndex;
		

		enum {
			CONTINUE_PRESENTATION,
			STOP_NOW
			//STOP_AFTER_NEXT_SLIDE_ONSET //Stupid. If the user wants this, shouldn't they just say stop the next time the user function is called?
			
		} userStatus;
	};

	enum class CX_CSP_ErrorMode {
		PROPAGATE_DELAYS,
		FIX_TIMING_FROM_FIRST_SLIDE
	};

	enum class CX_SP_PresentationStatus {
		STOPPED,
		SYNCHRONIZING,
		PRESENTING
	};

	class CX_ContinuousSlidePresenter : protected CX_SlidePresenter {
	public:

		void update (void);
		void setUserFunction (std::function<void(CX_CSPInfo_t&)> userFunction);


		using CX_SlidePresenter::beginDrawingNextSlide;
		using CX_SlidePresenter::endDrawingCurrentSlide;

		using CX_SlidePresenter::setup;
		using CX_SlidePresenter::startSlidePresentation;
		using CX_SlidePresenter::isPresentingSlides;
		using CX_SlidePresenter::getActualPresentationDurations;
		using CX_SlidePresenter::getActualFrameCounts;

		using CX_SlidePresenter::getActiveSlideIndex;
		using CX_SlidePresenter::getSlide;

	protected:
		std::function<void(CX_CSPInfo_t&)> _userFunction;
		//void (*) (CX_CSPInfo_t&) _userFunction;

		void _deallocateCompletedSlides (void);

		void _handleLastSlide (void);

		CX_CSP_ErrorMode _errorMode;
	};


}

#endif //_CX_CONTINUOUS_SLIDE_PRESENTER_H_