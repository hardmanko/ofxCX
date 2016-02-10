#include "CX.h"

/*
This example is a more advanced version of the change detection task presented in
the basicChangeDetection example. It is not "advanced" because it is more complex,
but because it uses more features of CX. It actually ends up being simpler because 
of how it uses the features of CX. The advances features are

1) Using CX_DataFrame (more information can be found in the dataFrame example).
2) Using CX_CoordinateConverter and CX_DegreeToPixel converter, which makes it
possible to work in units of degrees of visual angle rather than pixels.

Items that are commented are new, although not all new stuff will neccessarily be 
commented. The main feature that as demonstrated as the CX_DataFrame, which is a
way to store and output experimental data. Using custom units and a custom 
coordinate system is shown with CX_CoordinateConverter and CX_DegreeToPixelConverter
as the unit converter.
*/

CX_DataFrame generateTrials(int trialCount);

void drawStimuli(void);
void presentStimuli(void);
void getResponse(void);

void drawFixation (void);
void drawBlank (void);
void drawSampleArray (void);
void drawTestArray (void);

CX_SlidePresenter SlidePresenter;

CX_DataFrame trialDf; //The data frame into which data from each trial is stored.
int trialIndex = 0; //The index of the current trial.

int circleRadius = 0; //We will set this later in terms of visual angle units.
ofColor backgroundColor(50);

void runExperiment (void) {

	trialDf = generateTrials(8);

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
	Clock.sleep(3000);
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

	SlidePresenter.beginDrawingNextSlide(1, "test");
	drawTestArray();
	SlidePresenter.endDrawingCurrentSlide();
}

void presentStimuli(void) {

	//In the basic example, there were a few steps here, but presentSlides() does all of them for you.
	SlidePresenter.presentSlides();

	Input.pollEvents();
	Input.Keyboard.clearEvents();
}

void getResponse (void) {
	while (true) {
		Input.pollEvents();

		while (Input.Keyboard.availableEvents() > 0) {
			CX_Keyboard::Event keyEvent = Input.Keyboard.getNextEvent();
			if (keyEvent.type == CX_Keyboard::PRESSED) {

				if (keyEvent.key == 'S' || keyEvent.key == 'D') {

					CX_Millis testArrayOnset = SlidePresenter.getSlideByName("test").actual.startTime;
					trialDf(trialIndex, "responseLatency") = keyEvent.time - testArrayOnset;

					bool changeTrial = trialDf(trialIndex, "changeTrial").to<bool>();

					if ((changeTrial && keyEvent.key == 'D') || (!changeTrial && keyEvent.key == 'S')) {
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

CX_DataFrame generateTrials (int trialCount) {

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
		//for many monitors) and viewer sitting 60 cm from screen. Set this is to the correct value for whatever monitor you
		//are actually using for real experiments!
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

	vector<bool> changeTrial = RNG.sample(trialCount, Util::concatenate<bool>(false, true), true);

	CX_DataFrame df;

	for (int trial = 0; trial < trialCount; trial++) {

		//Using a CX_DataFrameRow, we can fill in different columns, then once the row is filled,
		//append it to a data frame. This makes the code a little more compact because you don't
		//need to mess with the row index, becuase you only have the one row at a time.
		CX_DataFrameRow tr;
		int arraySize = 4;
		tr["arraySize"] = arraySize; //Use square brackets with a string to select the column.

		//Note that you'll have to use functions like toInt() in cases like this, where it isn't obvious what type the data should be converted to.
		//In this case, it would be converted to unsigned int (because that's what the function takes), which gives a warning because the inserted
		tr["colors"] = RNG.sample(arraySize, objectColors, false);

		tr["locations"] = RNG.sample(arraySize, objectLocations, false);

		tr["changeTrial"] = (bool)changeTrial[trial]; 

		if (changeTrial[trial]) {
			tr["changedObjectIndex"] = RNG.randomInt(0, arraySize - 1);

			//Here, we need to get a vector of ofColors out, so we use tr["colors"].toVector<ofColor>()
			tr["newObjectColor"] = RNG.sampleExclusive(objectColors, tr["colors"].toVector<ofColor>());
		} else {
			tr["changedObjectIndex"] = -1;
			tr["newObjectColor"] = backgroundColor;
		}
		
		df.appendRow( tr );
	}

	df.shuffleRows(); //Shuffle all of the rows of the data frame so that the trials come in random order.

	//After generating the trials, the column names for all of the parameters that control those trials will be 
	//in the data frame, but we still need to add two more columns for response data and a column to track presentation errors:
	df.addColumn("responseCorrect");
	df.addColumn("responseLatency");
	df.addColumn("presentationErrors");

	//cout << df.print();

	Log.flush(); //Check for errors that might have occurred during trial generation

	return df;
}

void drawFixation(void) {
	ofBackground(backgroundColor);

	ofSetColor(ofColor(255));
	Draw::fixationCross(Disp.getCenter(), 30, 5);
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