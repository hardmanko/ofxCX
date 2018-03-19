#include "CX_SlideBuffer.h"

#include "CX_Logger.h"
#include "CX_Private.h"

namespace CX {

///////////////////////////
// CX_SlideBuffer::Slide //
///////////////////////////

void CX_SlideBuffer::Slide::renderSlide(CX_Display* disp) {

	if (_status >= PresentationStatus::RenderStarted) {
		//warn that slide was re-rendered?
	}

	disp->beginDrawingToBackBuffer();

	if (this->drawingFunction != nullptr) {
		this->drawingFunction();
	} else if (this->framebuffer != nullptr) {
		ofPushStyle();
		ofDisableAlphaBlending();
		ofSetColor(255);
		this->framebuffer->draw(0, 0);
		ofPopStyle();
	}

	disp->endDrawingToBackBuffer();

	_fenceSync.startSync();

	_status = PresentationStatus::RenderStarted;
}

bool CX_SlideBuffer::Slide::isRendering(void) const {
	return _status == PresentationStatus::RenderStarted && _fenceSync.isSyncing();
}

void CX_SlideBuffer::Slide::updateRenderStatus(void) {
	if (!isRendering()) {
		return;
	}

	_fenceSync.updateSync();

	if (_fenceSync.syncComplete()) {
		this->presInfo.renderStartTime = _fenceSync.getStartTime();
		if (_fenceSync.syncSuccess()) {
			this->presInfo.renderCompleteTime = _fenceSync.getSyncTime();

			// It seems like the rendering should be marked as complete regardless of success
			// but not setting it to RenderComplete on sync failure allows other stuff to see
			// that the render did not complete.
			_status = PresentationStatus::RenderComplete;
		}
	}
}

void CX_SlideBuffer::Slide::swappedIn(CX_Millis swapTime, uint64_t swapFrame) {

	if (isInactive()) {
		Instances::Log.error("CX_SlideBuffer") << "Slide \"" << this->name << "\" swapped in when it was inactive.";
		return;
	}

	//if (_status == PresentationStatus::OnScreen) {
	// already on screen. this is legal in case of repeated rendering and swapping
	//}

	this->updateRenderStatus(); // one last check of the fence sync. this does not require a guard, like checking that the fence sync is incomplete.

	if (_status == PresentationStatus::RenderStarted) {
		// warning: swapped before rendering complete
		presInfo.swappedBeforeRenderingComplete = true;
		presInfo.renderCompleteTime = CX_Millis(-1);
	} else if (_status == PresentationStatus::RenderComplete) {
		presInfo.swappedBeforeRenderingComplete = false;
	}

	_fenceSync.clear(); // done with fence sync

	actual.startTime = swapTime;
	actual.startFrame = swapFrame;

	_status = PresentationStatus::OnScreen;

}

void CX_SlideBuffer::Slide::swappedOut(CX_Millis swapTime, uint64_t swapFrame) {
	if (_status != PresentationStatus::OnScreen) {
		Instances::Log.error("CX_SlideBuffer") << "Slide \"" << this->name << "\" swapped out when it was not on screen.";
		// warn: not on screen when swapped out
	}

	actual.timeDuration = swapTime - actual.startTime;
	actual.frameDuration = swapFrame - actual.startFrame;

	_status = PresentationStatus::Finished;

}

bool CX_SlideBuffer::Slide::isInactive(void) const {
	return _status == PresentationStatus::NotStarted || _status == PresentationStatus::Finished;
}

bool CX_SlideBuffer::Slide::isActive(void) const {
	return !isInactive();
}

bool CX_SlideBuffer::Slide::isOnScreen(void) const {
	return _status == PresentationStatus::OnScreen;
}

bool CX_SlideBuffer::Slide::isPreparingToSwap(void) const {
	return _status == PresentationStatus::RenderStarted || _status == PresentationStatus::RenderComplete;
}

bool CX_SlideBuffer::Slide::isPreparedToSwap(void) const {
	return _status == PresentationStatus::RenderComplete;
}

void CX_SlideBuffer::Slide::resetPresentationInfo(void) {
	actual = SlideTimingInfo();
	presInfo = SlidePresentationInfo();

	_status = PresentationStatus::NotStarted;
	_fenceSync.clear();
}

////////////////////
// CX_SlideBuffer //
////////////////////

CX_SlideBuffer::CX_SlideBuffer(void) :
	_renderingToCurrentSlide(false)
{
	setup(&Instances::Disp); // wow, really?
}

CX_SlideBuffer::CX_SlideBuffer(CX_Display* disp) :
	CX_SlideBuffer()
{
	setup(disp);
}

void CX_SlideBuffer::setup(CX_Display* disp) {
	Configuration config;
	config.display = disp;
	setup(config);
}

void CX_SlideBuffer::setup(Configuration config) {
	_config = config;
}

CX_SlideBuffer::Configuration& CX_SlideBuffer::getConfiguration(void) {
	return _config;
}


bool CX_SlideBuffer::_appendSlide(Slide&& slide) {

	if (slide.name == "") {
		slide.name = "Slide " + ofToString(_slides.size() + 1);
	}

	if (slide.intended.timeDuration <= CX_Millis(0) && slide.intended.frameDuration == 0) {
		CX::Instances::Log.warning("CX_SlideBuffer") << "Slide named \"" << slide.name << "\" with timeDuration <= 0 and frameDuration == 0 ignored.";
		return false;
	}

	bool fboReady = slide.framebuffer != nullptr && slide.framebuffer->isAllocated();
	if (!fboReady && slide.drawingFunction == nullptr) {
		CX::Instances::Log.error("CX_SlideBuffer") << "appendSlide(): For slide named \"" << slide.name << 
			"\", the framebuffer was not allocated and the drawing function was a nullptr, so the frame was ignored.";
		return false;
	}

	//if (_slides.size() > 0) {
	//	Slide& prevSlide = _slides.back();
	//	prevSlide.intended.timeDuration = slide.intended.startTime - prevSlide.intended.startTime;
	//}

	_slides.push_back(std::move(slide));

	CX::Instances::Log.verbose("CX_SlideBuffer") << "Slide #" << (_slides.size() - 1) << " (" << _slides.back().name <<
		") appended.";

	return true;
}

/*! Add a fully configured slide to the end of the list of slides.
Use of this function is discouraged. It is better to use `beginDrawingNextSlide()` or `appendSlideFunction()`.

The user code must configure a few components of the slide:

+ If the framebuffer will be used, the framebuffer must be allocated and drawn to.
+ If the drawing function will be used, a valid function pointer must be given. A check is made that either the
drawing function is set or the framebuffer is allocated and an error is logged if neither is configured.
+ The intended duration must be set.
+ The name may be set (optional). If equal to the empty string (`""`; the default), the name will be set to
"Slide N", where N is the slide number, indexed from 0.

\param slide The slide to append.
*/
void CX_SlideBuffer::appendSlide(Slide slide) {
	endDrawingCurrentSlide();

	_appendSlide(std::move(slide));
}

/*! Appends a slide to the slide presenter that will call the given drawing function when it comes time
to render the slide to the back buffer. This approach has the advantage over using framebuffers that
it takes essentially zero time to append a function to the list of slides, whereas a framebuffer must
be allocated, which takes time. Additionally, because framebuffers must be allocated, they use video
memory, so if you are using a very large number of slides, you could potentially run out of video memory.
Also, when it comes time to draw the slide to the back buffer, it may be faster to draw directly to the
back buffer than to copy an FBO to the back buffer (although this depends on various factors).

\param drawingFunction A pointer to a function that will draw the slide to the back buffer. The contents of
the back buffer are not cleared before this function is called, so the function must clear the background
to the desired color.
\param slideDuration The amount of time to present the slide for. If this is less than or equal to 0, the slide will be ignored.
\param slideName The name of the slide. This can be anything and is purely for the user to use to help identify the slide. If equal
to the empty string (`""`; the default), the name will be set to "Slide N", where N is the slide number, indexed from 0.

\note See \ref visualStimuli for more information about framebuffers.

One of the most tedious parts of using drawing functions is the fact that they can take no arguments. Here are two
ways to get around that limitation using `std::bind` and function objects ("functors"):

\code{.cpp}
#include "CX.h"

CX_SlidePresenter SlidePresenter;

//This is the function we want to use to draw a stimulus, but it takes two
//arguments. It needs to take 0 arguments in order to be used by the CX_SlidePresenter.
void drawRectangle(ofRectangle r, ofColor col) {
	ofBackground(0);
	ofSetColor(col);
	ofRect(r);
}

//One option is to use a functor to shift around where the arguments to the function come from. With a
//functor, like rectFunctor, below, you can define an operator() that takes no arguments directly, but gets its
//data from the position and color members of the structure. Because rectFunctor has operator(), it looks
//like a function and can be called like a function, so you can use instances of it as drawing functions.
struct rectFunctor {
	ofRectangle position;
	ofColor color;
	void operator() (void) {
		drawRectangle(position, color);
	}
};

void runExperiment(void) {

	SlidePresenter.setup(&Disp);


	//Here we use the functor. We set up the values for position and color and then give the functor to `appendSlideFunction()`.
	rectFunctor rf;
	rf.position = ofRectangle(100, 100, 50, 80);
	rf.color = ofColor(0, 255, 0);
	SlidePresenter.appendSlideFunction(2000.0, "functor rect", rf);


	//The other method is to use std::bind to "bake in" values for the arguments of drawRectangle. We will
	//set up the rectPos and rectColor values to bind to the arguments of drawRectangle.
	ofRectangle rectPos(100, 50, 100, 30);
	ofColor rectColor(255, 255, 0);

	//With the call to std::bind, we bake in the values rectPos and rectColor to their respective arguments,
	//resulting in a function that takes 0 arguments, which we pass into appendSlideFunction().
	SlidePresenter.appendSlideFunction(2000.0, "bind rect", std::bind(drawRectangle, rectPos, rectColor));


	SlidePresenter.startSlidePresentation();
	while (SlidePresenter.isPresentingSlides()) {
		SlidePresenter.update();
	}
}
\endcode
*/
void CX_SlideBuffer::appendSlideFunction(CX_Millis timeDuration, std::function<void(void)> drawingFunction, std::string slideName, uint64_t frameDuration) {

	endDrawingCurrentSlide();

	if (drawingFunction == nullptr) {
		CX::Instances::Log.error("CX_SlideBuffer") << "appendSlideFunction(): nullptr to drawing function given.";
		return;
	}

	Slide slide;
	slide.name = slideName;
	slide.intended.timeDuration = timeDuration;
	slide.intended.frameDuration = frameDuration;
	slide.drawingFunction = drawingFunction;

	_appendSlide(std::move(slide));

}

/*! Prepares the framebuffer of the next slide for drawing so that any drawing
commands given between a call to beginDrawingNextSlide() and endDrawingCurrentSlide()
will cause stimuli to be drawn to the framebuffer of the next slide.

\param slideDuration The amount of time to present the slide for. If this is less than or equal to 0, the slide will be ignored.
\param slideName The name of the slide. This can be anything and is purely for the user to use to help identify the slide. If equal
to the empty string (`""`; the default), the name will be set to "Slide N", where N is the slide number, indexed from 0.

\code{.cpp}
CX_SlideBuffer sp; //Assume that this has been set up.

sp.beginDrawingNextSlide(2000, "circles");
ofBackground(50);
ofSetColor(255, 0, 0);
ofCirlce(100, 100, 30);
ofCircle(210, 50, 20);
sp.endDrawingCurrentSlide();
\endcode
*/
void CX_SlideBuffer::beginDrawingNextSlide(CX_Millis timeDuration, std::string slideName, uint64_t frameDuration) {

	endDrawingCurrentSlide();

	if (!_config.display->renderingOnThisThread()) {
		CX::Instances::Log.error("CX_SlideBuffer") << "Cannot draw slides while the rendering context is on the display thread. "
			"You must disable the frame queue with CX_DisplayThread::enableFrameQueue()";
		return;
	}

	if (_config.display == nullptr) {
		CX::Instances::Log.error("CX_SlideBuffer") << "Cannot draw slides without a valid CX_Display attached. "
			"Call setup() before calling beginDrawingNextSlide().";
		return;
	}

	// Always make a new one
	_currentSlide = std::make_shared<Slide>();

	_currentSlide->name = slideName;
	_currentSlide->intended.timeDuration = timeDuration;
	_currentSlide->intended.frameDuration = frameDuration;

	CX::Instances::Log.verbose("CX_SlideBuffer") << "Allocating framebuffer...";
	_currentSlide->framebuffer = std::make_shared<ofFbo>();

	ofRectangle resolution = _config.display->getResolution();

	_currentSlide->framebuffer->allocate(resolution.x, resolution.y,
		GL_RGB, //Because we are always drawing over the whole display, there is no reason to have an alpha channel
		CX::Util::getMsaaSampleCount());
	Instances::Log.verbose("CX_SlideBuffer") << "Finished allocating.";

		
		
	// Not done in this class any more
	//_slides.back().intended.frameCount = _calculateFrameCount(slideDuration);

	Instances::Log.verbose("CX_SlideBuffer") << "Beginning to draw to framebuffer.";

	_currentSlide->framebuffer->begin();
	_renderingToCurrentSlide = true;

}

/*! Ends drawing to the framebuffer of the slide that is currently being drawn to. See beginDrawingNextSlide(). Calling this function is optional: It will be called for you as needed. */
void CX_SlideBuffer::endDrawingCurrentSlide(void) {

	if (!_config.display->renderingOnThisThread()) {
		return;
	}

	if (_currentSlide == nullptr) {
		// Nothing being drawn
		return;
	}

	if (_renderingToCurrentSlide) {
		_currentSlide->framebuffer->end();
		_renderingToCurrentSlide = false;
	}

	_appendSlide(std::move(*_currentSlide));

	_currentSlide = nullptr;

}

/*! \brief Clears all of the slides contained in the slide buffer. */
void CX_SlideBuffer::clear(void) {
	_slides.clear();
}

bool CX_SlideBuffer::slideExists(std::string name) const {
	int index = _namedSlideIndex(name);
	return index > 0;
}

/*! Gets a pointer to the slide with the given name, if any. If the named slide is not found,
a `nullptr` is returned. It is the users responsibility to either


exception is thrown and an error is logged (although you will never see the log
message unless the exception is caught).
\param name The name of the slide to get.
\return A reference to the named slide.
\note Because the user supplies slide names, there is no guarantee that any given slide name will
be unique. Because of this, this function simply returns a reference to the first slide for which
the name matches.
*/
CX_SlideBuffer::Slide* CX_SlideBuffer::getSlide(std::string name) {
	int index = _namedSlideIndex(name);
	if (index < 0) {
		Instances::Log.error("CX_SlideBuffer") << "getSlide(): No slide found with name \"" << name << "\".";
		return nullptr;
	}
	return getSlide((unsigned int)index);
}

CX_SlideBuffer::Slide* CX_SlideBuffer::operator[](std::string name) {
	return getSlide(name);
}

CX_SlideBuffer::Slide* CX_SlideBuffer::operator[](unsigned int index) {
	return getSlide(index);
}

bool CX_SlideBuffer::deleteSlide(std::string name) {
	int index = _namedSlideIndex(name);
	if (index < 0) {
		return false;
	}
	_slides.erase(_slides.begin() + index);
	return true;
}

int CX_SlideBuffer::_namedSlideIndex(std::string name) const {
	for (int i = 0; i < _slides.size(); i++) {
		if (_slides[i].name == name) {
			return i;
		}
	}
	return -1;
}

bool CX_SlideBuffer::slideExists(unsigned int index) const {
	return index < _slides.size();
}

CX_SlideBuffer::Slide* CX_SlideBuffer::getSlide(unsigned int index) {
	if (!slideExists(index)) {
		Instances::Log.error("CX_SlideBuffer") << "getSlide(): No slide found at index \"" << index << "\".";
		return nullptr;
	}
	return &_slides[index];
}

bool CX_SlideBuffer::deleteSlide(unsigned int index) {
	if (!slideExists(index)) {
		return false;
	}
	_slides.erase(_slides.begin() + index);
	return true;
}

unsigned int CX_SlideBuffer::size(void) const {
	return _slides.size();
}

std::vector<CX_SlideBuffer::Slide>& CX_SlideBuffer::getSlides(void) {
	return _slides;
}

void CX_SlideBuffer::setFramesFromTimes(uint64_t startFrame, CX_Millis framePeriod) {

	for (unsigned int i = 0; i < _slides.size(); i++) {
		
		Slide& slide = _slides[i];

		slide.intended.startFrame = startFrame;
		slide.intended.frameDuration = Util::round(slide.intended.timeDuration / framePeriod, 0, Util::Rounding::ToNearest);
		if (slide.intended.frameDuration == 0) {
			slide.intended.frameDuration = 1;
		}
		
		startFrame += slide.intended.frameDuration;

	}

}

void CX_SlideBuffer::setIntendedStartTimesFromDurations(CX_Millis startTime) {
	for (unsigned int i = 0; i < _slides.size(); i++) {

		Slide& slide = _slides[i];

		slide.intended.startTime = startTime;

		startTime += slide.intended.timeDuration;

	}
}





/*! Checks the timing data from the last presentation of slides for presentation errors. Currently it checks to
see if the intended frame count matches the actual frame count of each slide, which indicates if the duration was
correct. It also checks to make sure that the framebuffer was copied to the back buffer before the onset of the
slide. If not, vertical tearing might have occurred when the back buffer, containing a partially copied slide, was
swapped in.
\return A struct with information about the errors that occurred on the last presentation of slides.
\note If clearSlides() has been called since the end of the presentation, this does nothing as its data has been cleared.
\note If this function is called during slide presentation, the returned struct will have the presentationErrorsSuccessfullyChecked
member set to false and an error will be logged.
*/
CX_SlideBuffer::PresentationErrorInfo CX_SlideBuffer::checkForPresentationErrors(void) const {

	CX_SlideBuffer::PresentationErrorInfo errors;

	for (unsigned int i = 0; i < _slides.size(); i++) {
		const CX_SlideBuffer::Slide &sl = _slides.at(i);

		bool errorOnThisSlide = false;

		if (sl.intended.frameDuration != sl.actual.frameDuration) {
			//This error does not apply to the last slide because the duration of the last slide is undefined.
			if (i != _slides.size() - 1) {
				errors.incorrectFrameCounts++;
				errorOnThisSlide = true;
			}
		}

		if (sl.presInfo.swappedBeforeRenderingComplete) {
		//if (sl.presInfo.renderCompleteTime > sl.actual.startTime) {
			errors.lateCopiesToBackBuffer++;
			errorOnThisSlide = true;
		}

		if (sl.actual.startTime > sl.intended.startTime) {
			errors.lateStarts++;
			errorOnThisSlide = true;
		}

		if (errorOnThisSlide) {
			errors.namesOfSlidesWithErrors.push_back(sl.name);
		}
	}

	//errors.presentationErrorsSuccessfullyChecked = true;
	return errors;
}


/*! This function prints a ton of data relating to the last presentation of slides.
It prints the total number of errors and the types of the errors. For each slide,
it prints the slide index and name, and various information about the slide presentation
timing. 

All of the printed information can also be accessed by with getSlide().

\return A string containing formatted presentation information. Errors are marked with two
asterisks (**).
*/
std::string CX_SlideBuffer::printLastPresentationInformation(void) const {
	PresentationErrorInfo errors = checkForPresentationErrors();

	std::stringstream s;

	s << "Errors: " << errors.totalErrors() << endl;
	if (errors.totalErrors() > 0) {
		s << "Incorrect frame counts: " << errors.incorrectFrameCounts << endl;
		s << "Late copies to back buffer: " << errors.lateCopiesToBackBuffer << endl;
	}
	s << endl;

	for (unsigned int i = 0; i < _slides.size(); i++) {

		const Slide& slide = _slides[i];

		s << "-----------------------------------" << endl;
		s << "Index: " << i;
		s << " Name: " << slide.name << endl;

		s << "Measure:\tIntended,\tActual" << endl;
		s << "Start time: \t" << slide.intended.startTime << ", " << slide.actual.startTime;

		if (slide.actual.startTime > slide.intended.startTime) {
			s << "**";
		}
		s << endl;

		s << "Time duration:   \t" << slide.intended.timeDuration << ", " << slide.actual.timeDuration << endl;


		s << "Start frame:\t" << slide.intended.startFrame << ", " << slide.actual.startFrame << endl;

		s << "Frame duration:\t" << slide.intended.frameDuration << ", " << slide.actual.frameDuration;
		if (slide.intended.frameDuration != slide.actual.frameDuration) {
			if (i != (_slides.size() - 1)) {
				s << "**"; //Mark the error, but not for the last slide
			}
		}
		s << endl;

		s << "Render start: " << slide.presInfo.renderStartTime.millis() << std::endl <<
			"Render complete: " << slide.presInfo.renderCompleteTime.millis();
		if (slide.presInfo.swappedBeforeRenderingComplete) {
			s << "**"; //Mark the error
		}

		s << std::endl << std::endl;
	}

	return s.str();
}

/*! This function produces a CX_DataFrame with the following information related to slide
presentation for each slide (drawn directly from the CX_SlideBuffer::Slide struct used by each
slide): name, intended and actual timing information, and copyToBackBufferCompleteTime. In
addition, the slide index is given.

The column names are "index", "name", "copyToBackBufferCompleteTime",
"actual.startTime", "actual.timeDuration", "actual.startFrame", and "actual.frameDuration".
Plus, for the intended timings, replace "actual" with "intended" for the 4 intended timings
columns.

\return The data frame.
*/
CX_DataFrame CX_SlideBuffer::getLastPresentationInformation(void) const {
	CX_DataFrame df;

	for (unsigned int i = 0; i < _slides.size(); i++) {

		const Slide& slide = _slides[i];

		df(i, "index") = i;

		df(i, "name") = slide.name;

		df(i, "actual.startTime") = slide.actual.startTime;
		df(i, "actual.timeDuration") = slide.actual.timeDuration;
		df(i, "actual.startFrame") = slide.actual.startFrame;
		df(i, "actual.frameDuration") = slide.actual.frameDuration;

		df(i, "intended.startTime") = slide.intended.startTime;
		df(i, "intended.timeDuration") = slide.intended.timeDuration;
		df(i, "intended.startFrame") = slide.intended.startFrame;
		df(i, "intended.frameDuration") = slide.intended.frameDuration;

		df(i, "presInfo.renderStartTime") = slide.presInfo.renderStartTime;
		df(i, "presInfo.renderCompleteTime") = slide.presInfo.renderCompleteTime;
		df(i, "presInfo.swappedBeforeRenderingComplete") = slide.presInfo.swappedBeforeRenderingComplete;
	}

	return df;
}

/*! Gets a vector containing the durations of the slides from the last presentation of slides.
Note that these durations may be wrong. If checkForPresentationErrors() does not detect any errors,
the durations are likely to be right, but there is no guarantee.
\return A vector containing the durations. The duration corresponding to the first slide added
to the slide presenter will be at index 0.
\note The duration of the last slide is meaningless. As far as the slide presenter is concerned,
as soon as the last slide is put on the screen, it is done presenting the slides. Because the
slide presenter is not responsible for removing the last slide from the screen, it has no idea
about the duration of that slide. */
std::vector<CX_Millis> CX_SlideBuffer::getActualTimeDurations(void) {
	/*
	if (isLocked()) {
		CX::Instances::Log.error("CX_SlidePresenter") << "getActualPresentationDurations called during slide presentation."
			" Wait until presentation is done to call this function.";
		return std::vector<CX_Millis>();
	}
	*/

	vector<CX_Millis> durations(_slides.size());
	for (unsigned int i = 0; i < _slides.size(); i++) {
		durations[i] = _slides[i].actual.timeDuration;
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
std::vector<uint64_t> CX_SlideBuffer::getActualFrameDurations(void) {

	/*
	if (isLocked()) {
		CX::Instances::Log.error("CX_SlidePresenter") << "getActualFrameCounts called during slide presentation."
			" Wait until presentation is done to call this function.";
		return std::vector<uint64_t>();
	}
	*/

	vector<uint64_t> frameCount(_slides.size());
	for (unsigned int i = 0; i < _slides.size(); i++) {
		frameCount[i] = _slides[i].actual.frameDuration;
	}
	return frameCount;
}


//////////////////////////////////
// CX_SlideBufferPlaybackHelper //
//////////////////////////////////

void CX_SlideBufferPlaybackHelper::startPresenting(void) {
	resetPresentationInfo();

	_currentSlide = -1; // the slide before the first on screen slide

	_slideAdvancedOnLastSwap = false;

	_presenting = true;
}

void CX_SlideBufferPlaybackHelper::stopPresenting(void) {
	_presenting = false;
}

bool CX_SlideBufferPlaybackHelper::isPresenting(void) {
	// If there is no next slide, then the current slide is the last slide, at which point presentation is complete
	if (!getNextSlide()) {
		_presenting = false;
	}
	return _presenting;
}

void CX_SlideBufferPlaybackHelper::updatePresentation(void) {

	if (!isPresenting()) {
		return;
	}

	CX_SlideBuffer::Slide* nextSlide = getNextSlide();
	if (nextSlide && nextSlide->isRendering()) {
		nextSlide->updateRenderStatus();
	}

}

// returns true if slides swapped in and out
void CX_SlideBufferPlaybackHelper::bufferSwap(CX_Millis swapTime, uint64_t swapFrame) {

	CX_SlideBuffer::Slide* nextSlide = getNextSlide();

	if (nextSlide) {
		if (!nextSlide->isPreparingToSwap()) {
			_slideAdvancedOnLastSwap = false;
			return; // If the next slide is not preparing to swap, this swap does not change what is on screen
		}

		if (!nextSlide->isPreparedToSwap()) {
			Instances::Log.warning("CX_SlideBufferPlaybackHelper") << "The next slide was not prepared to swap in but a buffer swap took place.";
		}
		
		nextSlide->swappedIn(swapTime, swapFrame);
	}

	CX_SlideBuffer::Slide* currentSlide = getCurrentSlide();
	if (currentSlide) {
		currentSlide->swappedOut(swapTime, swapFrame);
	}

	// Oncea all of the buffer swap logic is complete, then the slide that is now on screen gets set at the current slide
	_currentSlide++;

	_slideAdvancedOnLastSwap = true;
	return;
}

bool CX_SlideBufferPlaybackHelper::slideAdvancedOnLastSwap(void) {
	return _slideAdvancedOnLastSwap;
}

void CX_SlideBufferPlaybackHelper::renderNextSlide(void) {
	CX_SlideBuffer::Slide* nextSlide = getNextSlide();
	if (nextSlide) {
		nextSlide->renderSlide(_display);
	}
}

void CX_SlideBufferPlaybackHelper::rerenderCurrentSlide(void) {
	CX_SlideBuffer::Slide* currentSlide = getCurrentSlide();
	if (currentSlide) {
		currentSlide->renderSlide(_display);
	}
}

/*
CX_SlideBuffer::Slide* CX_SlideBuffer::getPreviousSlide(void) {
	int index = _presData.currentSlide - 1;
	if (!_presData.presenting || index < 0 || index >= _slides.size()) {
		return nullptr;
	}
	return &_slides[index];
}
*/

CX_SlideBuffer::Slide* CX_SlideBufferPlaybackHelper::getCurrentSlide(void) {
	if (!_presenting || _currentSlide < 0 || _currentSlide >= _sb->size()) {
		return nullptr;
	}
	return _sb->getSlide(_currentSlide);
}

CX_SlideBuffer::Slide* CX_SlideBufferPlaybackHelper::getNextSlide(void) {
	int index = _currentSlide + 1;
	if (!_presenting || index < 0 || index >= _sb->size()) {
		return nullptr;
	}
	return _sb->getSlide(index);
}

void CX_SlideBufferPlaybackHelper::setIntendedStartTimesOfRemainingSlidesFromCurrentSlide(void) {

	CX_SlideBuffer::Slide* currentSlide = getCurrentSlide();
	if (!currentSlide) {
		return;
	}

	int nextSlideIndex = _currentSlide + 1;
	CX_Millis nextSlideStartTime = currentSlide->actual.startTime + currentSlide->intended.timeDuration;

	for (int i = nextSlideIndex; i < _sb->size(); i++) {

		CX_SlideBuffer::Slide* slide = _sb->getSlide(i);

		slide->intended.startTime = nextSlideStartTime;

		nextSlideStartTime += slide->intended.timeDuration;
	}
}

void CX_SlideBufferPlaybackHelper::setIntendedStartFramesOfRemainingSlidesFromCurrentSlide(void) {

	CX_SlideBuffer::Slide* currentSlide = getCurrentSlide();
	if (!currentSlide) {
		return;
	}

	int nextSlideIndex = _currentSlide + 1;
	uint64_t nextSlideStartFrame = currentSlide->actual.startFrame + currentSlide->intended.frameDuration;

	for (int i = nextSlideIndex; i < _sb->size(); i++) {

		CX_SlideBuffer::Slide* slide = _sb->getSlide(i);

		slide->intended.startFrame = nextSlideStartFrame;

		nextSlideStartFrame += slide->intended.frameDuration;
	}
}

void CX_SlideBufferPlaybackHelper::resetPresentationInfo(void) {
	_presenting = false;
	_currentSlide = -1;

	for (unsigned int i = 0; i < _sb->size(); i++) {
		_sb->getSlide(i)->resetPresentationInfo();
	}
}


/*
void CX_SlideBufferPlaybackHelper::propagateDelay(unsigned int startIndex, CX_Millis time, uint64_t frames) {

	for (unsigned int i = 0; i < (_slides.size() - 1); i++) {

		const Slide& currentSlide = _slides[i];
		Slide& nextSlide = _slides[i + 1];

		// do it from the beginning to the end

		//currentSlide.presentationComplete()

	}

}
*/



} // namespace CX