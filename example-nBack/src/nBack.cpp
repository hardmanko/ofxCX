#include "CX_EntryPoint.h"

#include "CX_ContinuousSlidePresenter.h"

CX_ContinuousSlidePresenter csp;

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

void lastSlideFunction (CX_CSPInfo_t& info);
void drawStimulusForTrial (unsigned int trial, bool showInstructions);
void generateTrials (int numberOfTrials);

void setupExperiment (void) {


	Input.setup(true, false);

	letterFont.loadFont(OF_TTF_SANS, 20);
	instructionFont.loadFont(OF_TTF_SANS, 12);

	generateTrials(10);

	csp.setDisplay(&Display); //This should definitely be called "setup"

	//Set a function that you want to be called every time the slide presenter has started to present the last
	//slide you put in. In your function, you can add more slides to the slide presenter. Every time it reaches 
	//the last slide, it will called lastSlideFunction again.
	csp.setUserFunction(&lastSlideFunction);

	//Start loading slides into the slide presenter. Load up a little countdown screen.
	for (int i = 3; i > 0; i--) {
		csp.beginDrawingNextSlide(1000000, "fixation");
		ofBackground(backgroundColor);
		ofSetColor(textColor);
		stringstream s;
		s << nBack << "-back task" << endl;
		s << "Press '" << targetKey << "' for targets and '" << nonTargetKey << "' for non-targets" << endl;
		s << "Starting in " << i;

		drawCenteredString(s.str(), letterFont, Display.getCenterOfDisplay() );
	}

	//Now load the first nBack + 1 stimuli into the slide presenter. The participant does not need to respond to these stimuli, until the nBack + 1th stimulus.
	for (unsigned int i = 0; i <= nBack; i++) {
		csp.beginDrawingNextSlide(stimulusPresentationDuration, "stimulus");
		drawStimulusForTrial(i, (i == nBack));
		csp.endDrawingCurrentSlide();

		csp.beginDrawingNextSlide(interStimulusInterval, "blank");
		ofBackground(backgroundColor); //blank
		csp.endDrawingCurrentSlide();
	}
	trialNumber = nBack; //This will be the stimulus number that was just presented the first time the user function is called.

	csp.startSlidePresentation();
}

void updateExperiment (void) {
	csp.update();
}


void generateTrials (int numberOfTrials) {

	trialCount = numberOfTrials;

	string letterArray [8] = { "A", "F", "H", "L", "M", "P", "R", "Q" };
	vector<string> letters = arrayToVector(letterArray, 8); //Once c++11 is fully implemented, you will be able 
		//to use an initializer list for vectors as well. Until then, arrayToVector is useful.

	//Draw trialCount deviates from a binomial distribution with 1 trial and 40% probability of a success (i.e. trialCount slightly unfair coin flips).
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

	cout << df.print() << endl;
	cout << endl;
}



void drawStimulusForTrial (unsigned int trial, bool showInstructions) {
	string letter = df(trial, "letter").toString();

	ofBackground(backgroundColor);
	ofSetColor( textColor );
	CX::drawCenteredString( letter, letterFont, Display.getCenterOfDisplay() );

	if (showInstructions) {
		stringstream s;
		s << "Press '" << targetKey << "' for targets and '" << nonTargetKey << "' for non-targets";
		instructionFont.drawString( s.str(), 30, Display.getResolution().y - 30 );
	}
}



void lastSlideFunction (CX_CSPInfo_t& info) {
	Log.flush();

	//At this point in time, the last slide has just been put on screen. The last slide is a blank, which means that the slide before it
	//was a stimulus that should have been responded to. We'll check for keyboard events.
	bool validResponseMade = false;
	if (Input.Keyboard.availableEvents() > 0) {
		//We don't want any responses made before the stimulus was presented, so let's find out when it was presented.
		CX_Slide_t &lastStimulusSlide = csp.getSlide( info.currentSlideIndex - 1 );
		CX_Micros_t stimulusOnset = lastStimulusSlide.actualSlideOnset;

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
			}
		}
	}

	if (!validResponseMade) {
		df(trialNumber, "responseType") = "noValidResponse";
		df(trialNumber, "responseLatency") = 0;
	}

	//Draw the next letter and the following blank.
	info.instance->beginDrawingNextSlide(stimulusPresentationDuration, "stimulus");
	drawStimulusForTrial(trialNumber, true);

	info.instance->beginDrawingNextSlide(interStimulusInterval, "blank");
	ofBackground(backgroundColor);
	info.instance->endDrawingCurrentSlide();

	if (++trialNumber == trialCount) {
		info.userStatus = CX::CX_CSPInfo_t::STOP_NOW;
		df.printToFile("N-Back output.txt");

		Display.beginDrawingToBackBuffer();
		ofBackground(backgroundColor);
		CX::drawCenteredString( "Experiment complete!", letterFont, Display.getCenterOfDisplay() );

		Display.endDrawingToBackBuffer();
		Display.BLOCKING_swapFrontAndBackBuffers();

		ofSleepMillis(3000);
		ofExit();
	} else {
		info.userStatus = CX::CX_CSPInfo_t::CONTINUE_PRESENTATION;
	}

}


