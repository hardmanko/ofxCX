#include "CX_EntryPoint.h"

/*! \file
In this example, we are going to consider two different ways of using the slide presenter
to present stimuli. One method is the one used in the change detection example: Draw the
stimuli to framebuffers that are managed by the SlidePresenter. This approach is fairly
easy to do, but it had a time cost, because allocating and copying all of the framebuffers
takes time. This example builds on the nBack example.

In this example, we are going to contrast using the framebuffer approach with the slide
presenter with the use of drawing functions (or to be specific, functors:
http://www.cprogramming.com/tutorial/functors-function-objects-in-c++.html). The
standard framebuffer approach has the following major steps:
1) Allocate the framebuffer
2) Draw your stimuli to the famebuffer
3) Copy the framebuffer to the back buffer
4) Swap front and back buffers
Using drawing functions/functors, we avoid steps 1 and 3, which are generally both
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

Another consideration
*/

CX_DataFrame df;
CX_DataFrame::rowIndex_t trialNumber = 0;
int trialCount = 40;
int lastSlideIndex = 0;
int nBack = 2;

ofTrueTypeFont letterFont;
ofTrueTypeFont instructionFont;

ofColor backgroundColor(50);
ofColor textColor(255);

char targetKey = 'f';
char nonTargetKey = 'j';
string keyReminderInstructions;

CX_Millis stimulusPresentationDuration = 1000;
CX_Millis interStimulusInterval = 1000;

CX_SlidePresenter SlidePresenter;
void finalSlideFunction(CX_SlidePresenter::FinalSlideFunctionArgs& info);
//void drawStimulusForTrial(unsigned int trial, bool showInstructions);

void drawStimuliToFramebuffers(CX_SlidePresenter& sp, int trialIndex);
void appendDrawingFunctions(CX_SlidePresenter& sp, int trialIndex);

void drawStimulus(string letter, bool showInstructions);
void drawBlank(void);
void generateTrials(int numberOfTrials);


bool useFramebuffersForStimuli = false; //If true, the standard framebuffer approach will be used. If 
//false, drawing functions will be used.

//This is a "functor": an object that has data members (letter and showInstructions), but can be called
//as a function using the overloaded operator().
struct stimulusFunctor {
	string letter;
	bool showInstructions;

	void operator()(void) {
		drawStimulus(this->letter, this->showInstructions);
	}
};


void drawStimulus(string letter, bool showInstructions) {
	ofBackground(backgroundColor);
	ofSetColor(textColor);
	Draw::centeredString(Display.getCenterOfDisplay(), letter, letterFont);

	if (showInstructions) {
		instructionFont.drawString(keyReminderInstructions, 30, Display.getResolution().y - 30);
	}
}

void drawBlank(void) {
	ofBackground(backgroundColor);
};

vector<stimulusFunctor> stimulusFunctors;

void runExperiment(void) {

	//Display.setFullScreen(true);

	Input.setup(true, false); //Use keyboard, not mouse.

	letterFont.loadFont(OF_TTF_SANS, 26);
	instructionFont.loadFont(OF_TTF_SANS, 12);

	generateTrials(10);

	CX_SlidePresenter::Configuration config;
	config.display = &Display;
	config.swappingMode = CX_SlidePresenter::Configuration::MULTI_CORE;
	config.finalSlideCallback = &finalSlideFunction;
	config.deallocateCompletedSlides = useFramebuffersForStimuli; //If we aren't using framebuffers, then deallocating the framebuffers is meaningless.

	config.useFenceSync = true;
	config.waitUntilFenceSyncComplete = true;

	SlidePresenter.setup(config);

	stringstream s;
	s << "Press '" << targetKey << "' for targets and '" << nonTargetKey << "' for non-targets";
	keyReminderInstructions = s.str();


	//Start loading slides into the SlidePresenter. Load up a little countdown-to-start screen.
	//We'll always use framebuffers for this because it isn't timing-critical.
	for (int i = 3; i > 0; i--) {
		SlidePresenter.beginDrawingNextSlide(1000, "fixation");
		ofBackground(backgroundColor);
		ofSetColor(textColor);
		stringstream s;
		s << nBack << "-back task" << endl;
		s << keyReminderInstructions << endl;
		s << "Starting in " << i;

		Draw::centeredString(Display.getCenterOfDisplay(), s.str(), letterFont);
	}

	for (unsigned int i = 0; i <= nBack; i++) {
		if (useFramebuffersForStimuli) {
			drawStimuliToFramebuffers(SlidePresenter, i);
		} else {
			appendDrawingFunctions(SlidePresenter, i);
		}
	}
	trialNumber = nBack;

	SlidePresenter.startSlidePresentation();

	while (SlidePresenter.isPresentingSlides()) {
		SlidePresenter.update();
		Input.pollEvents();
	}

	df.printToFile("N-Back output.txt");

	//Calling this function can give us a lot of information about the last presentation of slides.
	Log.notice() << "Slide presentation information: " << endl << SlidePresenter.printLastPresentationInformation();

	//Display.setFullScreen(false);

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

	vector<int> targetTrial = RNG.binomialDeviates(trialCount, 1, .4);

	for (int i = 0; i < nBack; i++) {
		df(i, "letter") = RNG.sample(letters);
	}

	for (int i = nBack; i < trialCount; i++) {
		if (targetTrial[i] == 1) {
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

	CX_Millis startTime = Clock.getTime();

	sp.beginDrawingNextSlide(stimulusPresentationDuration, "stimulus");
	string letter = df(trialIndex, "letter").toString();
	drawStimulus(letter, (trialIndex >= nBack));
	sp.endDrawingCurrentSlide();

	sp.beginDrawingNextSlide(interStimulusInterval, "blank");
	drawBlank();
	sp.endDrawingCurrentSlide();
}

void appendDrawingFunctions(CX_SlidePresenter& sp, int trialIndex) {
	//Because stimulusFunctors contains objects that can be called as functions, you can treat 
	//an instance of the object as though it were a function.
	sp.appendSlideFunction(stimulusFunctors[trialIndex], stimulusPresentationDuration, "stimulus");

	sp.appendSlideFunction(drawBlank, stimulusPresentationDuration, "blank");
}