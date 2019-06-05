#include "CX.h"

CX_SoundStream ss;
CX_SoundBufferRecorder recorder;
CX_SoundBufferPlayer player;

void processKeypress(int key);
void drawDisplay(void);

void runExperiment(void) {

	RNG.setSeed("abcd");

	Input.setup(true, false);

	// Configure the sound stream for sound input and output.
	CX_SoundStream::Configuration ssc;

	ssc.api = RtAudio::Api::WINDOWS_DS;

	// Mono in and out using the default input and output devices.
	ssc.inputChannels = 1;
	ssc.outputChannels = 1;

	ssc.sampleRate = 48000;
	
	// Set up the sound stream with the sound stream configuration
	bool setupSuccess = true;
	setupSuccess = setupSuccess && ss.setup(ssc);

	// Set up recorder and player with the sound stream
	setupSuccess = setupSuccess && recorder.setup(&ss);
	setupSuccess = setupSuccess && player.setup(&ss);

	

	if (!setupSuccess) {
		Log.error() << "Error while setting up sound.";
		Input.Keyboard.waitForKeypress(-1);
	}

	// Have the recorder create an internal CX_SoundBuffer to record to
	recorder.createNewSoundBuffer();


	Log.flush();

	while (true) {

		drawDisplay();

		if (Input.pollEvents()) {
			while (Input.Keyboard.availableEvents() > 0) {
				CX_Keyboard::Event ev = Input.Keyboard.getNextEvent();
				if (ev.type == CX_Keyboard::Pressed) {
					processKeypress(ev.key);
				}
			}
		}

		Log.flush();

		Clock.sleep(0);

	}

}

void processKeypress(int key) {
	if (key == 'R') {
		// record
		recorder.record(false);
	}
	else if (key == 'S') {
		// stop recording
		recorder.stop();

		// and give the player the sound buffer
		player.setSoundBuffer(recorder.getSoundBuffer());
	}
	else if (key == 'C') {
		// clear recording
		recorder.clear();
	}
	else if (key == ' ') {
		// play/pause player
		if (player.isPlaying()) {
			player.stop();
		}
		else {
			player.play(false); // Don't restart
		}
	}
	else if (key == CX::Keycode::BACKSPACE) {
		// rewind player
		player.seek(0);
	}
	else if (key == 'Q') {
		player.queuePlayback(Clock.now() + CX_Seconds(2), CX_Millis(100));
	}
}

void drawDisplay(void) {

	string commands = "-- Recorder --\nR: Record\nS: Stop\nC: Clear\n\n-- Player --\nSpace: Play/pause\nBackspace: Rewind\nQ: Queue playback (2 seconds)";

	string recStat = "Recorder status: ";
	if (recorder.isRecording()) {
		recStat += "Recording";
	}
	else {
		recStat += "Stopped";
	}
	recStat += "\nLen: " + ofToString(recorder.getRecordingLength().seconds());



	string playStat = "Player status: ";
	if (player.isPlaying()) {
		playStat += "Playing";
	}
	else if (player.isPlaybackQueued()) {
		playStat += "Queued";
	}
	else {
		playStat += "Stopped";
	}
	playStat += "\nLen: " + ofToString(player.getPlaybackTime().seconds());



	Disp.beginDrawingToBackBuffer();

	ofBackground(0);

	ofSetColor(255);

	ofDrawBitmapString(commands, 20, 20);

	ofDrawBitmapString(recStat, 250, 20);

	ofDrawBitmapString(playStat, 250, 50);

	Disp.endDrawingToBackBuffer();
	Disp.swapBuffers();
}