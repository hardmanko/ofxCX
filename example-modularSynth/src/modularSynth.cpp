#include "CX_EntryPoint.h"

#include "CX_ModularSynth.h"

CX_SoundStream ss;


void drawInformation(void);


void runExperiment(void) {
	CX_SoundStreamConfiguration_t config;
	config.api = RtAudio::Api::WINDOWS_DS;
	config.outputChannels = 2;
	config.sampleRate = 48000;
	config.bufferSize = 256;
	config.streamOptions.numberOfBuffers = 4;
	ss.setup(config);


	Oscillator osc;
	osc.frequency = 2000;
	osc.setGeneratorFunction(Noisemaker::sine);

	RCFilter f;
	f.setBreakpoint(1600);

	Amplifier a;
	a.amplitude = .01;

	Envelope en;
	en.a = 1;
	en.d = 2;
	en.s = .5;
	en.r = 1;
	
	SoundOut output;
	output.setOuputStream(ss);

	f.setInput(&osc);
	a.setInput(&f);
	en.setInput(&a);
	output.setInput(&en);
	
	ss.start();

	Input.setup(true, true);

	drawInformation();

	while (1) {
		if (Input.pollEvents()) {
			while (Input.Mouse.availableEvents()) {
				CX_MouseEvent_t ev = Input.Mouse.getNextEvent();
				if (ev.eventType == CX_MouseEvent_t::MOVED || ev.eventType == CX_MouseEvent_t::DRAGGED) {
					osc.frequency = ev.x * 8;
					cout << "F = " << osc.frequency << endl;

					a.amplitude = (pow(Display.getResolution().y - ev.y, 1.5)) / (Display.getResolution().y * 10);
					cout << "A = " << a.amplitude << endl;
					
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
					en.gate();
				}

				if (ev.eventType == CX_MouseEvent_t::RELEASED) {
					en.release();
				}
			}

			while (Input.Keyboard.availableEvents()) {
				CX_KeyEvent_t ev = Input.Keyboard.getNextEvent();

				ss.hasSwappedSinceLastCheck();
				while (!ss.hasSwappedSinceLastCheck())
					;

				switch (ev.key) {
				case 't': osc.setGeneratorFunction(Noisemaker::triangle); break;
				case 'q': osc.setGeneratorFunction(Noisemaker::square); break;
				case 'i': osc.setGeneratorFunction(Noisemaker::sine); break;
				case 'w': osc.setGeneratorFunction(Noisemaker::saw); break;
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

	ofDrawBitmapString("Key: Waveform\nt: triangle\nq: square\ni: sine\nw: saw", Display.getCenterOfDisplay());

	Display.endDrawingToBackBuffer();
	Display.BLOCKING_swapFrontAndBackBuffers();
}