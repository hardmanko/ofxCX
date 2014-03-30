#include "CX_EntryPoint.h"

/*! \file
In this example, we are going to consider two different ways of using the slide presenter
to present stimuli. One method is the one used in the change detection example: Draw the
stimuli to framebuffers that are managed by the SlidePresenter. This approach is fairly
easy to do, but it had a time cost, because allocating and copying all of the framebuffers
takes time. This example builds on the nBack example.

In this example, we are going to contrast using the framebuffer approach with the slide
presenter with the use of drawing functions. The standard framebuffer approach has the 
following major steps:

1) Allocate the framebuffer
2) Draw your stimuli to the famebuffer
3) Copy the framebuffer to the back buffer
4) Swap front and back buffers

Using drawing functions, we avoid steps 1 and 3, which are generally both
very costly in terms of time. Step 2 becomes "Draw your stimuli to the back buffer directly".
For an N-back task, there are no indefinitely-long pauses where you can prepare stimuli: you
have to get the next sitmulus ready within a fixed interval, so using drawing functions may
be the best approach. We are going to examine that issue with this example.

The way in which you use a drawing function with a CX_SlidePresenter is with the appendSlideFunction()
function. This function takes three arguments: the drawing function, the duration for which the stimuli
drawn by that drawing function should be presented, and the name of the slide (optional). When the
stimuli specified by the drawing function are ready to be presented, the drawing function will be called.
In the drawing function, you do not need to call Display.beginDrawingToBackBuffer(); the slide presenter
does that for you. See drawStimulus() below for an example of what a drawing function might look like.

We can compare the performance of the framebuffer and functor approaches by examining how much time is
used for various processes. One thing to consider is the amount of time it takes to allocate the framebuffers
and render your stimuli to the framebuffers.

Another consideration is the amount of time it takes to copy a framebuffer to the back buffer (step 3 above).
This may take longer than drawing a small amount of stimuli directly to the back buffer. The framebuffer approach
provides a nice security blanket: "I know that regardless of whatever is the the framebuffer, it will be copied
in the same amount of time as any other framebuffer." However, if that copying time is longer than any of the
stimulus drawing times, then using framebuffers is less efficient than drawing directly to the back buffer.

We will be using a special kind of function object, known as a functor (http://www.cprogramming.com/tutorial/functors-function-objects-in-c++.html)
to do our drawing. A functor is basically a structure that can be called like a function using operator().
But unlike a normal function, a functor carries data along with it that can be used in the function call.
Thus, a fuctor is a way to have a function without certain arguments for which you can still specify those
arguments (in a sense) by setting data members of the functor. See struct stimulusFunctor below for an
implementation.
*/

CX_DataFrame df;
CX_DataFrame::rowIndex_t trialNumber = 0;
int trialCount = 40;
int lastSlideIndex = 0;
int nBack = 2;

ofColor backgroundColor(50);
ofColor textColor(255);

ofTrueTypeFont letterFont;
ofTrueTypeFont instructionFont;

char targetKey = 'f';
char nonTargetKey = 'j';
string keyReminderInstructions;

CX_Millis stimulusPresentationDuration = 500;
CX_Millis interStimulusInterval = CX_Seconds(1.0/60.0);

CX_SlidePresenter SlidePresenter;
void finalSlideFunction(CX_SlidePresenter::FinalSlideFunctionArgs& info);

//Here are two new functions that draw the stimuli in different ways
void drawStimuliToFramebuffers(CX_SlidePresenter& sp, int trialIndex);
void appendDrawingFunctions(CX_SlidePresenter& sp, int trialIndex);

void drawStimulus(string letter, bool showInstructions);
void drawBlank(void);
void drawFixationSlide(int remainingTime);

void generateTrials(int numberOfTrials);


bool useFramebuffersForStimuli = false; //If true, the standard framebuffer approach will be used. If 
//false, drawing functions will be used.

//This is a "functor": an object that has data members (letter and showInstructions), but can be called
//as a function using operator().
struct stimulusFunctor {
	string letter;
	bool showInstructions;

	void operator()(void) {
		drawStimulus(this->letter, this->showInstructions);
	}
};

vector<stimulusFunctor> stimulusFunctors;


