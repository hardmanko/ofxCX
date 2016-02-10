#include "CX.h"

/*
This example shows how to do a simple change-detection experiment using CX.
The stimuli are colored circles which are presented in a 3X3 matrix.

Press the S key to indicate that you think the test array is the same or the D
key to indicate that you think the test array is different.
*/


//This structure stores information about the trials in the experiment.
struct TrialData_t {
	int arraySize;
	vector<ofColor> colors;
	vector<ofPoint> locations;

	bool changeTrial;
	int changedObjectIndex;
	ofColor newColor;

	CX_Millis responseLatency;
	bool responseCorrect;
};

//Function declarations (could be in a header file, if preferred).
vector<TrialData_t> generateTrials (int trialCount);
void outputData (void);

void drawStimuli(void);
void presentStimuli(void);
void getResponse(void);

void drawFixation (void);
void drawBlank (void);
void drawSampleArray (const TrialData_t &tr);
void drawTestArray (const TrialData_t &tr);

//Global variables (at least from the perspective of the experiment)
CX_SlidePresenter SlidePresenter;

vector<TrialData_t> trials;
int trialIndex = 0;

int circleRadius = 30;
ofColor backgroundColor(50);


void runExperiment (void) {
	trials = generateTrials(8); //Generate 8 trials (see the definition of generateTrials in this file for how the trials are generated).

	SlidePresenter.setup(&Disp); //Associate Disp with the SlidePresenter so that the SlidePresenter has something to draw to.

	Input.setup(true, false); //Use the keyboard for this experiment, but not the mouse.

	Log.notice() << "Instructions: Press \'s\' for same, \'d\' for different. Press escape to quit.";
	Log.flush();

	//trialIndex is a global variable, so its value is just being set here.
	for (trialIndex = 0; trialIndex < trials.size(); trialIndex++) {
		drawStimuli();
		presentStimuli();
		getResponse();

		//The end of a trial is a good time to flush() the logs, to see if any warnings/errors have happened during the trial.
		//See example-logging in the ofxCX folder for an example of how the logging system works.
		Log.flush();
	}

	outputData();

	Log.notice() << "Experiment complete: exiting...";
	Log.flush();
	Clock.sleep(3000);
}

void drawStimuli(void) {
	//The CX_SlidePresenter is an abstraction that is responsible for displaying visual stimuli for specified durations.
	//One called SlidePresenter is instanstiated for you, but you can create more if you want.
	SlidePresenter.clearSlides(); //Start by clearing all slides (from the last trial).

	//To draw to a slide, call beginDrawingNextSlide() with the name of the slide and the duration
	//that you want the contents of the slide to be presented for.
	SlidePresenter.beginDrawingNextSlide(1000, "fixation");
	//After calling beginDrawingNextSlide(), all drawing commands will be directed to the current
	//slide until beginDrawingNextSlide() is called again or endDrawingCurrentSlide() is called.
	drawFixation(); //See the definition of this function below for some examples of how to draw stuff.

	//Add some more slides.
	SlidePresenter.beginDrawingNextSlide(250, "blank");
	drawBlank();

	SlidePresenter.beginDrawingNextSlide(500, "sample");
	drawSampleArray(trials.at(trialIndex));

	SlidePresenter.beginDrawingNextSlide(1000, "maintenance");
	drawBlank();

	//The duration given for the last slide must be > 0, but is otherwise ignored.
	//The last slide has an infinite duration: Once it is presented, it will stay
	//on screen until something else is drawn (i.e. the slide presenter does not
	//remove it from the screen after its duration is complete).
	SlidePresenter.beginDrawingNextSlide(1, "test");
	drawTestArray(trials.at(trialIndex));
	SlidePresenter.endDrawingCurrentSlide(); //After drawing the last slide, it is good form to call endDrawingCurrentSlide().
}

void presentStimuli(void) {

	SlidePresenter.startSlidePresentation(); //Once all of the slides are ready to go for the next trial,
	//call startSlidePresentation() to do just that. The drawn slides will be drawn on the screen for
	//the specified duration.

	//Check that the slide presenter is still at work (i.e. not yet on the last slide).
	//As soon as the last slide is presented, isPresentingSlides() will return false.
	while (SlidePresenter.isPresentingSlides()) {
		SlidePresenter.update(); //The whole time that stimuli are being presented, the update function of the
		//SlidePresenter should be called. This will allow it to update its state and present the next stimulus, etc.
	}

	Input.pollEvents(); //Check for any events that were made during encoding and maintenance.
	Input.Keyboard.clearEvents(); //Then clear all keyboard responses, if any, that were made,
		//because we are not interested in responses made before the test array was presented.
}

