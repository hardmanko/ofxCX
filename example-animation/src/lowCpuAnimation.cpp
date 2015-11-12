#include "CX.h"

/*
Because CX is supposed to be for high-performance real-time applications, generally code
written for it is run in very tight loops that are constantly checking something, like
if a response has been received or a if buffer swap has occured. This has the side
effect of making CX programs huge CPU hogs, because they are always doing something,
so they ask for maximum CPU time from the operating system. This is not a problem if
you are using a modern computer with a multi-core CPU because the CX program can peg one
core to 100% and everything else that needs to happen in other programs can happen
without cutting into the CPU time that CX wants. Only on a single-core CPU would CX
need to share CPU time with other programs.

If you have an issue with limited CPU time, buy a modern computer with more than 1
core. Honestly, this is the best solution. The next best solution is to figure out
how much of a CPU hog your CX program needs to be and then scale back on the amount
of CPU time requested by your CX program. One way to do this is to insert sleeps into
your code at appropriate times. But, seriously, get multiple cores.

This example makes a few small changes to the animation example that are designed to profile 
the animation code to see how long it takes to draw everything and process input, and adds
a small amount of sleeping in order to reduce CPU use. We will be able to see from the
profiling if this trivial example really needs to hog the CPU 100% of the time and, if not,
we can feel good about putting in some sleeps.
*/

//Use CX::Instances::Clock for time information for the segment profiler.
Util::CX_SegmentProfiler segmentProfiler(&Clock);


int mouseX = 100;
double circleRadius = 30;
double angles[3] = { 0, 0, 0 };
double angleMultiplier[3] = { 1, 2, 3 };
int directions[3] = { 1, 1, 1 };
double distancesFromCenter[3] = { 75, 150, 225 };
double distanceMultiplier = 1;
CX_Millis lastAnimationDrawTime = 0;

void updateAnimation(void);
void drawNextFrameOfAnimation(void);
ofPoint getCircleLocation(int circleIndex);



//This function is the same as in the basic animation example.
void runExperiment(void) {
	Input.setup(false, true);
	Disp.setWindowResolution(600, 600);
	Disp.setAutomaticSwapping(true);

	segmentProfiler.name = "Time spent active profiler";

	while (true) {
		updateAnimation();
	}
}



void updateAnimation(void) {

	//In this version of the example, if the display has not just swapped, we won't do anything except sleep for 1 ms.
	//Sleeping is effectively telling the operating system "I don't need to do anything for the next N milliseconds,
	//so give the CPU time I would normally have gotten to some other program." We will just sleep for 1 millisecond,
	//because we want to regularly check to see if the buffer swap has occured.
	if (!Disp.hasSwappedSinceLastCheck()) {
		Clock.sleep(1); //If the buffer swap hasn't happened, sleep more.
	} else {

		//We will check to see how long it takes to check for input and to draw the animation.
		//Here we mark time point 1, which is the beginning of the segment of code you want to profile.
		segmentProfiler.t1();


		//It's good to check if there was ant input during the past frame before drawing new stuff that is 
		//dependent on that input.
		Input.pollEvents();
		while (Input.Mouse.availableEvents() > 0) {
			CX_Mouse::Event mev = Input.Mouse.getNextEvent();
			switch (mev.type) {
			case CX_Mouse::MOVED:
				mouseX = mev.x;
				break;
			case CX_Mouse::PRESSED:
				for (int i = 0; i < 3; i++) {
					if (getCircleLocation(i).distance(ofPoint(mev.x, mev.y)) <= circleRadius) {
						directions[i] *= -1;
					}
				}
				break;
			case CX_Mouse::SCROLLED:
				distanceMultiplier += mev.y * .02; //The y component of the scroll wheel is the typical scroll wheel on most mice
				distanceMultiplier = Util::clamp(distanceMultiplier, -1.5, 1.5);
				break;
			}
		}

		Disp.beginDrawingToBackBuffer(); 
		drawNextFrameOfAnimation();
		Disp.endDrawingToBackBuffer();


		//We are now out of the segment of code we want to profile, so we mark time point 2.
		segmentProfiler.t2();

		//If enough samples have been collected, we will print out the data.
		if (segmentProfiler.collectedSamples() == 120) {
			Log.notice() << segmentProfiler.getStatString();
			Log.flush();

			segmentProfiler.restart(); //Clear all of the existing samples so that every 
				//time data is printed, it is a totally new set of data.
		}

	}
}

void drawNextFrameOfAnimation(void) {
	static ofColor colors[3] = { ofColor::red, ofColor::green, ofColor::blue };

	ofBackground(0);

	ofSetColor(255);
	ofDrawBitmapString("Move the mouse to the left or right to change speed.\n"
					   "Click on a circle to change its direction.\n"
					   "Use the mouse wheel to change the orbit size.", ofPoint(30, 30));

	CX_Millis now = Clock.now();
	CX_Millis elapsedTime = now - lastAnimationDrawTime;
	lastAnimationDrawTime = now;

	for (int i = 0; i < 3; i++) {
		angles[i] += elapsedTime.seconds() / 5 * mouseX * directions[i] * angleMultiplier[i];
		ofSetColor(colors[i]);
		ofCircle(getCircleLocation(i), circleRadius);
	}
}

ofPoint getCircleLocation(int circleIndex) {
	return Util::getRelativePointFromDistanceAndAngle(Disp.getCenter(),
													  distancesFromCenter[circleIndex] * distanceMultiplier,
													  angles[circleIndex]);
}