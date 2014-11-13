#include "CX.h"

/*
This example is a more advanced version of the change detection task presented in
the basicChangeDetection example. It is not "advanced" because it is more complex,
but because it uses more features of CX. It actually ends up being simpler because 
of how it uses features of CX.

Items that are commented are new, although not all new stuff will neccessarily be 
commented. The main feature that as demonstrated as the CX_DataFrame, which is a
way to store and output experimental data. Using custom units and a custom 
coordinate system is shown with CX_CoordinateConverter and CX_DegreeToPixelConverter
as the unit converter.
*/

void drawStimuli(void);
void presentStimuli(void);
void getResponse(void);

void generateTrials (int trialCount);
void updateExperiment (void);

CX_SlidePresenter SlidePresenter;
void drawFixation (void);
void drawBlank (void);
void drawSampleArray (void);
void drawTestArray (void);

CX_DataFrame trialDf; //The data frame into which data from each trial is stored.
int trialIndex = 0; //The index of the current trial.

int circleRadius = 0; //We will set this later.
ofColor backgroundColor(50);

void runExperiment (void) {

	generateTrials(8);

	Input.setup(true, false);

	SlidePresenter.setup(&Disp);

	Log.notice() << "Instructions: Press \'s\' for same, \'d\' for different. Press escape to quit.";
	Log.flush();

	for (trialIndex = 0; trialIndex < trialDf.getRowCount(); trialIndex++) {
		drawStimuli();
		presentStimuli();
		getResponse();

		Log.flush();
	}

	trialDf.printToFile("change detection data.txt"); //This is all you have to do to output the data from the data frame. 
		//Compare to the data output function from the basicChangeDetectionTask example.
	Log.notice() << "Experiment complete: exiting...";
	Log.flush();
	ofSleepMillis(3000);
}

void drawStimuli (void) {
	SlidePresenter.clearSlides();

	SlidePresenter.beginDrawingNextSlide(1000, "fixation");
	drawFixation();
	
	SlidePresenter.beginDrawingNextSlide(250, "blank");
	drawBlank();

	SlidePresenter.beginDrawingNextSlide(500, "sample");
	drawSampleArray();

	SlidePresenter.beginDrawingNextSlide(1000, "maintenance");
	drawBlank();

	SlidePresenter.beginDrawingNextSlide(.001, "test");
	drawTestArray();
	SlidePresenter.endDrawingCurrentSlide();

	SlidePresenter.startSlidePresentation();
}

void presentStimuli(void) {
	while (SlidePresenter.isPresentingSlides()) {
		SlidePresenter.update();
	}

	Input.pollEvents();
	Input.Keyboard.clearEvents();
}

void getResponse (void) {
	while (true) {
		Input.pollEvents();

		while (Input.Keyboard.availableEvents() > 0) {
			CX_Keyboard::Event keyEvent = Input.Keyboard.getNextEvent();
			if (keyEvent.type == CX_Keyboard::PRESSED) {

				if (keyEvent.key == 's' || keyEvent.key == 'd') {

					CX_Millis testArrayOnset = SlidePresenter.getSlides().back().actual.startTime;
					trialDf(trialIndex, "responseLatency") = keyEvent.time - testArrayOnset;

					bool changeTrial = trialDf(trialIndex, "changeTrial").to<bool>();

					if ((changeTrial && keyEvent.key == 'd') || (!changeTrial && keyEvent.key == 's')) {
						trialDf(trialIndex, "responseCorrect") = true;
						Log.notice() << "Response correct!";
					} else {
						trialDf(trialIndex, "responseCorrect") = false;
						Log.notice() << "Response incorrect.";
					}

					trialDf(trialIndex, "presentationErrors") = SlidePresenter.checkForPresentationErrors().totalErrors();

					cout << SlidePresenter.printLastPresentationInformation() << endl;

					return;
				}
			}
		}
	}
}

