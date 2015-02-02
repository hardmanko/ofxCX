#include "CX.h"

/* This task is an implementation of an N-back task. In an N-Back task, participants must report whether
the current stimulus is the same as the stimulus that was presented N stimuli back. Assume that in
a 2-back task, the stimulus sequence is the letters (presented one at a time):

ABCACBBAB

The second C and the last B would both be targets that are the same as the letter 2 letters back.
None of the other letters are targets. In an N-Back task, the number of stimuli can be very high
and the stimulus sequence may need to change based on participant responses, so they can be difficult
to design.

This example shows how to implement an N-Back task using an advanced feature of the CX_SlidePresenter.
There is a feature of the slide presenter that allows you to give it a pointer to a function that will be 
called every time the slide presenter has just presented the final slide that it currently has. In the
supplied function, you can add more slides to the slide presenter, which will allow it to continue 
presenting slides. If you don't add any more slides, slide presentation will stop with the currently 
presented slide. 

This is useful for an N-Back task because as a trial progresses, you might need to present more 
stimuli in a response-dependent way, so you can't just set up a CX_SlidePresenter with a large 
number of stimuli to present. In addition, you might not have enough video memory to pre-render 
all of the stimuli. When using a CX_SlidePresenter in the standard way, everything is pre-rendered
to framebuffers, which takes up a lot of memory even if what is rendered is very simple. For this
example, the number of trials is very small, so running out of video memory should not be possible,
but sometimes N-Back tasks have a large number of trials. In this experiment, we will set up the
CX_SlidePresenter to automatically deallocate memory for slides that have been presented, which
means that it should be possible to have arbitrarily long stimulus sets without having problems.

For this N-Back task, the presentation of stimuli will follow the pattern stimulus-blank-stimulus-blank 
etc. The idea is that you will load up the slide presenter with the first few stimuli and blanks. The 
slide presenter will be started and will present the first few stimuli. When it runs out of stimuli, 
the last slide user function will be called. In this function, we will check for any responses that 
have been made since the last time the function was called and draw the next stimulus-blank pair. 
See the definition of finalSlideFunction() and setupExperiment() in this file for the implementation 
of these ideas.

Once you understand this example, please also see the advancedNBack example. It demonstrates some other 
advanced options of the CX_SlidePresenter that should make it possible to improve the timing reliability
of an N-Back (or similar) task.
*/

//Set up some variables that will be used throughout
CX_DataFrame df; //All task data will be stored in this
CX_DataFrame::rowIndex_t trialNumber = 0; //Tracks the current trial number for storing and retrieving data
int trialCount = 10; //The total number of trials
int nBack = 2; //Targets will be nBack trials back from the current stimulus, so this will be a 2-back task

ofColor backgroundColor(50);
ofColor textColor(255);

//Use two different sizes of font
ofTrueTypeFont bigFont;
ofTrueTypeFont smallFont;

//Define the response keys
char targetKey = 'F';
char nonTargetKey = 'J';
string keyReminderInstructions;

//Stimulus and blank timings
CX_Millis stimulusPresentationDuration = 1000;
CX_Millis interStimulusInterval = 1000;

//The slide presenter that will be used for stimulus presentation
CX_SlidePresenter SlidePresenter;

//Function declarations
void finalSlideFunction (CX_SlidePresenter::FinalSlideFunctionArgs& info);
void drawStimulusForTrial (unsigned int trial, bool showInstructions);
void generateTrials (void);