void runExperiment(void) {

	//CX_WindowConfiguration_t winConfig;
	//winConfig.desiredRenderer = ofPtr<ofGLProgrammableRenderer>();
	//reopenWindow(winConfig);

	Display.setFullScreen(false);
	Display.setVSync(true, true);

	//ofSetLogLevel("ofTrueTypeFont", ofLogLevel::OF_LOG_VERBOSE);
	Log.level(CX_LogLevel::LOG_ALL, "ofTrueTypeFont");

	Clock.sleep(1000);

	Log.levelForFile(CX_LogLevel::LOG_ALL, "Last run.txt");
	Log.level(CX_LogLevel::LOG_ALL, "CX_SlidePresenter");

	//Display.BLOCKING_estimateFramePeriod(CX_Seconds(2));
	Log.notice() << "Frame period: " << Display.getFramePeriod() << " (" << Display.getFramePeriodStandardDeviation() << ")";
	
	Input.setup(true, false); //Use keyboard, not mouse.

	letterFont.loadFont(OF_TTF_SANS, 26);
	instructionFont.loadFont(OF_TTF_SANS, 12);

	stringstream s;
	s << "Press '" << targetKey << "' for targets and '" << nonTargetKey << "' for non-targets";
	keyReminderInstructions = s.str();

	

	generateTrials(10);

	CX_SlidePresenter::Configuration config;
	config.display = &Display;
	config.swappingMode = CX_SlidePresenter::Configuration::MULTI_CORE;
	config.finalSlideCallback = &finalSlideFunction;
	config.deallocateCompletedSlides = useFramebuffersForStimuli; //Only deallocate if using framebuffers

	config.preSwapCPUHoggingDuration = 3;
	config.useFenceSync = true;
	config.waitUntilFenceSyncComplete = false;

	SlidePresenter.setup(config);


	//Start loading slides into the SlidePresenter. Load up a little countdown-to-start screen.
	//We'll always use framebuffers for this because it isn't timing-critical. You can use a
	//mixture of framebuffers and drawing functions with a slide presenter.
	for (int i = 3; i > 0; i--) {
		if (useFramebuffersForStimuli) {
			SlidePresenter.beginDrawingNextSlide(1000, "fixation");
			drawFixationSlide(i);
		} else {
			//std::bind sort of "bakes in" the value of i to drawFixationSlide, and the resulting function
			//takes no arguments, so it can be given to the slide presenter as a drawing function.
			SlidePresenter.appendSlideFunction( std::bind(drawFixationSlide, i), 1000, "fixation" );
		}
	}

	for (unsigned int i = 0; i <= nBack; i++) {
		if (useFramebuffersForStimuli) {
			drawStimuliToFramebuffers(SlidePresenter, i);
		} else {
			appendDrawingFunctions(SlidePresenter, i);
		}
	}
	trialNumber = nBack;

	Log.flush();

	SlidePresenter.startSlidePresentation();

	while (SlidePresenter.isPresentingSlides()) {
		SlidePresenter.update();
		Input.pollEvents();
	}

	df.printToFile("N-Back output.txt");

	//Calling this function can give us a lot of information about the last presentation of slides.
	Log.notice() << "Slide presentation information: " << endl << SlidePresenter.printLastPresentationInformation();


	vector<CX_SlidePresenter::Slide>& slides = SlidePresenter.getSlides();
	CX_Millis startMinusCopySum = 0;
	for (int i = 0; i < slides.size(); i++) {
		startMinusCopySum += slides[i].actual.startTime - slides[i].copyToBackBufferCompleteTime;
	}
	Log.notice() << "Average difference between back buffer copy completion and slide start: " << startMinusCopySum / slides.size();
	
	if (Display.isFullscreen()) {
		Display.setFullScreen(false);
	}

	if (Display.isAutomaticallySwapping()) {
		Display.BLOCKING_setAutoSwapping(false);
	}

	Display.beginDrawingToBackBuffer();
	ofBackground(backgroundColor);
	Draw::centeredString(Display.getCenterOfDisplay(), "Experiment complete!\nPress any key to exit.", letterFont);
	Display.endDrawingToBackBuffer();
	Display.BLOCKING_swapFrontAndBackBuffers();

	Log.flush();

	while (!Input.pollEvents())
		;
}

