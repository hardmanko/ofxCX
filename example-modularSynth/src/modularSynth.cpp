#include "CX_EntryPoint.h"

#include "CX_ModularSynth.h"

using namespace CX::Synth;

CX_SoundStream ss;




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

The waveform generating function should return a double representing the emplitude of the
wave at the given time point.

To put this all together, a sine wave generator looks like this: */
double sineWaveGeneratorFunction(double waveformPosition) {
	return sin(2 * PI * waveformPosition); //The argument for sin is in radians. 1 cycle is 2*PI radians.
}

void runExperiment(void) {

	simpleTest();

	CX_SoundStreamConfiguration_t config;
	config.api = RtAudio::Api::WINDOWS_DS;
	config.outputChannels = 2;
	config.sampleRate = 48000;
	config.bufferSize = 256;
	config.streamOptions.numberOfBuffers = 4;
	ss.setup(config);

	StreamOutput output;
	output.setOuputStream(ss);


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
	RCFilter filter;

	Envelope modEnv;
	modEnv.a = .1;
	modEnv.d = .1;
	modEnv.s = .5;
	modEnv.r = .2;

	Multiplier modMult;
	modMult.amount = 3000;

	Adder modOffset;
	modOffset.amount = 100;

	modEnv >> modMult >> modOffset >> filter.breakpoint;
	
	Envelope ampEnv;
	ampEnv.a = .3;
	ampEnv.d = .2;
	ampEnv.s = .6;
	ampEnv.r = .2;



	//After the mixer, filter, attach the amp envelope, and route into the output.
	mainOsc >> filter >> ampEnv >> output;


	FIRFilter fir;
	fir.setup(FIRFilter::LOW_PASS, 21);
	
	ModuleControlData_t dat;
	dat.sampleRate = 1000;
	fir.setData(dat);
	fir.setCutoff(125);


	/*
	RecursiveFilter rec;
	rec.setBreakpoint(1000);
	rec.setBandwidth(100);
	rec.setup(RecursiveFilter::FilterType::BAND_PASS);

	RCFilter rc;
	rc.breakpoint = 1000;

	SoundObjectOutput filterOut;
	filterOut.setup(44100);

	mainOsc.frequency = 500;
	mainOsc.setGeneratorFunction(Oscillator::square);

	mainOsc >> rec >> filterOut;
	filterOut.sampleData(2);
	filterOut.so.normalize();

	rec.setBandwidth(300);
	rec.setup(RecursiveFilter::FilterType::BAND_PASS);
	filterOut.sampleData(2);

	//mainOsc >> rc >> filterOut;
	//filterOut.sampleData(3);

	filterOut.so.writeToFile("Recursive types.wav");
	*/


	AdditiveSynth as;
	as.setHarmonicSeries(101, AdditiveSynth::HarmonicSeriesType::HS_MULTIPLE, 1);
	as.setAmplitudes(AdditiveSynth::HarmonicAmplitudeType::SAW);
	as.setFundamentalFrequency(300);
	as.pruneLowAmplitudeHarmonics(0.05);

	Splitter ts;
	Multiplier lm;
	Multiplier rm;

	lm.amount = 0.1;
	rm.amount = 0.01;

	as >> ts >> lm;
	ts >> rm;

	StereoSoundObjectOutput stereo;
	stereo.setup(48000);

	lm >> stereo.left;
	rm >> stereo.right;

	stereo.sampleData(3);

	lm.amount = .01;
	rm.amount = .1;

	stereo.sampleData(3);

	stereo.so.writeToFile("stereo.wav");


	SoundObjectOutput asOut;
	asOut.setup(48000);

	Multiplier mm;
	mm.amount = .5;

	as >> mm >> asOut;

	asOut.sampleData(1);

	cout << "Peaks: " << asOut.so.getPositivePeak() << " " << asOut.so.getNegativePeak() << endl;

	asOut.so.normalize();
	asOut.so.writeToFile("add synth.wav");



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
	
	ss.start();

	Input.setup(true, true);

	drawInformation();

	while (1) {
		if (Input.pollEvents()) {
			while (Input.Mouse.availableEvents()) {
				CX_MouseEvent_t ev = Input.Mouse.getNextEvent();
				if (ev.eventType == CX_MouseEvent_t::MOVED || ev.eventType == CX_MouseEvent_t::DRAGGED) {
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

				if (ev.eventType == CX_MouseEvent_t::PRESSED) {
					ampEnv.attack();
					modEnv.attack();
				}

				if (ev.eventType == CX_MouseEvent_t::RELEASED) {
					ampEnv.release();
					modEnv.release();
				}
			}

			while (Input.Keyboard.availableEvents()) {
				CX_KeyEvent_t ev = Input.Keyboard.getNextEvent();

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

	ofDrawBitmapString("Key: Waveform\nt: triangle\nq: square\ni: sine\na: saw\nw: white noise", Display.getCenterOfDisplay());

	Display.endDrawingToBackBuffer();
	Display.BLOCKING_swapFrontAndBackBuffers();
}



/*
osc >> f >> a >> soOut;

soOut.setup(44100);

osc.setGeneratorFunction(Oscillator::sine);
osc.frequency = 1500;
f.setBreakpoint(10000);
a.amount = 1;
soOut.sampleData(.1);
a.amount = 0;
soOut.sampleData(.1);
a.amount = 1;
soOut.sampleData(.1);

soOut.so.normalize(1);
soOut.so.writeToFile("beep beep.wav");


soOut.so.clear();
osc.setGeneratorFunction(Oscillator::sine);
osc.frequency = 600;
f.setBreakpoint(10000);
a.amount = 1;
soOut.sampleData(.5);

soOut.so.normalize(1);
soOut.so.writeToFile("beep.wav");
*/