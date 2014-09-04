#include "CX_SlidePresenter.h"

#include "CX_Private.h"

using namespace CX;
using namespace CX::Instances;

CX_SlidePresenter::CX_SlidePresenter(void) :
	_presentingSlides(false),
	_synchronizing(false),
	_currentSlide(0),
	_lastFramebufferActive(false),
	_frameNumberOnLastSwapCheck(0)
{}

/*! Set up the slide presenter with the given CX_Display as the display.
\param display Pointer to the display to use.
\return False if there was an error during setup, in which case a message will be logged. */
bool CX_SlidePresenter::setup(CX_Display *display) {
	CX_SlidePresenter::Configuration config;
	config.display = display;
	return setup(config);
}

/*! Set up the slide presenter using the given configuration.
\param config The configuration to use.
\return False if there was an error during setup, in which case a message will be logged.
*/
bool CX_SlidePresenter::setup(const CX_SlidePresenter::Configuration &config) {
	if (config.display == nullptr) {
		Log.error("CX_SlidePresenter") << "setup: display is a null pointer. Did you forget to set it to point to a CX_Display?";
		return false;
	}

	_config = config;

	if (!CX::Private::glFenceSyncSupported()) {
		_config.useFenceSync = false; //Override the setting
		Log.warning("CX_SlidePresenter") << "OpenGL fence sync not supported by the video card in this computer. This means that the slide"
			" presenter will be unable to determine when rendering commands are complete. Normally, the slide presenter uses a fence sync"
			" to verify that all drawing operations have completed by a certain point of time. Typically, that they have completed by the"
			" time at which the front and back buffers are swapped, bringing the new stimulus onscreen. Without fence sync, there is no"
			" way for the slide presenter to know if the drawing has completed by swap time, potentially allowing vertical tearing to go"
			" unnoticed";
	}

	if (_config.swappingMode == SwappingMode::SINGLE_CORE_BLOCKING_SWAPS) {
		//_config.display->setVSync(true, false);
	}

	if (_config.preSwapCPUHoggingDuration > (_config.display->getFramePeriod() - CX_Millis(1))) {
		_config.preSwapCPUHoggingDuration = _config.display->getFramePeriod() - CX_Millis(1);
		Log.warning("CX_SlidePresenter") << "preSwapCPUHoggingDuration was set to a value greater than the frame period minus one millisecond." <<
			" It has been set to the frame period minus one millisecond.";
	}

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
If you do not call update(), nothing will be presented.
\return False if an error was encountered while starting presentation, in which case messages will
be logged, true otherwise.
*/
bool CX_SlidePresenter::startSlidePresentation (void) {
	if (_config.display == nullptr) {
		Log.error("CX_SlidePresenter") << "Cannot start slide presentation without a valid CX_Display attached. Use setup() to attach a CX_Display to the SlidePresenter.";
		return false;
	}

	if (_slides.size() == 0) {
		Log.warning("CX_SlidePresenter") << "startSlidePresentation was called without any slides to present.";
		return false;
	}

	if (_config.swappingMode == SwappingMode::MULTI_CORE) {
		if (!_config.display->isAutomaticallySwapping()) {
			_config.display->setAutomaticSwapping(true);
			Log.notice("CX_SlidePresenter") << "Display was not set to automatically swap at start of presentation. It was set to swap automatically in order for the slide presentation to occur.";
		}
	}

	if (_config.swappingMode == SwappingMode::SINGLE_CORE_BLOCKING_SWAPS) {
		if (_config.display->isAutomaticallySwapping()) {
			_config.display->setAutomaticSwapping(false);
			Log.notice("CX_SlidePresenter") << "Display was set to automatically swap at start of presentation. It was set to not swap automatically in order for the slide presentation to occur.";
		}
	}

	if (_lastFramebufferActive) {
		Log.warning("CX_SlidePresenter") << "startSlidePresentation was called before last slide was finished. Call endDrawingCurrentSlide() before starting slide presentation.";
		endDrawingCurrentSlide();
	}

	for (unsigned int i = 0; i < _slides.size(); i++) {
		_slides.at(i).slideStatus = CX_SlidePresenter::Slide::NOT_STARTED;
	}

	_synchronizing = true;
	_presentingSlides = false;

	//Wait for any ongoing rendering operations to complete before starting slide presentation.
	_config.display->waitForOpenGL();

	if (_config.swappingMode == SwappingMode::MULTI_CORE) {
		this->_hasSwappedSinceLastCheck();
	}

	return true;
}

/*! \brief Stops slide presentation. */
void CX_SlidePresenter::stopSlidePresentation(void) {
	_synchronizing = false;
	_presentingSlides = false;

	for (unsigned int i = 0; i < _slideInfo.size(); i++) {
		_slideInfo[i].awaitingFenceSync = false;
	}
}

/*! Performs a "standard" slide presentation in a single function call as a convenience.
This function calls startSlidePresentation() to begin the presentation and then calls update() and
CX::Instances::Input.pollEvents() continuously as long as isPresentingSlides() returns true.
\return `true` if the slide presentation completed successfully or `false` if the slide presentation
could not be started. */
bool CX_SlidePresenter::presentSlides(void) {
	if (!this->startSlidePresentation()) {
		return false;
	}

	while (this->isPresentingSlides()) {
		this->update();
		CX::Instances::Input.pollEvents();
	}

	return true;
}


/*! Prepares the framebuffer of the next slide for drawing so that any drawing
commands given between a call to beginDrawingNextSlide() and endDrawingCurrentSlide()
will cause stimuli to be drawn to the framebuffer of the slide.

\param slideDuration The amount of time to present the slide for. If this is less than or equal to 0, the slide will be ignored.
\param slideName The name of the slide. This can be anything and is purely for the user to use to help identify the slide.

\code{.cpp}
CX_SlidePresenter sp; //Assume that this has been set up.

sp.beginDrawingNextSlide(2000, "circles");
ofBackground(50);
ofSetColor(255, 0, 0);
ofCirlce(100, 100, 30);
ofCircle(210, 50, 20);
sp.endDrawingCurrentSlide();
\endcode
*/
void CX_SlidePresenter::beginDrawingNextSlide(CX_Millis slideDuration, string slideName) {

	if (_lastFramebufferActive) {
		Log.verbose("CX_SlidePresenter") << "The previous frame was not finished before new frame started. Call endDrawingCurrentSlide() before starting slide presentation.";
		endDrawingCurrentSlide();
	}

	if (_config.display == nullptr) {
		Log.error("CX_SlidePresenter") << "Cannot draw slides without a valid CX_Display attached. Call setup() before calling beginDrawingNextSlide.";
		return;
	}

	if (slideDuration <= CX_Millis(0)) {
		Log.warning("CX_SlidePresenter") << "Slide named \"" << slideName << "\" with duration <= 0 ignored.";
		return;
	}

	_slides.push_back(CX_SlidePresenter::Slide());
	_slideInfo.push_back(ExtraSlideInfo());

	_slides.back().slideName = slideName;
	Log.verbose("CX_SlidePresenter") << "Allocating framebuffer...";
	_slides.back().framebuffer.allocate(_config.display->getResolution().x, _config.display->getResolution().y, GL_RGB, CX::Util::getMsaaSampleCount());
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
void CX_SlidePresenter::appendSlide (CX_SlidePresenter::Slide slide) {
	if (slide.intended.duration <= CX_Millis(0)) {
		Log.warning("CX_SlidePresenter") << "Slide named \"" << slide.slideName << "\" with duration <= 0 ignored.";
		return;
	}

	if (_lastFramebufferActive) {
		Log.verbose("CX_SlidePresenter") << "appendSlide: The previous slide was not finished before new slide was appended."
			" Call endDrawingCurrentSlide() before appending a slide.";
		endDrawingCurrentSlide();
	}

	if (!slide.framebuffer.isAllocated() && (slide.drawingFunction == nullptr)) {
		Log.error("CX_SlidePresenter") << "appendSlide: The framebuffer was not allocated and the drawing function was a nullptr.";
		return;
	}

	_slides.push_back( slide );
	_slideInfo.push_back(ExtraSlideInfo());
	_slides.back().intended.frameCount = _calculateFrameCount(slide.intended.duration);

	Log.verbose("CX_SlidePresenter") << "Slide #" << (_slides.size() - 1) << " (" << _slides.back().slideName << ") appended. Frame count: " << _slides.back().intended.frameCount;
}

/*! Appends a slide to the slide presenter that will call the given drawing function when it comes time
to render the slide to the back buffer. This approach has the advantage over using framebuffers that
it takes essentially zero time to append a function to the list of slides, whereas a framebuffer must
be allocated, which takes time. Additionally, because framebuffers must be allocated, they use video
memory, so if you are using a very large number of slides, you could potentially run out of video memory.
Also, when it comes time to draw the slide to the back buffer, it may be faster to draw directly to the
back buffer than to copy an FBO to the fack buffer (although this depends on various factors).
\param drawingFunction A pointer to a function that will draw the slide to the back buffer. The contents of
the back buffer are not cleared before this function is called, so the function must clear the background
to the desired color.
\param slideDuration The amount of time to present the slide for. If this is less than or equal to 0, the slide will be ignored.
\param slideName The name of the slide. This can be anything and is purely for the user to use to help identify the slide.

\note See \ref framebufferSwapping for more information about framebuffers.

One of the most tedious parts of using drawing functions is the fact that they can take no arguments. Here are two
ways to get around that limitation using std::bind and function objects (functors):

\code{.cpp}
#include "CX_EntryPoint.h"

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
//data from members of the structure like position and color. Because `rectFunctor` has operator(), it looks
//like a function and can be called like a function, so you can use instances of it as drawing functions.
struct rectFunctor {
	ofRectangle position;
	ofColor color;
	void operator() (void) {
		drawRectangle(position, color);
	}
};

void runExperiment(void) {

	SlidePresenter.setup(&Display);


	//Here we use the functor. We set up the values for position and color and then give the functor to appendSlideFunction.
	rectFunctor rf;
	rf.position = ofRectangle(100, 100, 50, 80);
	rf.color = ofColor(0, 255, 0);
	SlidePresenter.appendSlideFunction(rf, 2000.0, "functor rect");


	//The other method is to use std::bind to "bake in" values for the arguments of drawRectangle. We will
	//set up the rectPos and rectColor values to bind to the arguments of drawRectangle.
	ofRectangle rectPos(100, 50, 100, 30);
	ofColor rectColor(255, 255, 0);

	//With the call to std::bind, we bake in the values rectPos and rectColor to their respective arguments,
	//resulting in a function that takes 0 arguments, which we pass into appendSlideFunction().
	SlidePresenter.appendSlideFunction(std::bind(drawRectangle, rectPos, rectColor), 2000.0, "bind rect");


	SlidePresenter.startSlidePresentation();
	while (SlidePresenter.isPresentingSlides()) {
		SlidePresenter.update();
	}
}
\endcode
*/
void CX_SlidePresenter::appendSlideFunction(std::function<void(void)> drawingFunction, CX_Millis slideDuration, string slideName) {

	if (slideDuration <= CX_Millis(0)) {
		Log.warning("CX_SlidePresenter") << "Slide named \"" << slideName << "\" with duration <= 0 ignored.";
		return;
	}

	if (drawingFunction == nullptr) {
		Log.error("CX_SlidePresenter") << "Null pointer to drawing function given.";
		return;
	}

	if (_lastFramebufferActive) {
		Log.verbose("CX_SlidePresenter") << "appendSlideFunction: The previous slide was not finished before new slide function was appended."
			" Call endDrawingCurrentSlide() before appending a slide function.";
		endDrawingCurrentSlide();
	}

	_slides.push_back(CX_SlidePresenter::Slide());
	_slideInfo.push_back(ExtraSlideInfo());

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
std::vector<CX_SlidePresenter::Slide>& CX_SlidePresenter::getSlides (void) {
	return _slides;
}

/*! Gets a reference to the slide with the given name, if found. If the named slide is not found,
a std::out_of_range exception is thrown and an error is logged (although you will never see the log
message unless the exception is caught).
\param name The name of the slide to get.
\return A reference to the named slide.
\note Because the user supplies slide names, there is no guarantee that any given slide name will
be unique. Because of this, this function simply returns a reference to the first slide for which
the name matches.
*/
CX_SlidePresenter::Slide& CX_SlidePresenter::getSlideByName(std::string name) {
	for (unsigned int i = 0; i < _slides.size(); i++) {
		if (_slides[i].slideName == name) {
			return _slides[i];
		}
	}
	std::string errorString = "Slide named \"" + name + "\" not found in CX_SlidePresenter::getSlideByName().";
	CX::Instances::Log.error("CX_SlidePresenter") << errorString;
	throw(std::out_of_range(errorString.c_str()));
}

/* Get a reference to the slide at a given index.
\param slideIndex The index of the slide to get. This function throws a std::exception of slideIndex is out of range.
\return A reference to the slide. */
/*
CX_SlidePresenter::Slide& CX_SlidePresenter::getSlide(unsigned int slideIndex) {
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
std::vector<CX_Millis> CX_SlidePresenter::getActualPresentationDurations(void) {
	if (isPresentingSlides()) {
		Log.error("CX_SlidePresenter") << "getActualPresentationDurations called during slide presentation."
			" Wait until presentation is done to call this function.";
		return std::vector<CX_Millis>();
	}

	vector<CX_Millis> durations(_slides.size());
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
see if the intended frame count matches the actual frame count of each slide, which indicates if the duration was
correct. It also checks to make sure that the framebuffer was copied to the back buffer before the onset of the
slide. If not, vertical tearing might have occurred when the back buffer, containing a partially copied slide, was
swapped in.
\return A struct with information about the errors that occurred on the last presentation of slides.
\note If clearSlides() has been called since the end of the presentation, this does nothing as its data has been cleared.
\note If this function is called during slide presentation, the returned struct will have the presentationErrorsSuccessfullyChecked
member set to false and an error will be logged.
*/
CX_SlidePresenter::PresentationErrorInfo CX_SlidePresenter::checkForPresentationErrors(void) const {

	CX_SlidePresenter::PresentationErrorInfo errors;

	if (isPresentingSlides()) {
		Log.error("CX_SlidePresenter") << "checkForPresentationErrors called during slide presentation."
			" Wait until presentation is done to call this function.";
		errors.presentationErrorsSuccessfullyChecked = false;
		return errors;
	}

	for (unsigned int i = 0; i < _slides.size(); i++) {
		const CX_SlidePresenter::Slide &sl = _slides.at(i);

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

/*! This function prints a ton of data relating to the last presentation of slides.
It prints the total number of errors and the types of the errors. For each slide,
it prints the slide index and name, and various information about the slide presentation
timing. All of the printed information can also be accessed programmatically by using getSlides().
\return A string containing formatted presentation information.
*/
std::string CX_SlidePresenter::printLastPresentationInformation(void) const {
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
		s << " Name: " << slide.slideName << endl;

		s << "Measure:\tIntended,\tActual" << endl;
		s << "Start time: \t" << slide.intended.startTime << ", " << slide.actual.startTime;
		if (slide.intended.startTime > slide.actual.startTime) {
			s << "*";
		}
		s << endl;

		s << "Duration:   \t" << slide.intended.duration << ", " << slide.actual.duration << endl;
		s << "Start frame:\t" << slide.intended.startFrame << ", " << slide.actual.startFrame << endl;

		s << "Frame count:\t" << slide.intended.frameCount << ", " << slide.actual.frameCount;
		if (slide.intended.frameCount != slide.actual.frameCount) {
			if (i != (_slides.size() - 1)) {
				s << "***"; //Mark the error, but not for the last slide
			}
		}
		s << endl;

		s << "Copy to back buffer complete time: " << slide.copyToBackBufferCompleteTime;
		if (slide.copyToBackBufferCompleteTime > slide.actual.startTime) {
			s << "***"; //Mark the error
		}

		s << endl << endl;
	}

	return s.str();
}


void CX_SlidePresenter::_singleCoreThreadedUpdate(void) {

	if (_presentingSlides) {

		//If the current slide should be swapped in
		if ((_slides.at(_currentSlide).slideStatus == CX_SlidePresenter::Slide::SWAP_PENDING) ||
			(!_config.waitUntilFenceSyncComplete && (_slides.at(_currentSlide).slideStatus == CX_SlidePresenter::Slide::COPY_TO_BACK_BUFFER_PENDING)))
		{

			//Check to see if we are within the CPU hogging phase of presentation
			CX_Millis currentTime = CX::Instances::Clock.now();
			if (currentTime >= _hoggingStartTime) {
				_config.display->swapBuffersInThread();
			}
		}

		if (this->_hasSwappedSinceLastCheck()) {

			CX_Millis currentSlideOnset = _config.display->getLastSwapTime();

			CX::Instances::Log.verbose("CX_SlidePresenter") << "Slide #" << _currentSlide << " in progress. Started at " << currentSlideOnset;

			_slides.at(_currentSlide).slideStatus = CX_SlidePresenter::Slide::IN_PROGRESS;
			//_slides.at(_currentSlide).actual.startFrame = currentFrameNumber; //Don't have frame number information.
			_slides.at(_currentSlide).actual.startTime = currentSlideOnset;

			if (_currentSlide == 0) {
				//_slides.at(0).intended.startFrame = currentFrameNumber;
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

				_hoggingStartTime = currentSlideOnset + _slides.at(_currentSlide).intended.duration - _config.preSwapCPUHoggingDuration;

				_currentSlide++;
				_renderCurrentSlide(); //render the next slide. You can do this because you know the buffers will not swap automatically
			}
		}
	}

	if (_synchronizing) {
		_config.display->swapBuffers();
		_currentSlide = 0;
		_renderCurrentSlide();
		_synchronizing = false;
		_presentingSlides = true;

		_hoggingStartTime = CX::Instances::Clock.now();
	}

	_waitSyncCheck();
}

void CX_SlidePresenter::_singleCoreBlockingUpdate(void) {
	static CX_Millis _hoggingStartTime = 0;

	if (_presentingSlides) {

		//If the current slide should be swapped in
		if ((_slides.at(_currentSlide).slideStatus == CX_SlidePresenter::Slide::SWAP_PENDING) ||
			(!_config.waitUntilFenceSyncComplete && (_slides.at(_currentSlide).slideStatus == CX_SlidePresenter::Slide::COPY_TO_BACK_BUFFER_PENDING)))
		{

			//Check to see if we are within the CPU hogging phase of presentation
			CX_Millis currentTime = CX::Instances::Clock.now();
			if (currentTime >= _hoggingStartTime) {

				_config.display->swapBuffers();

				CX_Millis currentSlideOnset = CX::Instances::Clock.now();

				Log.verbose("CX_SlidePresenter") << "Slide #" << _currentSlide << " in progress. Started at " << currentSlideOnset;

				_slides.at(_currentSlide).slideStatus = CX_SlidePresenter::Slide::IN_PROGRESS;
				_slides.at(_currentSlide).actual.startFrame = 0; //Don't have frame number information.
				_slides.at(_currentSlide).actual.startTime = currentSlideOnset;

				if (_currentSlide == 0) {
					_slides.at(0).intended.startFrame = 0;
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

					_hoggingStartTime = currentSlideOnset + _slides.at(_currentSlide).intended.duration - _config.preSwapCPUHoggingDuration;
					Log.verbose("CX_SlidePresenter") << "Slide #" << (_currentSlide + 1) << " hogging start time: " << _hoggingStartTime;

					_currentSlide++;
					_renderCurrentSlide(); //render the next slide. You can do this because you know the buffers will not swap automatically
				}
			}
		}
	}

	if (_synchronizing) {

		//This is kind of a shitty hack to force v-sync
		//CX_Millis syncSwapStart = Clock.now();
		CX_Millis swapStart;
		do {
			swapStart = CX::Instances::Clock.now();
			_config.display->swapBuffers();
			//Log.notice("CX_SlidePresenter") << "swapped during sync";
		} while (CX::Instances::Clock.now() - swapStart < _config.display->getFramePeriod() - CX_Millis(1));

		//Log.notice("CX_SlidePresenter") << "Sync swap duration: " << Clock.now() - syncSwapStart;

		_currentSlide = 0;
		_renderCurrentSlide();
		_synchronizing = false;
		_presentingSlides = true;

		_hoggingStartTime = CX::Instances::Clock.now();
	}

	_waitSyncCheck();
}

void CX_SlidePresenter::_multiCoreUpdate(void) {
	_waitSyncCheck();

	if (_presentingSlides) {

		if (!this->_hasSwappedSinceLastCheck()) {
			return;
		}

		uint64_t currentFrameNumber = _config.display->getFrameNumber();

		//Was the current frame just swapped in? If so, store information about the swap time.
		if ((_slides.at(_currentSlide).slideStatus == CX_SlidePresenter::Slide::SWAP_PENDING) ||
			(_slides.at(_currentSlide).slideStatus == CX_SlidePresenter::Slide::COPY_TO_BACK_BUFFER_PENDING))
		{

			CX_Millis currentSlideOnset = _config.display->getLastSwapTime();

			Log.verbose("CX_SlidePresenter") << "Slide #" << _currentSlide << " in progress. Started at " << currentSlideOnset;

			_slides.at(_currentSlide).slideStatus = CX_SlidePresenter::Slide::IN_PROGRESS;
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
		
	} else if (_synchronizing) {
		if (this->_hasSwappedSinceLastCheck()) {
			_currentSlide = 0;
			_renderCurrentSlide();
			_synchronizing = false;
			_presentingSlides = true;
		}
	}
}

/*! Updates the state of the slide presenter. If the slide presenter is presenting stimuli,
update() must be called very regularly (at least once per millisecond) in order for the slide
presenter to function. If slide presentation is stopped, you do not need to call update() */
void CX_SlidePresenter::update(void) {
	switch (_config.swappingMode) {
	case SwappingMode::MULTI_CORE: return _multiCoreUpdate();
	case SwappingMode::SINGLE_CORE_BLOCKING_SWAPS: return _singleCoreBlockingUpdate();
	//case CX_SlidePresenter::Configuration::SwappingMode::SINGLE_CORE_THREADED_SWAPS: return _singleCoreThreadedUpdate();
	}
}

void CX_SlidePresenter::_finishPreviousSlide(void) {
	CX_SlidePresenter::Slide &previousSlide = _slides.at(_currentSlide - 1);
	previousSlide.slideStatus = CX_SlidePresenter::Slide::FINISHED;

	Log.verbose("CX_SlidePresenter") << "Slide #" << (_currentSlide - 1) << " marked as finished.";

	if (_config.deallocateCompletedSlides) {
		if (previousSlide.drawingFunction == nullptr) { //If there is no drawing function
			previousSlide.framebuffer.allocate(0, 0); //"Deallocate" the framebuffer
		}
	}

	//Now that the slide is finished, figure out its duration.
	previousSlide.actual.duration = _slides.at(_currentSlide).actual.startTime - previousSlide.actual.startTime;
	previousSlide.actual.frameCount = _slides.at(_currentSlide).actual.startFrame - previousSlide.actual.startFrame;
}

void CX_SlidePresenter::_handleFinalSlide(void) {

	unsigned int previousSlideCount = _slides.size();

	if (_config.finalSlideCallback != nullptr) {
		CX_SlidePresenter::FinalSlideFunctionArgs info;
		info.instance = this;
		info.currentSlideIndex = _currentSlide;

		_config.finalSlideCallback(info);
	}

	//Start from the first new slide and go to the last new slide. This is not strictly neccessary.
	for (unsigned int i = previousSlideCount; i < _slides.size(); i++) {
		_slides.at(i).slideStatus = CX_SlidePresenter::Slide::NOT_STARTED;
	}

	//If there are no new slides, or if the user requested a stop, or if there is no user function,
	//stop the presentation and fill in info for the final slides.
	if ((previousSlideCount == _slides.size()) || (!_presentingSlides) || (_config.finalSlideCallback == nullptr)) {
		_presentingSlides = false;

		//The duration of the current slide is set to undefined (user may keep it on screen indefinitely).
		_slides.at(_currentSlide).actual.duration = CX_Millis::max();
		_slides.at(_currentSlide).actual.frameCount = std::numeric_limits<uint32_t>::max();

		//The durations of following slides (if any) are set to 0 (never presented).
		for (unsigned int i = _currentSlide + 1; i < _slides.size(); i++) {
			_slides.at(i).actual.duration = 0;
			_slides.at(i).actual.frameCount = 0;
		}

		//Deallocate all slides from here on
		if (_config.deallocateCompletedSlides) {
			for (unsigned int i = _currentSlide; i < _slides.size(); i++) {
				if (_slides.at(i).drawingFunction == nullptr) { //If there is no drawing function
					_slides.at(i).framebuffer.allocate(0, 0);
				}
			}
		}
	}
}

void CX_SlidePresenter::_prepareNextSlide(void) {
	CX_SlidePresenter::Slide &currentSlide = _slides.at(_currentSlide);
	CX_SlidePresenter::Slide &nextSlide = _slides.at(_currentSlide + 1);

	if (_config.errorMode == ErrorMode::PROPAGATE_DELAYS) {
		if (currentSlide.actual.startTime > currentSlide.intended.startTime) {
			//If it went over time, use the actual time.
			nextSlide.intended.startTime = currentSlide.actual.startTime + currentSlide.intended.duration;
			nextSlide.intended.startFrame = currentSlide.actual.startFrame + currentSlide.intended.frameCount;
		} else {
			//If not over time, use intended start time
			nextSlide.intended.startTime = currentSlide.intended.startTime + currentSlide.intended.duration;
			nextSlide.intended.startFrame = currentSlide.intended.startFrame + currentSlide.intended.frameCount;
		}
	}
	/*
	else if (_config.errorMode == CX_SlidePresenter::ErrorMode::FIX_TIMING_FROM_FIRST_SLIDE) {

		nextSlide.intended.startTime = currentSlide.intended.startTime + currentSlide.intended.duration;
		nextSlide.intended.startFrame = currentSlide.intended.startFrame + currentSlide.intended.frameCount;

		uint64_t endFrameNumber = nextSlide.intended.startFrame + nextSlide.intended.frameCount;

		if (endFrameNumber <= _config.display->getFrameNumber()) {

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
	*/
}


void CX_SlidePresenter::_waitSyncCheck(void) {
	if (!_config.useFenceSync) {
		return;
	}

	for (unsigned int i = 0; i < _slideInfo.size(); i++) {
		if (_slideInfo[i].awaitingFenceSync) {
			GLenum result = glClientWaitSync(_slideInfo[i].fenceSyncObject, 0, 10);
			if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {

				_slides.at(i).copyToBackBufferCompleteTime = CX::Instances::Clock.now();
				_slideInfo[i].awaitingFenceSync = false;

				if (_slides[i].slideStatus == Slide::COPY_TO_BACK_BUFFER_PENDING) {
					_slides[i].slideStatus = Slide::SWAP_PENDING;
					Log.verbose("CX_SlidePresenter") << "Slide #" << i << " copied to back buffer at " <<
						_slides[i].copyToBackBufferCompleteTime;
				} else {
					Log.warning("CX_SlidePresenter") << "Slide #" << i <<
						" fence sync completed when active slide was not waiting for copy to back buffer. At " <<
						_slides[i].copyToBackBufferCompleteTime;
				}
			}
		}
	}
}

void CX_SlidePresenter::_renderCurrentSlide(void) {
	if (_slides.at(_currentSlide).drawingFunction != nullptr) {
		_config.display->beginDrawingToBackBuffer();
		_slides.at(_currentSlide).drawingFunction();
		_config.display->endDrawingToBackBuffer();
	} else {
		//_config.display->copyFboToBackBuffer(_slides.at(_currentSlide).framebuffer);
		_config.display->beginDrawingToBackBuffer();
		ofPushStyle();
		ofDisableAlphaBlending();
		ofSetColor(255);
		_slides.at(_currentSlide).framebuffer.draw(0, 0);
		ofPopStyle();
		_config.display->endDrawingToBackBuffer();
	}

	Log.verbose("CX_SlidePresenter") << "Slide #" << _currentSlide << " rendering started at " << Clock.now();

	if (_config.useFenceSync) {
		_slideInfo.at(_currentSlide).fenceSyncObject = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		glFlush(); //This flush assures that the funce sync object gets pushed into the command queue.
		_slideInfo.at(_currentSlide).awaitingFenceSync = true;

		_slides.at(_currentSlide).slideStatus = CX_SlidePresenter::Slide::COPY_TO_BACK_BUFFER_PENDING;
	} else {
		_slides.at(_currentSlide).slideStatus = CX_SlidePresenter::Slide::SWAP_PENDING;
	}
}

unsigned int CX_SlidePresenter::_calculateFrameCount(CX_Millis duration) {
	double framesInDuration = duration / _config.display->getFramePeriod();
	framesInDuration = CX::Util::round(framesInDuration, 0, CX::Util::CX_RoundingConfiguration::ROUND_TO_NEAREST);
	return (unsigned int)framesInDuration;
}

//This is a bit odd. it is a direct copy of CX_Display::hasSwappedSinceLastCheck(). Why did I reimplement it
//for CX_SlidePresenter? Because if a user is using a slide presenter and they are also checking 
//CX_Display::hasSwappedSinceLastCheck(), it is possible to end up in the strange position of the slide presenter
//having already checked and found a buffer swap, eating it for the purposes of the user code. Another solution to
//this issue is to have CX_Display::hasSwappedSinceLastCheck() take an argument specifying the call site, but that is
//overly complex for no reason.
bool CX_SlidePresenter::_hasSwappedSinceLastCheck(void) {
	uint64_t currentFrameNumber = _config.display->getFrameNumber();
	if (currentFrameNumber != _frameNumberOnLastSwapCheck) {
		_frameNumberOnLastSwapCheck = currentFrameNumber;
		return true;
	}
	return false;
}