void getResponse(void) {
	while (true) { //Loop continuously,
		Input.pollEvents(); //and check to see if any input has been given.

		while (Input.Keyboard.availableEvents() > 0) { //While there are available events, 
			CX_Keyboard::Event keyEvent = Input.Keyboard.getNextEvent(); //get the next event for processing.

			//Only examine key presses (as opposed to key releases or repeats). Everything would probably work 
			//just fine without this check for this experiment, but it is generally a good idea to filter your input.
			if (keyEvent.type == CX_Keyboard::PRESSED) {

				//Ignore all responses that are not s or d. For a lot of keys, you can compare 
				//CX_Keyboard::Event.key to a character literal for many keys.
				if (keyEvent.key == 'S' || keyEvent.key == 'D') {

					//Figure out the response time. CX does no automatic response time calculation. You have
					//to find out when the stimulus that the participant is responding to was presented. In
					//this case, that is easy to do because the SlidePresenter tracks that information for us.
					//The last slide (given by getSlides().back()) has the slide presentation time stored in
					//the actualSlideOnset member.
					CX_Micros testArrayOnset = SlidePresenter.getSlideByName("test").actual.startTime;
					//One you have the onset time of the test array, you can subtract that from the time
					//of the response, giving the "response time" (better known as response latency).
					trials.at(trialIndex).responseLatency = keyEvent.time - testArrayOnset;

					bool changeTrial = trials.at(trialIndex).changeTrial;

					//Code the response.
					if ((changeTrial && keyEvent.key == 'D') || (!changeTrial && keyEvent.key == 'S')) {
						trials.at(trialIndex).responseCorrect = true;
						Log.notice() << "Response correct!";
					} else {
						trials.at(trialIndex).responseCorrect = false;
						Log.notice() << "Response incorrect.";
					}

					//Now that we have a valid response, clear any other responses and return from this function.
					Input.Keyboard.clearEvents();
					return;
				}
			}
		}
	}
}


vector<TrialData_t> generateTrials (int trialCount) {

	vector<ofColor> objectColors;
	vector<ofPoint> objectLocations;

	//Set up a vector of colors that will be sampled to make the objects.
	objectColors.push_back(ofColor::red);
	objectColors.push_back(ofColor::orange);
	objectColors.push_back(ofColor::yellow);
	objectColors.push_back(ofColor::green);
	objectColors.push_back(ofColor::blue);
	objectColors.push_back(ofColor::purple);

	//Make a 3x3 grid of object locations around the center of the screen.
	ofPoint screenCenter(Disp.getResolution().x / 2, Disp.getResolution().y / 2);
	for (int i = 0; i < 9; i++) {
		int col = i % 3;
		int row = i / 3;

		ofPoint p;
		p.x = screenCenter.x - 100 + (row * 100);
		p.y = screenCenter.y - 100 + (col * 100);

		objectLocations.push_back(p);
	}

	vector<bool> changeTrial = RNG.sample(trialCount, Util::concatenate<bool>(false, true), true);

	vector<TrialData_t> _trials;

	for (int trial = 0; trial < trialCount; trial++) {

		TrialData_t tr;
		tr.arraySize = 4;

		//This randomly picks tr.arraySize colors from objectColors without replacement.
		//RNG is the global Random Number Generator object that is useful for a wide variety of randomization stuff.
		tr.colors = RNG.sample(tr.arraySize, objectColors, false);
		
		tr.locations = RNG.sample(tr.arraySize, objectLocations, false);

		tr.changeTrial = changeTrial[trial];

		if (tr.changeTrial) {

			//If something is going to change, we have to pick which object will change.
			//RNG.randomInt() returns an unsigned int from the given range (including both endpoints).
			tr.changedObjectIndex = RNG.randomInt(0, tr.arraySize - 1);

			//For the new color, sample 1 color from objectColors, excluding any of 
			//the colors already selected for the trial (the 1 is implicit).
			tr.newColor = RNG.sampleExclusive(objectColors, tr.colors); 
		} else {
			//You don't have to set the newColor or the changedObjectIndex for a no-change trial because there is no change and they won't be used.
			tr.changedObjectIndex = -1; //But we'll set them anyway to unreasonable values just to make sure they aren't mistaken for real values.
			tr.newColor = backgroundColor;
		}
		
		_trials.push_back( tr );
	}

	//This version of shuffleVector() (which takes the address of a vector) shuffles the argument, returning nothing.
	RNG.shuffleVector( &_trials );

	return _trials;
}

