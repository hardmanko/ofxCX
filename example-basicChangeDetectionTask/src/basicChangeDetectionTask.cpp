#include "CX_EntryPoint.h"

/*
This example shows how to do a simple change-detection experiment using CX.
The stimuli are colored circles which are presented in a 3X3 matrix.
*/


//This structure stores information about the trials in the experiment.
struct TrialData_t {
	int arraySize;
	vector<ofColor> colors;
	vector<ofPoint> locations;

	bool changeTrial;
	int changedObjectIndex;
	ofColor newColor;

	//CX_KeyEvent_t response;
	int64_t responseTime;
	bool responseCorrect;
};

//Function declarations (could be in a header file, if preferred).
vector<TrialData_t> generateTrials (int trialCount);
void outputData (void);

void drawFixation (void);
void drawBlank (void);
void drawSampleArray (const TrialData_t &tr);
void drawTestArray (const TrialData_t &tr);

//Global variables (at least from the perspective of the experiment)
vector<TrialData_t> trials;
int trialIndex = 0;

int circleRadius = 30;
ofColor backgroundColor(50);

string trialPhase = "drawStimuli";


void setupExperiment (void) {
	trials = generateTrials(8); //Generate 8 trials (see the definition of generateTrials in this file for how it does that)

	Input.setup(true, false); //Use the keyboard for this experiment, but not the mouse.

	cout << "Insturctions: Press \'s\' for same, \'d\' for different. Press escape to quit." << endl;
}

