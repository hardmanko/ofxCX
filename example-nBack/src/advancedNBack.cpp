#include "CX.h"

/* This example is a more advanced version of basicNBack.cpp, so reading that example first
is recommended.

In this example, we are going to consider two different ways of using the slide presenter
to present stimuli. One method is the one used in the basic N-Back example: Draw the
stimuli to framebuffers that are managed by the slide presenter. This approach is fairly
easy to do, but it had a time cost, because allocating and copying all of the framebuffers
takes time.

In this example, we are going to contrast using the standard framebuffer-based approach 
to using the slide presenter with an approach that uses drawing functions with the slide
presenter. The standard framebuffer approach has the following major steps:

1) Allocate the framebuffer (happens in CX_SlidePresenter::beginDrawingNextSlide())
2) Draw your stimuli to the famebuffer (done after the call to CX_SlidePresenter::beginDrawingNextSlide())
3) Draw the framebuffer to the back buffer (happens during slide presentation)
4) Swap front and back buffers (also during slide presentation)

Using drawing functions, we avoid steps 1 and 3. Step 1 can be very costly in terms of time. Step 3
may take longer than just drawing stimuli directly to the back buffer. With drawing functions, 
steps 1 through 3 become "Draw your stimuli to the back buffer directly". For an N-back task, there are no 
indefinitely-long inter-trial pauses where you can prepare stimuli: you have to get the next stimulus 
ready within a fixed interval, so using drawing functions may be the best approach to acheive good timing
precision. We are going to examine that issue with this example.

The way in which you use a drawing function with a CX_SlidePresenter is with the appendSlideFunction()
function. This function takes three arguments: the drawing function, the duration for which the stimuli
drawn by that drawing function should be presented, and the name of the slide (optional). When the
stimuli specified by the drawing function are ready to be presented, the drawing function will be called.
In the drawing function, you do not need to call Disp.beginDrawingToBackBuffer(); the slide presenter
does that for you. See drawStimulus() below for an example of what a drawing function might look like.

We can compare the performance of the framebuffer and drawing function approaches by examining how much time is
used for the two approaches. The drawing function approach should take less time because it does not require
allocating framebuffers. Another consideration is the amount of time it takes to copy a framebuffer to the back 
buffer (step 3 above). This may take longer than drawing a small amount of stimuli directly to the back buffer. 
The framebuffer approach provides a nice security blanket: "I know that regardless of whatever is the the 
framebuffer, it will be copied in the same amount of time as any other framebuffer." However, if that copying 
time is longer than the stimulus drawing times, then using framebuffers is less efficient than drawing directly 
to the back buffer.

We will be using a special kind of function object, known as a functor in C++ 
(http://www.cprogramming.com/tutorial/functors-function-objects-in-c++.html)
to do our drawing. A functor is basically a struct that can be called like a function using operator().
But unlike a normal function, a functor carries data along with it that can be used in the function call.
Thus, a fuctor is a way to have a function without any arguments for which you can still specify those
arguments (in a sense) by setting data members of the functor struct. See struct stimulusFunctor below for an
implementation. This idea is discussed further in the documentation for CX::CX_SlidePresenter::appendSlideFunction(),
with a code example.
*/

//If true, the standard framebuffer approach will be used. If false, drawing functions will be used.
//Try with both settings and look at the output in the console (or the log file in data/logfiles).
bool useFramebuffersForStimuli = false; 

CX_DataFrame df;
CX_DataFrame::rowIndex_t trialNumber = 0;
int trialCount = 10;
int nBack = 2;

ofColor backgroundColor(50);
ofColor textColor(255);

ofTrueTypeFont bigFont;
ofTrueTypeFont smallFont;

char targetKey = 'F';
char nonTargetKey = 'J';
string keyReminderInstructions;

CX_Millis stimulusPresentationDuration = 1000;
CX_Millis interStimulusInterval = 1000;

CX_SlidePresenter SlidePresenter;
void finalSlideFunction(CX_SlidePresenter::FinalSlideFunctionArgs& info);

//Here are two new functions that draw the stimuli in different ways
void drawStimuliToFramebuffers(CX_SlidePresenter& sp, int trialIndex);
void appendDrawingFunctions(CX_SlidePresenter& sp, int trialIndex);

void drawStimulus(string letter, bool showInstructions);
void drawBlank(void);
void drawFixationSlide(int remainingTime);

