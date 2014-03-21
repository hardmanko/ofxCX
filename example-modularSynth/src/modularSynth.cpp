#include "CX_EntryPoint.h"

using namespace CX::Synth; //All of the modules for the modular synth are in this namespace.

/*! \file
This example shows some of the ways in which a modular synthesizer can be constructed using
modules provided in the CX::Synth namespace.
*/


void drawInformation(void);
void modularSynthInternals(void);


void runExperiment(void) {

	Input.setup(true, true);

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

	Multiplier m;
	m.setGain(-10); //Make the output quieter by 10 decibels

	osc >> m >> output; //operator>> means that osc feeds into m which then feeds into output.

	cout << "Let's listen to a saw wave for 3 seconds" << endl;
	//Clock.sleep(CX_Seconds(3));
	

	//Lets add a low pass filter to the chain.
	Filter lpf;
	lpf.setType(Filter::LOW_PASS);
	lpf.cutoff = 600; //Set the cutoff frequency of the filter to 600 Hz, so frequencies past there get attentuated.

	osc >> lpf >> m >> output; //Reconnect things so that the osc goes through the filter.

	cout << "Now a filtered saw" << endl;
	//Clock.sleep(CX_Seconds(3));

	//Lets add an envelope
	Envelope env;
	env.a = 0.5; //Set the attack time to .5 seconds (i.e. 500 ms)
	env.d = 0.5; //Decay time
	env.s = 0.7; //Sustain at .7 times the full amplitude
	env.r = 1.0; //Release time

	osc >> lpf >> env >> m >> output;
	env.attack();
	Clock.sleep(CX_Seconds(3));
	env.release();
	Clock.sleep(CX_Seconds(2));


	/* You can route the output from a modular synth into a SoundObjectOutput,
	which allows you to use the sounds you make in that same way that you would
	use a sound object, including saving them to a file. */
	SoundObjectOutput soOut;

	m >> soOut; //Without changin the other connections, route m into soOut, disconnecting it from output.
	soOut.setup(44100); //Use the same sample rate as the sound stream

	env.attack(); //Start by priming the evelope so that sound comes out of it.
	soOut.sampleData(CX_Seconds(2)); //Sample 1 second worth of data at the given sample rate.

	env.release(); //Now relase the evelope
	soOut.sampleData(CX_Seconds(1)); //And sample an additional 1/2 second of data.

	//Now that you're done sampling, you can use the sound object that you made!
	soOut.so; // <-- This is the sound object. See the soundObject example for to see how to use it in detail.
	soOut.so.normalize(); //Its a good idea to normalize before saving to a file to get the levels up.
	soOut.so.writeToFile("Envelope sample.wav"); //You can save it to file, like in this example, or play it using a CX_SoundObjectPlayer.



	//Now we make a relatively complex synthesizer, with two oscillators, an LFO
	//modifying the frequency of one of the oscillators, an envelope modifying the cutoff frequency
	//of the filter, and a mixer to combine together the oscillators.
	Mixer oscMix;

	Oscillator mainOsc;
	mainOsc.frequency = 1000;
	mainOsc.setGeneratorFunction(Oscillator::sine);

	Multiplier mainOscGain;
	mainOscGain.setGain(-10);

	mainOsc >> mainOscGain >> oscMix; //Run the main oscillator into the mixer.

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
	//it into a good frequency range. This offset will be changed along with the mainOsc frequency.
	lfo >> lfoGain >> lfoOffset >> doublingOsc.frequency;
	
	Multiplier doublingOscGain;
	doublingOscGain.setGain(-20);

	doublingOsc >> doublingOscGain >> oscMix; //Now run the doubliing oscillator into the mixer.


	//Create a filter and run a mod envelope into the filter cutoff frequency.
	Filter filter;
	filter.setType(Filter::LOW_PASS);

	Envelope modEnv;
	modEnv.a = .1;
	modEnv.d = .1;
	modEnv.s = .5;
	modEnv.r = .2;

	Multiplier modMult;
	modMult.amount = 1000;

	Adder modOffset;
	modOffset.amount = 100;

	modEnv >> modMult >> modOffset >> filter.cutoff;

	
	Envelope ampEnv;
	ampEnv.a = .3;
	ampEnv.d = .2;
	ampEnv.s = .6;
	ampEnv.r = .2;

	//After the mixer, filter to mixed data, attach the amp envelope, and route into the output.
	oscMix >> filter >> ampEnv >> output;

	drawInformation();

	while (true) {
		if (Input.pollEvents()) {
			while (Input.Mouse.availableEvents()) {
				CX_Mouse::Event ev = Input.Mouse.getNextEvent();
				if (ev.eventType == CX_Mouse::Event::MOVED || ev.eventType == CX_Mouse::Event::DRAGGED) {
					mainOsc.frequency = ev.x * 8;
					//doublingOsc.frequency = mainOsc.frequency + 2;
					lfoOffset.amount = mainOsc.frequency; //We don't set the frequency of the doubling osc directly,
						//instead we set the offset for the lfo that feeds into the frequency of the doubling osc.

					cout << "F = " << mainOsc.frequency.getValue() << endl;

					double gain = -ev.y / 20;
					mainOscGain.setGain(gain);
					doublingOscGain.setGain(gain);

					//mainOscGain.amount = (pow(Display.getResolution().y - ev.y, 1.5)) / (Display.getResolution().y * 40);
					//doublingOscGain.amount = mainOscGain.amount;
					cout << "A = " << mainOscGain.amount.getValue() << endl;
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

	ofDrawBitmapString("Low frequency", Display.getCenterOfDisplay() + ofPoint(-230, 0));
	ofDrawBitmapString("High frequency", Display.getCenterOfDisplay() + ofPoint(170, 0));
	ofDrawBitmapString("Low volume", Display.getCenterOfDisplay() + ofPoint(-30, 200));
	ofDrawBitmapString("High volume", Display.getCenterOfDisplay() + ofPoint(-30, -200));

	ofDrawBitmapString("Click to trigger the attack,\nunclick to trigger the release\n\n"
					   "Move the mouse to change amplitude and frequency.\n\n"
					   "Press keys to change the waveform.\n"
					   "Key: Waveform\nt: triangle\nq: square\ni: sine\na: saw\nw: white noise", Display.getCenterOfDisplay() + ofPoint(-50,-50));

	Display.endDrawingToBackBuffer();
	Display.BLOCKING_swapFrontAndBackBuffers();
}

//This function shows a part of how the modules work internally, based on a sample-by-sample model.
void modularSynthInternals(void) {
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