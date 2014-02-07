#include "CX_EntryPoint.h"

/*! \file
This example is a more advanced version of the change detection task presented in
the basicChangeDetectionTask example. It is not "advanced" because it is more complex,
but because it uses more features of CX. It actually ends up being more simple
because of how it uses features of CX.

Items that are commented are new, although not all new stuff will neccessarily be 
commented. The two main features that are demonstrated are CX_SafeDataFrame and
CX_TrialController.
*/

/*
One of the main additions to this example is the CX_TrialController. As you will
see, the trial controller manages which stage of the trial you are in, which means
that you don't have to track the trial stage with a variable.
Functions that are given to a CX_TrialController must take void and return int.
*/
CX_TrialController trialController;
int drawStimuli (void);
int presentStimuli (void);
int getResponse (void);

void generateTrials (int trialCount);

void drawFixation (void);
void drawBlank (void);
void drawSampleArray (void);
void drawTestArray (void);


CX_SafeDataFrame trialDf;
int trialIndex = 0;

int circleRadius = 30;
ofColor backgroundColor(50);

void setupExperiment (void) {
	generateTrials(8);

	Input.setup(true, false);

	CX_SP_Configuration config;
	config.errorMode = CX_SP_ErrorMode::FIX_TIMING_FROM_FIRST_SLIDE;
	config.display = &Display;

	SlidePresenter.setup(config);

	cout << "Insturctions: Press \'s\' for same, \'d\' for different. Press escape to quit." << endl;

	//Add the functions to the trial controller in the order in which you want them to be called.
	trialController.appendFunction( &drawStimuli );
	trialController.appendFunction( &presentStimuli );
	trialController.appendFunction( &getResponse );
	trialController.start();
}

/*
In updateExperiment in the basicChangeDetection example, there were three stages of each
trial that were gone through in order. The trialController helps with progressing through
the stages. In setupExperiment, three functions, each of which handles one stage of the
trial, were put into the trialController. In updateExperiment, the update function of the
trialController is called, which simply calls the function for the current stage of the
trial. When the current function determines that its stage of the trial is complete, it
indicates that to the trial controller by returning 1, and then the trialController moves
on to the next stage of the trial, calling the next function on the next call to update().

This makes updateExperiment trivial, with all of the processing offloaded into 
sub-functions.
*/
void updateExperiment (void) {
	trialController.update();
}

int drawStimuli (void) {
	SlidePresenter.clearSlides();

	SlidePresenter.beginDrawingNextSlide(1000000, "fixation");
	drawFixation();
	
	SlidePresenter.beginDrawingNextSlide(250000, "blank");
	drawBlank();

	SlidePresenter.beginDrawingNextSlide(500000, "sample");
	drawSampleArray();

	SlidePresenter.beginDrawingNextSlide(1000000, "maintenance");
	drawBlank();

	SlidePresenter.beginDrawingNextSlide(1, "test");
	drawTestArray();
	SlidePresenter.endDrawingCurrentSlide();

	SlidePresenter.startSlidePresentation();
	return 1; //If you want the trial controller to move on to the next function in its list, return 1 from the current function.
		//We only want to draw everything once per trial, so this function only ever returns 1.
}

//In this function, we want to check repeatedly if the SlidePresenter is presenting slides.
//If its done, then we want to move on to the next stage of the trial, which is getting 
//the response.
int presentStimuli (void) {
	if (!SlidePresenter.isPresentingSlides()) {
		Input.Keyboard.clearEvents();
		return 1; //Move on to next function
	}
	return 0; //If you don't want the trial controller to move on, return 0.
}

int getResponse (void) {
	while (Input.Keyboard.availableEvents() > 0) {

		CX_KeyEvent_t keyEvent = Input.Keyboard.getNextEvent();

		if (keyEvent.eventType == CX_KeyEvent_t::PRESSED) {

			if (keyEvent.key == 's' || keyEvent.key == 'd') {

				uint64_t testArrayOnset = SlidePresenter.getSlides().back().actual.startTime;
				trialDf(trialIndex, "responseTime") = keyEvent.eventTime - testArrayOnset;

				bool changeTrial = trialDf(trialIndex, "changeTrial").to<bool>();

				if ((changeTrial && keyEvent.key == 'd') || (!changeTrial && keyEvent.key == 's')) {
					trialDf(trialIndex, "responseCorrect") = true;
					Log.notice("myExperiment") << "Correct!";
				} else {
					trialDf(trialIndex, "responseCorrect") = false;
					Log.notice("myExperiment") << "Incorrect";
				}

				trialDf(trialIndex, "presentationErrors") = SlidePresenter.checkForPresentationErrors();

				Log.flush();

				if (++trialIndex >= trialDf.getRowCount()) {
					trialDf.printToFile("change detection data.txt"); //This is all you have to do to output the data from the data frame. 
						//Compare to the data output function from the basicChangeDetectionTask example.
					cout << "Experiment complete: exiting..." << endl;
					ofSleepMillis(3000);
					ofExit();
				}
				return 1; //Move on to next function. In this case, this function is at the end of the list, 
					//so the trial controller wraps around and calls the first function in the list.
			}
		}
	}
	return 0;
}