void generateTrials(void);



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

	//When checking the timing of things, make sure to try it in full screen mode as well, because
	//timing errors happen a lot more when experiments is windowed rather than full screen.
	Disp.setFullscreen(false);

	if (Disp.isFullscreen()) {
		//If going to full screen, sometimes its good to wait a little while for things to settle before drawing stuff.
		Clock.sleep(CX_Seconds(1));
	}

	Log.levelForFile(CX_Logger::Level::LOG_ALL, "Last run.txt");
	Log.level(CX_Logger::Level::LOG_ALL, "CX_SlidePresenter");

	Log.notice() << "Frame period: " << Disp.getFramePeriod() << " (" << Disp.getFramePeriodStandardDeviation() << ")";
	
	Input.setup(true, false); //Use keyboard, not mouse.

	bigFont.loadFont(OF_TTF_SANS, 26);
	smallFont.loadFont(OF_TTF_SANS, 12);

	stringstream s;
	s << "Press '" << targetKey << "' for targets and '" << nonTargetKey << "' for non-targets";
	keyReminderInstructions = s.str();

	generateTrials();

	//In this example, more of the configuration settings for the slide presenter are used.
	//See the documentation for CX_SlidePresenter::Configuration for more information.
	CX_SlidePresenter::Configuration config;
	config.display = &Disp;
	config.swappingMode = CX_SlidePresenter::SwappingMode::SINGLE_CORE_BLOCKING_SWAPS;
	config.finalSlideCallback = &finalSlideFunction;
	config.deallocateCompletedSlides = useFramebuffersForStimuli; //Only deallocate if using framebuffers

	config.preSwapCPUHoggingDuration = CX_Millis(3);
	config.useFenceSync = true;
	config.waitUntilFenceSyncComplete = false;

	SlidePresenter.setup(config);

	for (int i = 3; i > 0; i--) {
		if (useFramebuffersForStimuli) {
			//Note that regardless of whether drawing functions are used later, you can use the standard framebuffer approach as well.
			//To be clear: You can mix and match framebuffers with drawing functions in a single presentation of slides.
			SlidePresenter.beginDrawingNextSlide(1000, "fixation");
			drawFixationSlide(i);
		} else {
			//std::bind sort of "bakes in" the value of i to drawFixationSlide, and the resulting function
			//takes no arguments, so it can be given to the slide presenter as a drawing function.
			SlidePresenter.appendSlideFunction( std::bind(drawFixationSlide, i), 1000, "fixation" );
		}
	}

	for (unsigned int i = 0; i <= nBack; i++) {
		//Depending on whether using the framebuffer approach or the drawing function approach, call a different setup function.
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

	Log.notice() << "Slide presentation information: " << endl << SlidePresenter.printLastPresentationInformation();


	vector<CX_SlidePresenter::Slide>& slides = SlidePresenter.getSlides();
	CX_Millis startMinusCopySum = 0;
	for (int i = 0; i < slides.size(); i++) {
		startMinusCopySum += slides[i].actual.startTime - slides[i].copyToBackBufferCompleteTime;
	}
	Log.notice() << "Average difference between back buffer copy completion and slide start: " << startMinusCopySum / slides.size();
	
	if (Disp.isFullscreen()) {
		Disp.setFullscreen(false);
	}

	if (Disp.isAutomaticallySwapping()) {
		Disp.setAutomaticSwapping(false);
	}

	Disp.beginDrawingToBackBuffer();
	ofBackground(backgroundColor);
	Draw::centeredString(Disp.getCenter(), "Experiment complete!\nPress any key to exit.", bigFont);
	Disp.endDrawingToBackBuffer();
	Disp.swapBuffers();

	Log.flush();

	Input.Keyboard.waitForKeypress(-1);
}

void finalSlideFunction(CX_SlidePresenter::FinalSlideFunctionArgs& info) {

	bool validResponseMade = false;
	if (Input.Keyboard.availableEvents() > 0) {
		CX_SlidePresenter::Slide &lastStimulusSlide = SlidePresenter.getSlides().at(info.currentSlideIndex - 1);
		CX_Millis stimulusOnset = lastStimulusSlide.actual.startTime;

		while (Input.Keyboard.availableEvents() > 0) {
			CX_Keyboard::Event kev = Input.Keyboard.getNextEvent();
			if ((kev.time >= stimulusOnset) && (kev.type == CX_Keyboard::PRESSED) && (kev.key == targetKey || kev.key == nonTargetKey)) {

				if (kev.key == targetKey) {
					df(trialNumber, "responseType") = "target";
				} else if (kev.key == nonTargetKey) {
					df(trialNumber, "responseType") = "nonTarget";
				}

				df(trialNumber, "responseLatency") = kev.time - stimulusOnset;

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

void generateTrials(void) {

	string letterArray[8] = { "A", "F", "H", "L", "M", "P", "R", "Q" };
	vector<string> letters = Util::arrayToVector(letterArray, 8);

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
			df(i, "letter") = RNG.sampleExclusive(letters, df(i - nBack, "letter").toString());
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
	//an instance of the object as though it were a function. Yes, this is magical.
	sp.appendSlideFunction(stimulusFunctors[trialIndex], stimulusPresentationDuration, "stimulus");

	//You can also accomplish the same thing using std::bind() and the drawStimulus function directly.
	//This makes it so you don't need to set up the functors.
	//sp.appendSlideFunction( std::bind( drawStimulus, df(trialIndex, "letter").toString(), (trialIndex >= nBack) ), stimulusPresentationDuration, "stimulus" );

	sp.appendSlideFunction(drawBlank, interStimulusInterval, "blank");

	CX_Millis appendingDuration = Clock.now() - startTime;
	Log.notice() << "Drawing function appending duration: " << appendingDuration;
}

void drawStimulus(string letter, bool showInstructions) {
	ofBackground(backgroundColor);
	ofSetColor(textColor);
	Draw::centeredString(Disp.getCenter(), letter, bigFont);

	if (showInstructions) {
		smallFont.drawString(keyReminderInstructions, 30, Disp.getResolution().y - 30);
	}
}

void drawBlank(void) {
	ofBackground(backgroundColor);
}

void drawFixationSlide (int remainingTime) {
	ofBackground(backgroundColor);
	ofSetColor(textColor);
	stringstream s;
	s << nBack << "-back task" << endl;
	s << keyReminderInstructions << endl;
	s << "Starting in " << remainingTime << " seconds";

	Draw::centeredString(Disp.getCenter(), s.str(), bigFont);
}