void generateTrials (int trialCount) {

	vector<ofColor> objectColors;
	objectColors.push_back( ofColor::red );
	objectColors.push_back( ofColor::orange );
	objectColors.push_back( ofColor::yellow );
	objectColors.push_back( ofColor::green );
	objectColors.push_back( ofColor::blue );
	objectColors.push_back( ofColor::purple );

	//Make a 3x3 grid of object locations around the center of the screen. This time
	//we do it in units of degrees of visual angle by using a CX_CoordinateConverter
	//and a CX_DegreeToPixelConverter.
	Util::CX_CoordinateConverter cc(Disp.getCenter(), false, true); //Set the origin to be at the center of the display
		//and invert the y-axis.
	Util::CX_DegreeToPixelConverter d2p(35, 60); //Assume 35 pixels per cm on the monitor (this is fairly close to correct 
		//for many monitors) and viewer sitting 60 cm from screen.
	cc.setUnitConverter(&d2p); //Set the units of the coordinate converter to be in degrees of visual angle, as calculated by
		//the CX_DegreeToPixelConverter.

	vector<float> xDegrees;
	xDegrees.push_back(-3); //Make the objects be 3 degrees of visual angle apart
	xDegrees.push_back(0); //Centered at the origin.
	xDegrees.push_back(3);

	vector<float> yDegrees = xDegrees;

	vector<ofPoint> objectLocations;
	for (auto x : xDegrees) {
		for (auto y : yDegrees) {
			ofPoint pixelLocation = cc(x, y); //Convert values in degrees to pixels, also
			objectLocations.push_back(pixelLocation);
		}
	}

	//We'll also use the degree to pixel converter to make our circles have a diameter of 1.5 degrees of visual angle:
	circleRadius = d2p(1.5/2); //Radius being half the diameter.


	trialCount = trialCount + (trialCount % 2); //Make sure you have an even number of trials

	vector<int> changeTrial = Util::repeat( Util::intVector<int>(0, 1), trialCount/2 );	

	for (int trial = 0; trial < trialCount; trial++) {

		CX_DataFrameRow tr;
		tr["arraySize"] = 4;

		vector<unsigned int> colorIndices = RNG.shuffleVector( Util::intVector<unsigned int>(0, objectColors.size() - 1) );
		vector<ofColor> colors;

		//Note that you'll have to use functions like toInt() in cases like this, where it isn't obvious what type the data should be converted to.
		for (int i = 0; i < tr["arraySize"].toInt(); i++) {
			colors.push_back( objectColors[colorIndices[i]] );
		}

		tr["colors"] = colors;
		unsigned int newColorIndex = colorIndices[ tr["arraySize"].toInt() ];

		tr["locations"] = RNG.sample( tr["arraySize"].toInt(), objectLocations, false );

		tr["changeTrial"] = (bool)changeTrial[trial]; //Cast changeTrial to bool so that it will be stored
			//by the data frame as a boolean value, which means you won't get warnings when extracting the
			//value from the cell as bool.
		if (changeTrial[trial]) {
			tr["changedObjectIndex"] = (int)RNG.randomInt( 0, tr["arraySize"].toInt() - 1 ); //Store explicitly as int
			tr["newObjectColor"] = objectColors[ newColorIndex ];
		}
		
		trialDf.appendRow( tr );
	}

	trialDf.shuffleRows(); //Shuffle all of the rows of the data frame so that the trials come in random order.

	//After generating the trials, the column names for all of the parameters that control those trials will be 
	//in the data frame, but we still need to add two more columns for response data and a column to track presentation errors:
	trialDf.addColumn("responseCorrect");
	trialDf.addColumn("responseLatency");
	trialDf.addColumn("presentationErrors");

	//cout << trialDf.print();

	Log.flush(); //Check for errors that might have occurred during trial generation

}

void drawFixation (void) {
	ofBackground( backgroundColor );

	ofPoint centerpoint = Disp.getCenter();

	ofSetColor(ofColor(255));
	Draw::line(ofPoint(centerpoint.x - 10, centerpoint.y), ofPoint(centerpoint.x + 10, centerpoint.y), 3);
	Draw::line(ofPoint(centerpoint.x, centerpoint.y - 10), ofPoint(centerpoint.x, centerpoint.y + 10), 3);
}

void drawBlank (void) {
	ofBackground( backgroundColor );
}

void drawSampleArray (void) {
	ofBackground( backgroundColor );

	//We know that the contents of the colors and locations cells are vectors, so we read them out into vectors of the appropriate type.
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
	
	//We can do a little type conversion on extraction from the data frame to make sure that we are getting data of the correct type.
	if (trialDf(trialIndex, "changeTrial").to<bool>()) { //Here we say that we want a boolean value
		testColors.at( trialDf(trialIndex, "changedObjectIndex").toInt() ) = trialDf(trialIndex, "newObjectColor").to<ofColor>(); //Here we want an ofColor.
	}
	
	ofBackground( backgroundColor );

	for (int i = 0; i < testColors.size(); i++) {
		ofSetColor( testColors.at(i) );
		ofCircle( locations.at(i), circleRadius );
	}
}