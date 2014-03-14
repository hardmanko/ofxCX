#include "CX_EntryPoint.h"

#include "CX_ModularSynth.h"

using namespace CX::Synth;






void drawInformation(void);

void simpleTest(void) {
	//Here we create an Oscillator
	Oscillator osc;
	osc.frequency = 1; //We'll set the frequency to 1 Hz
	osc.setGeneratorFunction(Oscillator::sine); //Set the waveform to a sine wave.

	//We are going to manually tell the oscillator that the sample rate is 40 samples per second
	ModuleControlData_t controlData;
	controlData.sampleRate = 40;
	osc.setData(controlData);

	//If the frequency is 1 Hz (1 cycle per second) and there are 40 samples per second, if we take 40 samples,
	//we should expect to see a whole sine wave (0 to 1 to 0 to -1 and back to 0). So, we call getNextSample()
	//40 times to advance 40 samples through the process of generating the waveform and print the result.
	for (int i = 0; i < 40; i++) {
		cout << osc.getNextSample() << endl;
	}
}


/* It is very easy to make your own waveform generating functions to be used with an Oscillator. 
A waveform generating function takes a value that represents the location in the waveform at
the current point in time. These values are in the interval [0,1).

The waveform generating function should return a double representing the amplitude of the
wave at the given waveform position.

To put this all together, a sine wave generator looks like this: */
double sineWaveGeneratorFunction(double waveformPosition) {
	return sin(2 * PI * waveformPosition); //The argument for sin() is in radians. 1 cycle is 2*PI radians.
}

