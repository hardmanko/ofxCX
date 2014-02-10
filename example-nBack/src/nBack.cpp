#include "CX_EntryPoint.h"

/*! \file
This example shows how to implement an N-Back task using an advanced feature of the CX_SlidePresenter 
(SP). There is a feature of the SP that allows you to give it a pointer to a function that will be 
called every time the SP has just presented the final slide that it currently has. In your function,
you can add more slides to the SP, which will allow it to continue presenting slides. If you don't
add any more slides, slide presentation will stop with the currently presented slide.

For this N-Back task, the presentation of stimuli will follow the pattern stimulus-blank-stimulus-blank 
etc. The idea is that you will load up the SP with the first few stimuli and blanks. The SP will be started
and will present the first few stimuli. When it runs out of stimuli, the last slide user function will be 
called. In this function, we will check for any responses that have been made since the last time the 
function was called and draw the next stimulus-blank pair. See the definition of lastSlideFunction and
setupExperiment for the implementation of these ideas.
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

CX_Millis stimulusPresentationDuration = 1000.0;
CX_Millis interStimulusInterval = 1000.0;

CX_SlidePresenter SlidePresenter;
void lastSlideFunction (CX_FinalSlideFunctionInfo_t& info);
void drawStimulusForTrial (unsigned int trial, bool showInstructions);
void generateTrials (int numberOfTrials);

void runExperiment (void) {

	Log.level(CX_LogLevel::LOG_ALL, "CX_SlidePresenter");

	Input.setup(true, false); //Use keyboard, not mouse.

	letterFont.loadFont(OF_TTF_SANS, 26); //The easiest way to pick fonts is to use the constants OF_TTF_SANS,
		//OF_TTF_MONO, or OF_TTF_SERIF, which will load system fonts that satify the stated criterion (sans serif, monospaced, or serif).
	instructionFont.loadFont(OF_TTF_SANS, 12);

	generateTrials(10);

	//Configure the SlidePresenter:
	CX_SP_Configuration config;
	config.display = &Display; //Set the SlidePresenter to use Display for the display.

	//Set a function that you want to be called every time the SlidePresenter has started to present the last
	//slide you put in. In your function, you can add more slides to the SlidePresenter. Every time it reaches 
	//the last slide, it will called lastSlideFunction again.
	config.finalSlideCallback = &lastSlideFunction;

	config.deallocateCompletedSlides = true; //We know that for this experiment we will never want to present the
		//same slide twice, so we set the SlidePresenter to deallocate the memory used for slides that have already been presented.
		//This help to prevent out-of-memory issues with the video card.

	SlidePresenter.setup(config);


	//Start loading slides into the SlidePresenter. Load up a little countdown-to-start screen.
	for (int i = 3; i > 0; i--) {
		SlidePresenter.beginDrawingNextSlide(1000000, "fixation");
		ofBackground(backgroundColor);
		ofSetColor(textColor);
		stringstream s;
		s << nBack << "-back task" << endl;
		s << "Press '" << targetKey << "' for targets and '" << nonTargetKey << "' for non-targets" << endl;
		s << "Starting in " << i;

		Draw::centeredString(Display.getCenterOfDisplay(), s.str(), letterFont);
	}

	//Now load the first nBack + 1 stimuli into the slide presenter.
	for (unsigned int i = 0; i <= nBack; i++) {
		SlidePresenter.beginDrawingNextSlide(stimulusPresentationDuration, "stimulus");
		drawStimulusForTrial(i, (i == nBack)); //The i == nBack thing is just to draw the on screen instructions only for
			//trials on which the participant should respond (no on the first nBack trials, but on the nBack-th trial they should).
		SlidePresenter.endDrawingCurrentSlide();

		SlidePresenter.beginDrawingNextSlide(interStimulusInterval, "blank");
		ofBackground(backgroundColor);
		SlidePresenter.endDrawingCurrentSlide();
	}
	trialNumber = nBack; //This will be the stimulus number that was just presented the first time the user function is called.

	//Once everything is set up, start presenting the slides.
	SlidePresenter.startSlidePresentation();

	while (SlidePresenter.isPresentingSlides()) {
		SlidePresenter.update(); //Make sure that you call the update function of the SlidePresenter, otherwise it does nothing.

		Input.pollEvents(); //You must poll for input at regular intervals in order to get meaningful timing data
			//for responses. The reason for this is that responses are given timestamps in the pollEvents function,
			//so if it does not get called for long periods of time, what will happen is that the responses will
			//still be collected, but the timestamps for the responses will be wrong.
	}

	//When the slide presenter is done presenting slides, that means we are done with this mini-experiment.
	df.printToFile("N-Back output.txt"); //Output the data.

	Display.beginDrawingToBackBuffer();
	ofBackground(backgroundColor);
	Draw::centeredString(Display.getCenterOfDisplay(), "Experiment complete!\nPress any key to exit.", letterFont);
	Display.endDrawingToBackBuffer();
	Display.BLOCKING_swapFrontAndBackBuffers();

	Log.flush();

	//ofSleepMillis(3000); //Wait for three seconds before returning.
	while (!Input.pollEvents())
		;

	//Just past this point, runExperiment will implicitly return and the program will exit.
}

void lastSlideFunction(CX_FinalSlideFunctionInfo_t& info) {

	//At this point in time, the last slide has just been put on screen. The last slide is a blank, which means that the slide before it
	//was a stimulus that should have been responded to. We'll check for keyboard events.
	bool validResponseMade = false;
	if (Input.Keyboard.availableEvents() > 0) {
		//We don't want any responses made before the stimulus was presented, so let's find out when it was presented.
		CX_Slide_t &lastStimulusSlide = SlidePresenter.getSlides().at( info.currentSlideIndex - 1 );
		CX_Micros stimulusOnset = lastStimulusSlide.actual.startTime;

		while (Input.Keyboard.availableEvents() > 0) {
			CX_KeyEvent_t kev = Input.Keyboard.getNextEvent();
			if ((kev.eventTime >= stimulusOnset) && (kev.eventType == CX_KeyEvent_t::PRESSED) && (kev.key == targetKey || kev.key == nonTargetKey)) {

				if (kev.key == targetKey) {
					df(trialNumber, "responseType") = "target";
				} else if (kev.key == nonTargetKey) {
					df(trialNumber, "responseType") = "nonTarget";
				}

				df(trialNumber, "responseLatency") = kev.eventTime - stimulusOnset;

				validResponseMade = true;
				Input.Keyboard.clearEvents(); //Ignore any other responses after the first valid response.
			}
		}
	}

	if (!validResponseMade) {
		df(trialNumber, "responseType") = "noValidResponse";
		df(trialNumber, "responseLatency") = 0;
	}

	if (++trialNumber == trialCount) {
		info.instance->stopSlidePresentation(); //You can explicitly stop presentation using this function. 
			//You can also stop presentation by simply not adding any more slides to the SlidePresenter. 
			//Because it has no more slides to present, it will just stop.

	} else {
		//Draw the next letter and the following blank.
		info.instance->beginDrawingNextSlide(stimulusPresentationDuration, "stimulus");
		drawStimulusForTrial(trialNumber, true);

		info.instance->beginDrawingNextSlide(interStimulusInterval, "blank");
		ofBackground(backgroundColor);
		info.instance->endDrawingCurrentSlide();

		Log.flush(); //For this experiment, this is probably the best time to flush the logs, but it is hard to say. You could simply wait until
		//the experiment is finished or the end of a trial block to flush.
	}

}

void generateTrials(int numberOfTrials) {

	trialCount = numberOfTrials;

	string letterArray[8] = { "A", "F", "H", "L", "M", "P", "R", "Q" };
	vector<string> letters = arrayToVector(letterArray, 8); //Once c++11 is fully implemented, you will be able 
		//to use an initializer list for vectors as well as arrays. Until then, arrayToVector is useful.

	//Draw trialCount deviates from a binomial distribution with 1 trial and 40% probability of a success (i.e. trialCount slightly unfair coin flips).
	//For a real N-Back task, you would probably use a more complicated way of determining the trial sequence.
	vector<int> targetTrial = RNG.binomialDeviates(trialCount, 1, .4);

	//For the first N trials, pick letters randomly
	for (int i = 0; i < nBack; i++) {
		df(i, "letter") = RNG.sample(letters);
	}

	//From N on, pick based on trial type
	for (int i = nBack; i < trialCount; i++) {
		if (targetTrial[i] == 1) {
			df(i, "trialType") = "target";
			df(i, "letter") = df(i - nBack, "letter").toString();
		} else {
			df(i, "trialType") = "nonTarget";
			df(i, "letter") = RNG.randomExclusive(letters, df(i - nBack, "letter").toString());
		}
	}

	//Print out the current state of the data frame to make sure that everything looks normal.
	cout << df.print() << endl;
	cout << endl;
}

void drawStimulusForTrial(unsigned int trial, bool showInstructions) {
	string letter = df(trial, "letter").toString();

	ofBackground(backgroundColor);
	ofSetColor(textColor);
	Draw::centeredString(Display.getCenterOfDisplay(), letter, letterFont);

	if (showInstructions) {
		stringstream s;
		s << "Press '" << targetKey << "' for targets and '" << nonTargetKey << "' for non-targets";
		instructionFont.drawString(s.str(), 30, Display.getResolution().y - 30);
	}
}