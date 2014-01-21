#include "CX_SlidePresenter.h"


namespace CX {

	struct CX_CSPInfo_t {

		unsigned int currentSlideIndex;
		CX_Slide_t& lastSlide;

		CX_CSP_SlideInfo_t nextSlideInfo;
		enum {
			CONTINUE_PRESENTATION,
			STOP_NOW
			//STOP_AFTER_NEXT_SLIDE_ONSET //Stupid. If the user wants this, shouldn't they just say stop the next time the user function is called?
			
		} userStatus;
	};

	struct CX_CSP_SlideInfo_t {
		bool nextSlideRendered;
		std::string name;
		CX_Micros_t duration;
	};

	class CX_ContinuousSlidePresenter : protected CX_SlidePresenter {
	public:

		void update (void);
		void setUserFunction (std::function<void(CX_CSPInfo_t&)> userFunction);

		using CX_SlidePresenter::setDisplay;
		using CX_SlidePresenter::startSlidePresentation;
		using CX_SlidePresenter::isPresentingSlides;
		using CX_SlidePresenter::getActualPresentationDurations;
		using CX_SlidePresenter::getActualFrameCounts;

	protected:
		std::function<void(CX_CSPInfo_t&)> _userFunction;
	};


}