void runExperiment(void) {

	Input.setup(true, true);

	simpleTest();



	StreamOutput output; //StreamOutput is one of the ways to get sound out of a modular synth. 
		//It requires a CX_SoundStream to play the sounds, which is configured below.

	//Configure the sound stream. See the soundObject example for more information about these values. 
	//Also see the documentation for CX_SoundStream::Configuration.
	CX_SoundStream::Configuration config;
	config.api = RtAudio::Api::WINDOWS_DS;
	config.outputChannels = 2;
	config.sampleRate = 48000;
	config.bufferSize = 256;
	config.streamOptions.numberOfBuffers = 4;

	CX_SoundStream ss;
	ss.setup(config);
	ss.start();

	output.setOuputStream(ss); //Set the CX_SoundStream ss as the sound stream for the StreamOutput.


	//Now that we have an output, we can make a really basic synthesizer:
	Oscillator osc;
	osc.setGeneratorFunction(Oscillator::saw); //We'll generate a saw wave.
	osc.frequency = 440; //At 440 Hz (A4)

	osc >> output; //operator>> means that osc feeds into output.

	cout << "Let's listen to a saw wave for 6 seconds" << endl;
	Clock.sleep(CX_Seconds(6));
	
	//Lets add a filter module to the chain.
	RecursiveFilter filter;
	filter.setup(RecursiveFilter::FilterType::LOW_PASS); //Lets have it be a low pass filter.
	filter.cutoff = 600; //Set the cutoff frequency of the filter to 600 Hz, so frequencies past there get attentuated.

	osc >> filter >> output; //Reconnect things so that the osc goes through the filter.



	Mixer oscMix;

	Oscillator mainOsc;
	mainOsc.frequency = 1000;
	mainOsc.setGeneratorFunction(Oscillator::sine);

	Multiplier mainOscGain;
	mainOscGain.amount = .01;

	mainOsc >> mainOscGain >> oscMix;

	//This oscillator doubles the main oscillator, except that its frequency is modified by an LFO.
	Oscillator doublingOsc;
	doublingOsc.setGeneratorFunction(Oscillator::sine);

	Oscillator lfo;
	lfo.setGeneratorFunction(Oscillator::sine);
	lfo.frequency = 5;

	Multiplier lfoGain;
	lfoGain.amount = 2;

	Adder lfoOffset;
	lfoOffset.amount = mainOsc.frequency;

	//Feed to lfo signal (which goes from -1 to 1) into a multiplier to make its range a little bigger, then add an offset to put
	//in into a good frequency range. This offset will be changed along with the mainOsc frequency.
	lfo >> lfoGain >> lfoOffset >> doublingOsc.frequency;
	
	Multiplier doublingOscGain;
	doublingOscGain.amount = .003;

	doublingOsc >> doublingOscGain >> oscMix;


	//Create a filter and run the mod envelope into the filter breakpoint frequency.
	RecursiveFilter filter;
	filter.setup(RecursiveFilter::FilterType::LOW_PASS);

	Envelope modEnv;
	modEnv.a = .1;
	modEnv.d = .1;
	modEnv.s = .5;
	modEnv.r = .2;

	Multiplier modMult;
	modMult.amount = 3000;

	Adder modOffset;
	modOffset.amount = 100;

	modEnv >> modMult >> modOffset >> filter.cutoff;
	
	Envelope ampEnv;
	ampEnv.a = .3;
	ampEnv.d = .2;
	ampEnv.s = .6;
	ampEnv.r = .2;


	//After the mixer, filter, attach the amp envelope, and route into the output.
	mainOsc >> filter >> ampEnv >> output;



	/* You can route the output from a modular synth into a SoundObjectOutput,
	which allows you to use the sounds you make in that same way that you would
	use a sound object, including saving them to a file. */
	SoundObjectOutput soOut;

	ampEnv >> soOut;
	soOut.setup(44100);

	ampEnv.attack(); //Start by priming the ampEnv so that sound comes out of it.
	soOut.sampleData(1); //Sample 1 second worth of data at the given sample rate.

	ampEnv.release(); //Now relase the ampEnv
	soOut.sampleData(.5); //And sample an additional 1/2 second of data.

	//Now that you're done sampling, you can use the sound object that you made!
	soOut.so; // <-- This is the sound object. See the soundObject example for to see how to use it in detail.
	soOut.so.normalize(); //Its a good idea to normalize before saving to a file to get the levels up.
	soOut.so.writeToFile("Short sample.wav"); //You can save it to file, like in this example, or play it using a CX_SoundObjectPlayer.



	//Now that we're done hijacking the ampEnv output, let's route it back into the sound output.
	ampEnv >> output;


	

	drawInformation();

	while (1) {
		if (Input.pollEvents()) {
			while (Input.Mouse.availableEvents()) {
				CX_Mouse::Event ev = Input.Mouse.getNextEvent();
				if (ev.eventType == CX_Mouse::Event::MOVED || ev.eventType == CX_Mouse::Event::DRAGGED) {
					mainOsc.frequency = ev.x * 8;
					//doublingOsc.frequency = mainOsc.frequency + 2;
					lfoOffset.amount = mainOsc.frequency; //We don't set the frequency of the doubling osc directly,
						//instead we set the offset for the lfo that feeds into the frequency of the doubling osc.

					cout << "F = " << mainOsc.frequency.getValue() << endl;

					mainOscGain.amount = (pow(Display.getResolution().y - ev.y, 1.5)) / (Display.getResolution().y * 10);
					doublingOscGain.amount = mainOscGain.amount;
					cout << "A = " << mainOscGain.amount.getValue() << endl;
					
					/*
					float smax = 0;
					for (int i = 0; i < 10; i++) {
						float t = a.getNextSample();
						if (t > smax) {
							smax = t;
						}
					}
					cout << "smax = " << smax << endl;
					*/
				}

				if (ev.eventType == CX_Mouse::Event::PRESSED) {
					ampEnv.attack();
					modEnv.attack();
				}

				if (ev.eventType == CX_Mouse::Event::RELEASED) {
					ampEnv.release();
					modEnv.release();
				}
			}

			while (Input.Keyboard.availableEvents()) {
				CX_Keyboard::Event ev = Input.Keyboard.getNextEvent();

				ss.hasSwappedSinceLastCheck();
				while (!ss.hasSwappedSinceLastCheck())
					;

				switch (ev.key) {
				case 't': 
					mainOsc.setGeneratorFunction(Oscillator::triangle);
					doublingOsc.setGeneratorFunction(Oscillator::triangle);
					break;
				case 'q': 
					mainOsc.setGeneratorFunction(Oscillator::square); 
					doublingOsc.setGeneratorFunction(Oscillator::square);
					break;
				case 'i': 
					mainOsc.setGeneratorFunction(Oscillator::sine); 
					doublingOsc.setGeneratorFunction(Oscillator::sine);
					break;
				case 'a': 
					mainOsc.setGeneratorFunction(Oscillator::saw); 
					doublingOsc.setGeneratorFunction(Oscillator::saw);
					break;
				case 'w': 
					mainOsc.setGeneratorFunction(Oscillator::whiteNoise);
					doublingOsc.setGeneratorFunction(Oscillator::whiteNoise);
					break;
				}
			}

			drawInformation();
		}

	}
}

void drawInformation(void) {
	Display.beginDrawingToBackBuffer();
	ofBackground(50);
	ofSetColor(255);

	ofDrawBitmapString("Click to trigger the attack, unclick to trigger the release", Display.getCenterOfDisplay() + ofPoint(0, 30));

	ofDrawBitmapString("Low frequency", Display.getCenterOfDisplay() + ofPoint(-230, 0));
	ofDrawBitmapString("High frequency", Display.getCenterOfDisplay() + ofPoint(170, 0));
	ofDrawBitmapString("Low volume", Display.getCenterOfDisplay() + ofPoint(-30, 200));
	ofDrawBitmapString("High volume", Display.getCenterOfDisplay() + ofPoint(-30, -200));

	ofDrawBitmapString("Key: Waveform\nt: triangle\nq: square\ni: sine\na: saw\nw: white noise", Display.getCenterOfDisplay());

	Display.endDrawingToBackBuffer();
	Display.BLOCKING_swapFrontAndBackBuffers();
}

