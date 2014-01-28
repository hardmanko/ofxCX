#ifndef _CX_CONTINUOUS_SLIDE_PRESENTER_H_
#define _CX_CONTINUOUS_SLIDE_PRESENTER_H_

#include "CX_SlidePresenter.h"

namespace CX {

	class CX_ContinuousSlidePresenter;

	/*
	enum class CX_SP_PresentationStatus {
		STOPPED,
		SYNCHRONIZING,
		PRESENTING
	};
	*/

	class CX_ContinuousSlidePresenter : public CX_SlidePresenter {
	public:

		CX_ContinuousSlidePresenter(void);

		void update(void) override;
		void setUserFunction(std::function<void(CX_UserFunctionInfo_t&)> userFunction);

		/*
		using CX_SlidePresenter::beginDrawingNextSlide;
		using CX_SlidePresenter::endDrawingCurrentSlide;

		using CX_SlidePresenter::setup;
		using CX_SlidePresenter::startSlidePresentation;
		using CX_SlidePresenter::isPresentingSlides;
		using CX_SlidePresenter::getActualPresentationDurations;
		using CX_SlidePresenter::getActualFrameCounts;

		using CX_SlidePresenter::getActiveSlideIndex;
		using CX_SlidePresenter::getSlide;
		*/

	protected:
		std::function<void(CX_UserFunctionInfo_t&)> _userFunction;
		//void (*) (CX_CSPInfo_t&) _userFunction;

		//void _deallocateCompletedSlides (void);

		void _handleLastSlide (void);
		void _finishPreviousSlide (void);
		void _prepareNextSlide (void);

		
	};


}

#endif //_CX_CONTINUOUS_SLIDE_PRESENTER_H_