/*
updateExperiment is where most of the experiment takes place. It is critical that
the code you put into updateExperiment does not block (i.e. you cannot call functions
like ofSleepMillis() that prevent program execution for a long amount of time). This is
possibly the largest downside to CX, which forces program flow to be nonlinear.
In most psychology experiment software, program flow is linear, which is to say
that you can do, e.g.:

drawThing1();
sleep(1000000); //Sleep for 1 sec (argument in microseconds)
drawThing2();
waitForResponse();

In CX, you can't just sleep whenever you want because the back end code needs to work,
which can only happen if your code returns from updateExperiment quickly. In CX you have
to do something more like this (within updateExperiment):

void updateExperiment (void) {
	if (phase == "draw thing1") {
		drawThing1();

		presentThing2Time = Clock.getTime() + 1000000; //Present thing2 1000000 microseconds from now.
		phase = "draw thing2";
	}
	if (phase == "draw thing2") {
		if (Clock.getTime() <= presentThing2Time) {
			drawThing2();
		}
		phase == "get response";
	}
	if (phase == "get response") {
		checkForResponse();
		phase = "draw thing1";
	}
}

There is an abstraction which reduces the pain associated with this design pattern, called CX_TrialController.
It's application to this exact problem can be found in the advancedChangeDetection example.
*/
void updateExperiment (void) {
	if (trialPhase == "drawStimuli") {
		//At this phase of the experiment, we want to draw all of our stimuli for the coming trial.

		//The CX_SlidePresenter is an abstraction that is responsible for displaying visual stimuli for specified durations.
		//One called SlidePresenter is instanstiated for you, but you can create more if you want.
		SlidePresenter.clearSlides(); //Start by clearing all slides (from the last trial).

		//To draw to a slide, call beginDrawingNextSlide() with the name of the slide and the duration
		//that you want the contents of the slide to be presented for. The time unit used in CX is
		//microseconds (10^-6 seconds; 10^-3 milliseconds).
		SlidePresenter.beginDrawingNextSlide(1000000, "fixation");
		//After calling beginDrawingNextSlide(), all drawing commands will be directed to the current
		//slide until beginDrawingNextSlide() is called again or endDrawingCurrentSlide() is called.
		drawFixation(); //See the definition of this function below for some examples of how to draw stuff.
	
		//Add some more slides.
		SlidePresenter.beginDrawingNextSlide(250000, "blank");
		drawBlank();

		SlidePresenter.beginDrawingNextSlide(500000, "sample");
		drawSampleArray( trials.at( trialIndex ) );

		SlidePresenter.beginDrawingNextSlide(1000000, "maintenance");
		drawBlank();

		//The duration given for the last slide must be > 0, but is otherwise ignored.
		//The last slide has an infinite duration: Once it is presented, it will stay
		//on screen until something else is drawn (i.e. the slide presenter does not
		//remove it from the screen after the duration is complete). If this is confusing
		//to you, consider the question of what the slide presenter should replace the
		//last frame with that will always be correct.
		SlidePresenter.beginDrawingNextSlide(1, "test");
		drawTestArray( trials.at( trialIndex ) );
		SlidePresenter.endDrawingCurrentSlide(); //After drawing the last slide, it is good form to call endDrawingCurrentSlide().

		SlidePresenter.startSlidePresentation(); //Once all of the slides are ready to go for the next trial,
			//call startSlidePresentation() to do just that. The drawn slides will be drawn on the screen for
			//the specified duration.

		trialPhase = "presentStimuli";
	}

	if (trialPhase == "presentStimuli") {
		//Check that the slide presenter is still at work (i.e. not yet on the last slide).
		//As soon as the last slide is presented, isPresentingSlides() will return false.
		if (!SlidePresenter.isPresentingSlides()) {
			Input.Keyboard.clearEvents(); //Clear all keyboard responses, if any, made during the frame presentation.
			trialPhase = "getResponse";
		}
	}

	if (trialPhase == "getResponse") {
		while (Input.Keyboard.availableEvents() > 0) { //While there are available events, 

			CX_KeyEvent_t keyEvent = Input.Keyboard.getNextEvent(); //get the next event for processing.

			//Only examine key presses (as opposed to key releases or repeats). Everything would probably work 
			//just fine without this check for this experiment, but it is generally a good idea to filter your input.
			if (keyEvent.eventType == CX_KeyEvent_t::PRESSED) {

				//Ignore all responses that are not s or d.
				if (keyEvent.key == 's' || keyEvent.key == 'd') {
				
					//trials.at( trialIndex ).response = keyEvent; //Store the raw response event.

					//Figure out the response time. CX does no automatic response time calculation. You have
					//to find out when the stimulus that the participant is responding to was presented. In
					//this case, that is easy to do because the SlidePresenter tracks that information for us.
					//The last slide (given by getSlides().back()) has the slide presentation time stored in
					//the actualSlideOnset member.
					uint64_t testArrayOnset = SlidePresenter.getSlides().back().actual.startTime;
					//One you have the onset time of the test array, you can subtract that from the time
					//of the response, giving the "response time" (better known as response latency).
					trials.at( trialIndex ).responseTime = keyEvent.eventTime - testArrayOnset;

					//Code the response. For a lot of keys, you can compare the CX_KeyEvent_t::key to a character literal.
					if ((trials.at( trialIndex ).changeTrial && keyEvent.key == 'd') || (!trials.at( trialIndex ).changeTrial && keyEvent.key == 's')) {
						trials.at(trialIndex).responseCorrect = true;
						cout << "Correct!" << endl;
					} else {
						trials.at(trialIndex).responseCorrect = false;
						cout << "Incorrect" << endl;
					}

					//The end of a trial is a good time to flush() the logs, to see if any warnings/errors have happened during the trial.
					//See example-logging in the ofxCX folder for an example of how the logging system works.
					Log.flush();

					//This trial is now complete, so move on to the next trial, checking to see if you have completed all of the trials.
					if (++trialIndex >= trials.size()) {
						outputData();
						cout << "Experiment complete: exiting..." << endl;
						ofSleepMillis(3000);
						ofExit();
					}
					trialPhase = "drawStimuli";
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
	ofPoint screenCenter(Display.getResolution().x / 2, Display.getResolution().y / 2);
	for (int i = 0; i < 9; i++) {
		int col = i % 3;
		int row = i / 3;

		ofPoint p;
		p.x = screenCenter.x - 100 + (row * 100);
		p.y = screenCenter.y - 100 + (col * 100);

		objectLocations.push_back(p);
	}


	vector<TrialData_t> _trials;

	trialCount = trialCount + (trialCount % 2); //Make sure you have an even number of trials

	vector<int> changeTrial = CX::repeat(intVector<int>(0, 1), trialCount / 2);

	for (int trial = 0; trial < trialCount; trial++) {

		TrialData_t tr;
		tr.arraySize = 4;

		//RNG is an instance of CX_RandomNumberGenerator that is instantiated for you. It is useful for a variety of randomization stuff.
		//This version of shuffleVector() returns a shuffled copy of the argument without changing the argument.
		vector<int> colorIndices = RNG.shuffleVector( CX::intVector<int>(0, objectColors.size() - 1) );
		
		for (int i = 0; i < tr.arraySize; i++) {
			tr.colors.push_back( objectColors[colorIndices[i]] );
		}

		//This randomly picks tr.arraySize locations from objectLocations without replacement.
		tr.locations = RNG.sample(tr.arraySize, objectLocations, false);

		tr.changeTrial = changeTrial[trial];
		if (tr.changeTrial) {
			//randomInt() returns an unsigned int from the given range (including both endpoints).
			tr.changedObjectIndex = RNG.randomInt(0, tr.arraySize - 1); 
			tr.newColor = objectColors[colorIndices.at(tr.arraySize)]; //The color at colorIndices.at(tr.arraySize) is past the end
				//of the colors sampled for the stimuli, so it can be used for the changed stimulus
		} else {
			//You don't have to set the newColor or the changedObjectIndex for a no-change trial because there is no change and they won't be used.
		}
		
		_trials.push_back( tr );
	}

	//This version of shuffleVector() (which takes the address of a vector) shuffles the argument, returning nothing.
	RNG.shuffleVector( &_trials );

	return _trials;
}

/*
This function is here to show how sucky outputting data is when you have a user-defined struct to store data
becuase of the lack of reflection in c++. The main problems are:

1) Making errors because the column names must line up with the data, but are output at different places, so
it is really easy to make an error in the naming of columns (swapping names).
2) Leaving out a piece of data just because it was forgotten. This is easy to do if you have 20+ columns of data,
some of which are not very important to the main thrust of the experiment, but might be useful for secondary analyses.
Even for an experiment as simple as this example, making sure that nothing was missing took a little time.
3) It is a waste of time to manually write data outputting functions, because it is a trivial problem yet must be done carefully.
You have to make sure that you always include the delimiter between items, that line endings are in the right place, etc. This
is all a waste of time.

There is an example called advancedChangeDetectionTask than shows how using a CX_DataFrame allows you to totally
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
			it->responseTime << t << it->newColor;

		//Enclose the vectors in quotes so that they are not split on a delimiter when reading into a spreadsheet.
		out << "\"";
		for (int i = 0; i < it->colors.size(); i++) {
			out << it->colors[i];
			if (i < it->colors.size() - 1) {
				out << ";";
			}
		}
		out << "\"";

		//CX::vectorToString() does the above ^^^ but doesn't automatically add quotes. vectorToString makes manually outputting data
		//better, but still loses out to the convenience of a data frame.
		out << t << "\"" << CX::vectorToString(it->locations, ";") << "\"";
		out << endl;
	}

	CX::writeToFile("CD data.txt", out.str(), false); //This file can be found in the data directory of the project.
		//i.e. %openFrameworks directory%/apps/myApps/%your app name%/bin/data. You can also specify an absolute path
		//and the file should end up there as long the location does not require write permissions.
}

/*
Drawing stuff in CX just uses built in oF drawing functions.
This section gives some examples of such functions, although there
are many more, including 3D drawing stuff.
*/
void drawFixation (void) {
	ofBackground(backgroundColor);

	ofSetColor( ofColor( 255 ) );
	ofSetLineWidth( 3 );

	ofPoint centerpoint( Display.getResolution().x/2, Display.getResolution().y/2 );

	ofLine( centerpoint.x - 10, centerpoint.y, centerpoint.x + 10, centerpoint.y );
	ofLine( centerpoint.x, centerpoint.y - 10, centerpoint.x, centerpoint.y + 10 );
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

	vector<ofColor> testColors = tr.colors;
	
	if (tr.changeTrial) {
		testColors.at( tr.changedObjectIndex ) = tr.newColor;
	}
	
	ofBackground( backgroundColor );

	for (int i = 0; i < testColors.size(); i++) {
		ofSetColor( testColors.at(i) );
		ofCircle( tr.locations.at(i), circleRadius );
	}
}