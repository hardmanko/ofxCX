/* This example is an implementation of a flanker task that uses
the letters X and Y as the stimuli. The participant should press
the key corresponding to the letter that is in the center of the
row of letters, ignoring the flanking letters.

This example shows how to
1) collect keyboard data using CX_Keyboard::waitForKeypress(),
2) control timing directly using the Clock,
3) load and draw fonts of varying sizes,
4) and store and output data with a CX_DataFrame.

This example assumes that you have already read the helloWorld
example.
*/

#include "CX.h"

void runExperiment(void) {

	Input.Keyboard.enable(true); //We will be using the keyboard, so enable it.

	//Make a vector of the two letters that will be used, as strings.
	vector<string> letters(2);
	letters[0] = "X";
	letters[1] = "Y";

	//Also make a vector of the possible response keys, as chars, 
	//which are implicitly converted to ints. We need the keys to be
	//ints.
	vector<int> allowedKeys(2);
	allowedKeys[0] = 'X';
	allowedKeys[1] = 'Y';

	//We will use this font to present the letters.
	ofTrueTypeFont font;
	font.loadFont(OF_TTF_MONO, 48); //OF_TTF_MONO selects an available monospaced font, which we load at size 48.

	CX_DataFrame data; //Make a CX_DataFrame to store data in later.

	int trialCount = 12; //We'll do 12 trials.
	
	for (int trial = 0; trial < trialCount; trial++) {

		//For each trial, we are going to present a blank screen for 1 second.
		Disp.beginDrawingToBackBuffer();
			ofClear(ofColor::black);
		Disp.endDrawingToBackBuffer();

		Disp.swapBuffers();

		//Once the blank screen has been swapped in, note that we want the
		//letters to be presented after 1 second. We will then do some setup
		//for the trial, including drawing the letters to the back buffer during
		//the 1 second blank. Finally, once the setup is complete, we will wait
		//until the 1 second is up to swap the letters in.
		CX_Millis letterPresentationStartTime = Clock.now() + CX_Seconds(1);

		//Now we set up the trial.

		//For each trial, we want to randomize whether X or Y is the central letter.
		//RNG.shuffleVector() makes a copy of the letters, shuffles that copy, and returns it.
		vector<string> shuffledLetters = RNG.shuffleVector(letters);

		//Take letter 0 in the shuffled vector as the central letter.
		string centralLetter = shuffledLetters[0]; 

		//Choose whether it should be a trial on which the flanking letters interfere with the central letter.
		//interferenceTrial can be thought of as boolean, 1 if there should be interference, 0 otherwise.
		int interferenceTrial = RNG.randomInt(0, 1);

		//Pick the flanking letter based on the value of interferenceTrial. If it's 0, 
		//central and flanking letters are the same, if it's 1, they are different.
		string flankingLetter = shuffledLetters[interferenceTrial];

		//Mush together the letters into one string.
		string presentedLetters = flankingLetter + flankingLetter + centralLetter + flankingLetter + flankingLetter;

		//Draw the letters to the back buffer.
		Disp.beginDrawingToBackBuffer();
			ofClear(ofColor::black);
			ofSetColor(ofColor::white);
			Draw::centeredString(Disp.getCenter(), presentedLetters, font);
		Disp.endDrawingToBackBuffer();


		//Now we are done with the setup for this trial, so we just need to wait until the
		//blank screen has been on screen for 1 second, so we do a little loop.
		while (Clock.now() < letterPresentationStartTime)
			;

		//Swap in the letters and mark the current time.
		Disp.swapBuffers();
		CX_Millis startTime = Clock.now();

		//Using the allowed response keys set up at the beginning, wait until one of
		//those keys has been pressed. A CX_Keyboard::Event from the pressed key
		//is returned by waitForKeypress(), which we can use to learn which key was
		//pressed and at what time it was pressed.
		CX_Keyboard::Event response = Input.Keyboard.waitForKeypress(allowedKeys);

		//Store data from this trial. To access data in a CX_DataFrame, you give a row number and a column name
		//within parentheses. In this case, trial gives the row number and various strings give the column names.
		//For more information, see the dataFrame example.
		data(trial, "centralLetter") = centralLetter;
		data(trial, "interferenceTrial") = interferenceTrial;

		//We cast to char from the integer keycode to get a nice printed representation of the key that was pressed. This only works for some printable characters.
		data(trial, "responseKey") = (char)response.key;
		data(trial, "responseLatency") = response.time - startTime;

		//This is the end of one trial, at which point the loop restarts for the next trial.
	}

	//At the end of presenting all of the trials, output the data from the task.
	data.printToFile("flankerData.txt");
}