/*
Drawing stuff in CX uses built in oF drawing functions and some functions from
the CX::Draw namespace. This section gives some examples of such functions, 
although there are many more, including 3D drawing stuff. See the renderingTest
example for more examples.
*/
void drawFixation (void) {
	ofBackground( backgroundColor );

	ofSetColor(ofColor(255));
	Draw::fixationCross(Disp.getCenter(), 30, 5);
}

void drawBlank (void) {
	ofBackground( backgroundColor );
}

void drawSampleArray (const TrialData_t &tr) {
	ofBackground( backgroundColor );

	for (int i = 0; i < tr.colors.size(); i++) {
		ofSetColor( tr.colors.at(i) );
		ofCircle( tr.locations.at(i), circleRadius );
	}
}

void drawTestArray (const TrialData_t &tr) {
	vector<ofColor> testColors = tr.colors; //Make a copy because we want to modify the colors a little
	
	if (tr.changeTrial) {
		testColors.at( tr.changedObjectIndex ) = tr.newColor;
	}
	
	ofBackground( backgroundColor );

	for (int i = 0; i < testColors.size(); i++) {
		ofSetColor( testColors.at(i) );
		ofCircle( tr.locations.at(i), circleRadius );
	}
}

/*
This function is here to show how sucky outputting data is when you have a user-defined struct to store data
because of the lack of reflection in c++. Then, a possibly better solution is suggested. The main problems are:

1) Making errors because the column names must line up with the data, but are output at different places (one block
of code outputs the headers, another block outputs the data), so it is really easy to make an error in the naming of
columns (swapping names).
2) Leaving out a piece of data just because it was forgotten. This is easy to do if you have 20+ columns of data,
some of which are not very important to the main point of the experiment, but might be useful for secondary analyses.
Even for an experiment as simple as this example, making sure that nothing was missing took me a little time.
3) It is a waste of time to manually write data outputting functions, because it is a trivial problem yet must be done carefully.
You have to make sure that you always include the delimiter between items, that line endings are in the right place, etc. This
is all a waste of time.

There is an example called advancedChangeDetection.cpp than shows how using a CX_DataFrame allows you to totally
bypass all of these problems and output all of the data stored in the data frame with one trivial function call
(e.g. myDataFrame.printToFile("filename.txt")).
*/
void outputData(void) {
	string t = "\t";
	stringstream out;

	//Set up the headers
	out << "arraySize" << t << "changedObjectIndex" << t << "changeTrial" << t << "responseCorrect" << t <<
		"respTime" << t << "newColor";
	out << t << "colors" << t << "locations" << endl;

	for (vector<TrialData_t>::iterator it = trials.begin(); it != trials.end(); it++) {
		out << it->arraySize << t << it->changedObjectIndex << t << it->changeTrial << t << it->responseCorrect << t <<
			it->responseLatency << t << it->newColor;

		//Enclose the vectors in quotes so that they are not split on a delimiter when reading into a spreadsheet.
		out << "\"";
		for (int i = 0; i < it->colors.size(); i++) {
			out << it->colors[i];
			if (i < it->colors.size() - 1) {
				out << ";";
			}
		}
		out << "\"";

		//CX::vectorToString() does the above ^^^, more or less. vectorToString makes manually outputting data
		//better, but still loses out to the convenience of a data frame.
		out << t << "\"" << Util::vectorToString(it->locations, ";") << "\"";
		out << endl;
	}

	Util::writeToFile("basic change detection data.txt", out.str(), false); //This file can be found in the data directory of the project,
		//i.e. %openFrameworks directory%/apps/myApps/%your_app_name%/bin/data. You can also specify an absolute path
		//and the file should end up there as long the location does not require write permissions.
}