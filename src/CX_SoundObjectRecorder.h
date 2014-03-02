#pragma once

#include "CX_SoundStream.h"
#include "CX_SoundObject.h"

namespace CX {
	/*! This class is used for recording audio data from, e.g., a microphone. The recorded data is
	stored in a CX_SoundObject for further use. 
	
	\code{.cpp}
	CX_SoundObjectRecorder recorder;

	CX_SoundObjectRecorder::Configuration recorderConfig;
	recorderConfig.inputChannels = 1; //You will probably need to configure more than just this
	recorder.setup(recorderConfig);

	CX_SoundObject recording;
	recorder.setSoundObject(&recording); //Associate a CX_SoundObject with the recorder so that it can be recorded to.

	//Record for 5 seconds
	recorder.startRecording();
	ofSleepMillis(5000);
	recorder.stopRecording();

	//Write the recording to a file
	recording.writeToFile("recording.wav");
	\endcode
	\ingroup sound
	*/
	class CX_SoundObjectRecorder {
	public:
		typedef CX_SoundStream::Configuration Configuration; //!< This is typedef'ed to \ref CX::CX_SoundStream::Configuration.

		CX_SoundObjectRecorder(void);
		~CX_SoundObjectRecorder(void);

		bool setup(CX_SoundObjectRecorder::Configuration& config);

		void setSoundObject(CX_SoundObject* so);
		CX_SoundObject* getSoundObject(void);

		void startRecording(bool clearExistingData = false);
		void stopRecording(void);

	private:
		bool _inputEventHandler(CX_SoundStream::InputEventArgs& inputData);

		bool _recording;

		CX_SoundObject *_soundObject;
		CX_SoundStream _soundStream;
	};

}