void generateTrials (int trialCount) {

	vector<ofColor> objectColors;
	vector<ofPoint> objectLocations;

	//Set up a vector of colors that will be sampled to make the objects.
	objectColors.push_back( ofColor::red );
	objectColors.push_back( ofColor::orange );
	objectColors.push_back( ofColor::yellow );
	objectColors.push_back( ofColor::green );
	objectColors.push_back( ofColor::blue );
	objectColors.push_back( ofColor::purple );

	//Make a 3x3 grid of object locations around the center of the screen.
	for (int i = 0; i < 9; i++) {
		int col = i % 3;
		int row = i / 3;

		ofPoint p;
		p.x = Display.getCenterOfDisplay().x - 100 + (row * 100);
		p.y = Display.getCenterOfDisplay().y - 100 + (col * 100);

		objectLocations.push_back( p );
	}

	trialCount = trialCount + (trialCount % 2); //Make sure you have an even number of trials

	vector<int> changeTrial = repeat( intVector<int>(0, 1), trialCount/2 );	

	for (int trial = 0; trial < trialCount; trial++) {

		CX_DataFrameRow tr;
		tr["arraySize"] = 4;

		vector<unsigned int> colorIndices = RNG.shuffleVector( intVector<unsigned int>(0, objectColors.size() - 1) );
		vector<ofColor> colors;

		//Note that you'll have to use functions like toInt() in cases like this, where it isn't obvious what type the data should be converted to.
		for (int i = 0; i < tr["arraySize"].toInt(); i++) {
			colors.push_back( objectColors[colorIndices[i]] );
		}

		tr["colors"] = colors;
		unsigned int newColorIndex = colorIndices[ tr["arraySize"] ];

		tr["locations"] = RNG.sample( tr["arraySize"], objectLocations, false );

		tr["changeTrial"] = changeTrial[trial];
		if (changeTrial[trial]) {
			tr["changedObjectIndex"] = RNG.randomInt( 0, tr["arraySize"].toInt() - 1 ); 
			tr["newObjectColor"] = objectColors[ newColorIndex ];
		}
		
		trialDf.appendRow( tr );
	}

	/*
	set<string> columns;
	columns.insert("changeTrial");
	columns.insert("newObjectColor");
	columns.insert("changedObjectIndex");

	cout << trialDf.print(columns) << endl << endl;
	*/
	trialDf.shuffleRows();

	//cout << trialDf.print(columns) << endl << endl;

	//After generating the trials, the column names for all of those trials will be in the data frame, 
	//but we still need to add two more columns for response data:
	trialDf.addColumn("responseCorrect");
	trialDf.addColumn("responseTime");
	trialDf.addColumn("presentationErrors");

}

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

void drawSampleArray (void) {

	ofBackground( backgroundColor );

	//We know that the contents of the colors and locations *cells* are vectors, so we read them out into vectors of the appropriate type.
	vector<ofColor> colors = trialDf(trialIndex, "colors");
	vector<ofPoint> locations = trialDf(trialIndex, "locations");

	for (int i = 0; i < colors.size(); i++) {
		ofSetColor( colors.at(i) );
		ofCircle( locations.at(i), circleRadius );
	}
}

void drawTestArray (void) {

	vector<ofColor> testColors = trialDf(trialIndex, "colors");
	vector<ofPoint> locations = trialDf(trialIndex, "locations");
	
	if (trialDf(trialIndex, "changeTrial").to<bool>()) {
		testColors.at( trialDf(trialIndex, "changedObjectIndex").toInt() ) = trialDf(trialIndex, "newObjectColor").to<ofColor>();
	}
	
	ofBackground( backgroundColor );

	for (int i = 0; i < testColors.size(); i++) {
		ofSetColor( testColors.at(i) );
		ofCircle( locations.at(i), circleRadius );
	}
}