void runExperiment (void) {

	Input.setup(true, false); //Use keyboard, not mouse.

	bigFont.loadFont(OF_TTF_SANS, 26); //The easiest way to pick fonts is to use the constants OF_TTF_SANS,
		//OF_TTF_MONO, or OF_TTF_SERIF, which will load system fonts that satify the stated criterion (sans serif, monospaced, or serif).
	smallFont.loadFont(OF_TTF_SANS, 12);

	generateTrials();

	//Configure the SlidePresenter using advanced configuration options in the CX_SlidePresenter::Configuration struct:
	CX_SlidePresenter::Configuration config;
	config.display = &Disp; //Set the SlidePresenter to use Disp for the display.

	//Set a function that you want to be called every time the SlidePresenter has started to present the last
	//slide you put in. In your function, you can add more slides to the SlidePresenter. Every time it reaches 
	//the last slide, it will called finalSlideFunction again.
	config.finalSlideCallback = &finalSlideFunction;

	config.deallocateCompletedSlides = true; //We know that for this experiment we will never want to present the
		//same slide twice, so we set the SlidePresenter to deallocate the memory used for slides that have already been presented.
		//This can help to prevent out-of-memory issues with the video card.

	//Give the configuration struct to the slide presenter to configure it.
	SlidePresenter.setup(config);

	//Make an instruction string
	stringstream s;
	s << "Press '" << targetKey << "' for targets and '" << nonTargetKey << "' for non-targets";
	keyReminderInstructions = s.str();


	//Start loading slides into the SlidePresenter. Load up a little countdown-to-start screen.
	for (int i = 3; i > 0; i--) {
		SlidePresenter.beginDrawingNextSlide(1000, "fixation");
		ofBackground(backgroundColor);
		ofSetColor(textColor);
		stringstream s;
		s << nBack << "-back task" << endl;
		s << keyReminderInstructions << endl;
		s << "Starting in " << i;

		Draw::centeredString(Disp.getCenter(), s.str(), bigFont);
	}

	//Now load the first nBack + 1 stimuli into the slide presenter.
	for (unsigned int i = 0; i <= nBack; i++) {
		SlidePresenter.beginDrawingNextSlide(stimulusPresentationDuration, "stimulus");
		drawStimulusForTrial(i, (i == nBack)); //The i == nBack thing is just to draw the on screen instructions only for
			//trials on which the participant should respond (not on the first nBack trials, but on the nBack-th trial they should).
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
	//Note that the preceding while loop is essentially equivalent to calling SlidePresenter.presentSlides().

	//When the slide presenter is done presenting slides, that means we are done with this mini-experiment.
	df.printToFile("N-Back output.txt"); //Output the data.

	//Calling this function can give us a lot of information about the last presentation of slides.
	Log.notice() << "Slide presentation information: " << endl << SlidePresenter.printLastPresentationInformation();

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

	Log.flush(); //For this experiment, this is probably the best time to flush the logs, but it is hard to say. 
		//You could do it in each interstimulus blank, but there is more potential for timing problems there.

	Input.Keyboard.waitForKeypress(-1);

	//Just past this point, runExperiment will implicitly return and the program will exit.
}

//This is the function that is given to the slide presenter to call every time it runs out of
//slides to present.
void finalSlideFunction(CX_SlidePresenter::FinalSlideFunctionArgs& info) {

	//At this point in time, the last slide has just been put on screen. The last slide is a blank, which means that the slide before it
	//was a stimulus that should have been responded to. We'll check for keyboard events.
	bool validResponseMade = false;
	if (Input.Keyboard.availableEvents() > 0) {
		//We don't want any responses made before the stimulus was presented, so let's find out when it was presented.
		CX_SlidePresenter::Slide &lastStimulusSlide = SlidePresenter.getSlides().at( info.currentSlideIndex - 1 );
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
			//If it has no more slides to present, it will just stop.

	} else {
		//Draw the next letter and the following blank.
		info.instance->beginDrawingNextSlide(stimulusPresentationDuration, "stimulus");
		drawStimulusForTrial(trialNumber, true);

		info.instance->beginDrawingNextSlide(interStimulusInterval, "blank");
		ofBackground(backgroundColor);
		info.instance->endDrawingCurrentSlide();
	}
}

void generateTrials(void) {

	//Set up the possible stimulus letters and convert them into a vector.
	string letterArray[8] = { "A", "F", "H", "L", "M", "P", "R", "Q" };
	vector<string> letters = Util::arrayToVector(letterArray, 8);

	//Draw trialCount deviates from a Bernoulli distribution with 40% probability of a success (i.e. trialCount slightly unfair coin flips).
	//For a real N-Back task, you would probably use a more complicated way of determining the trial sequence.
	vector<bool> targetTrial = RNG.sampleRealizations(trialCount, std::bernoulli_distribution(.4));

	//For the first N trials, pick letters randomly
	for (int i = 0; i < nBack; i++) {
		df(i, "letter") = RNG.sample(letters);
	}

	//From N on, pick based on trial type
	for (int i = nBack; i < trialCount; i++) {
		if (targetTrial[i]) {
			df(i, "trialType") = "target";
			df(i, "letter") = df(i - nBack, "letter").toString();
		} else {
			df(i, "trialType") = "nonTarget";
			df(i, "letter") = RNG.sampleExclusive(letters, df(i - nBack, "letter").toString());
		}
	}

	//Print out the current state of the data frame to make sure that everything looks normal.
	cout << df.print() << endl;
	cout << endl;
}

//This function draws a single stimulus based on the trial number and whether to show response instructions
void drawStimulusForTrial(unsigned int trial, bool showInstructions) {
	string letter = df(trial, "letter").toString();

	ofBackground(backgroundColor);
	ofSetColor(textColor);
	Draw::centeredString(Disp.getCenter(), letter, bigFont);

	if (showInstructions) {
		smallFont.drawString(keyReminderInstructions, 30, Disp.getResolution().y - 30);
	}
}