void finalSlideFunction(CX_SlidePresenter::FinalSlideFunctionArgs& info) {

	bool validResponseMade = false;
	if (Input.Keyboard.availableEvents() > 0) {
		CX_SlidePresenter::Slide &lastStimulusSlide = SlidePresenter.getSlides().at(info.currentSlideIndex - 1);
		CX_Millis stimulusOnset = lastStimulusSlide.actual.startTime;

		while (Input.Keyboard.availableEvents() > 0) {
			CX_Keyboard::Event kev = Input.Keyboard.getNextEvent();
			if ((kev.eventTime >= stimulusOnset) && (kev.eventType == CX_Keyboard::Event::PRESSED) && (kev.key == targetKey || kev.key == nonTargetKey)) {

				if (kev.key == targetKey) {
					df(trialNumber, "responseType") = "target";
				} else if (kev.key == nonTargetKey) {
					df(trialNumber, "responseType") = "nonTarget";
				}

				df(trialNumber, "responseLatency") = kev.eventTime - stimulusOnset;

				validResponseMade = true;
				Input.Keyboard.clearEvents();
			}
		}
	}

	if (!validResponseMade) {
		df(trialNumber, "responseType") = "noValidResponse";
		df(trialNumber, "responseLatency") = 0;
	}

	if (++trialNumber < trialCount) {
		if (useFramebuffersForStimuli) {
			drawStimuliToFramebuffers(SlidePresenter, trialNumber);
		} else {
			appendDrawingFunctions(SlidePresenter, trialNumber);
		}
	}
}

void generateTrials(int numberOfTrials) {

	trialCount = numberOfTrials;

	string letterArray[8] = { "A", "F", "H", "L", "M", "P", "R", "Q" };
	vector<string> letters = arrayToVector(letterArray, 8);

	vector<bool> targetTrial = RNG.sampleRealizations(trialCount, std::bernoulli_distribution(.4));

	for (int i = 0; i < nBack; i++) {
		df(i, "letter") = RNG.sample(letters);
	}

	for (int i = nBack; i < trialCount; i++) {
		if (targetTrial[i]) {
			df(i, "trialType") = "target";
			df(i, "letter") = df(i - nBack, "letter").toString();
		} else {
			df(i, "trialType") = "nonTarget";
			df(i, "letter") = RNG.randomExclusive(letters, df(i - nBack, "letter").toString());
		}
	}

	//If not using framebuffers, set up the functors with the letters and whether they should include the instructions.
	if (!useFramebuffersForStimuli) {
		stimulusFunctors.resize(trialCount);
		for (int i = 0; i < trialCount; i++) {
			stimulusFunctors[i].letter = df(i, "letter").toString();
			if (i < nBack) {
				stimulusFunctors[i].showInstructions = false;
			} else {
				stimulusFunctors[i].showInstructions = true;
			}
		}
	}

	cout << df.print() << endl;
	cout << endl;
}

void drawStimuliToFramebuffers(CX_SlidePresenter& sp, int trialIndex) {
	CX_Millis startTime = Clock.now();

	sp.beginDrawingNextSlide(stimulusPresentationDuration, "stimulus");
	string letter = df(trialIndex, "letter").toString();
	drawStimulus(letter, (trialIndex >= nBack));
	sp.endDrawingCurrentSlide();

	sp.beginDrawingNextSlide(interStimulusInterval, "blank");
	drawBlank();
	sp.endDrawingCurrentSlide();

	CX_Millis renderingDuration = Clock.now() - startTime;
	Log.notice() << "framebuffer rendering duration: " << renderingDuration;
}

void appendDrawingFunctions(CX_SlidePresenter& sp, int trialIndex) {
	CX_Millis startTime = Clock.now();

	//Because stimulusFunctors contains objects that can be called as functions, you can treat 
	//an instance of the object as though it were a function.
	sp.appendSlideFunction(stimulusFunctors[trialIndex], stimulusPresentationDuration, "stimulus");

	//You can also accomplish the same thing using std::bind:
	//sp.appendSlideFunction( std::bind( drawStimulus, df(trialIndex, "letter").toString(), trialIndex >= nBack ), stimulusPresentationDuration, "stimulus" );

	sp.appendSlideFunction(drawBlank, interStimulusInterval, "blank");

	CX_Millis appendingDuration = Clock.now() - startTime;
	Log.notice() << "Drawing function appending duration: " << appendingDuration;
}

void drawStimulus(string letter, bool showInstructions) {
	ofBackground(backgroundColor);
	ofSetColor(textColor);
	Draw::centeredString(Display.getCenterOfDisplay(), letter, letterFont);

	if (showInstructions) {
		instructionFont.drawString(keyReminderInstructions, 30, Display.getResolution().y - 30);
	}
}

void drawBlank(void) {
	ofBackground(255);
}

void drawFixationSlide (int remainingTime) {
	ofBackground(backgroundColor);
	ofSetColor(textColor);
	stringstream s;
	s << nBack << "-back task" << endl;
	s << keyReminderInstructions << endl;
	s << "Starting in " << remainingTime << " seconds";

	Draw::centeredString(Display.getCenterOfDisplay(), s.str(), letterFont);
}