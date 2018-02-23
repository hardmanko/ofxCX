#pragma once

#include "CX_SoundStream.h"
#include "CX_SoundBuffer.h"

namespace CX {
	/*! This class is used for recording audio data from, e.g., a microphone. The recorded data is
	stored in a CX_SoundBuffer for further use. 
	
	\code{.cpp}
	CX_SoundBufferRecorder recorder;

	CX_SoundBufferRecorder::Configuration recorderConfig;
	recorderConfig.inputChannels = 1;
	//You will probably need to configure more than just the number of input channels.
	recorder.setup(recorderConfig);

	CX_SoundBuffer recording;
	recorder.setSoundBuffer(&recording); //Associate a CX_SoundBuffer with the recorder so that the buffer can be recorded to.

	//Record for 5 seconds
	recorder.start();
	Clock.sleep(CX_Seconds(5));
	recorder.stop();

	//Write the recording to a file
	recording.writeToFile("recording.wav");
	\endcode

	\ingroup sound
	*/
	class CX_SoundBufferRecorder {
	public:
		typedef CX_SoundStream::Configuration Configuration; //!< This is typedef'ed to \ref CX::CX_SoundStream::Configuration.

		CX_SoundBufferRecorder(void);
		~CX_SoundBufferRecorder(void);

		// 1. Set up the recorder (choose one)
		bool setup(Configuration& config);
		bool setup(CX_SoundStream* ss);
		bool setup(std::shared_ptr<CX_SoundStream> ss);


		// 2. Set up a buffer to record to (choose one)
		void createNewSoundBuffer(void);
		bool setSoundBuffer(std::shared_ptr<CX_SoundBuffer> buffer);
		bool setSoundBuffer(CX_SoundBuffer* buffer);

		// 3. Record or queue recording
		bool record(bool clearExistingData = false);
		bool queueRecording(bool clear, CX_Millis startTime, CX_Millis latencyOffset = CX_Millis(0));

		// (Optional) Check recording status
		bool isRecording(void);
		bool isRecordingQueued(void);
		bool isRecordingOrQueued(void);

		// 4. Stop recording
		void stop(void);

		// 5. Get the recorded sound buffer and recording metadata
		std::shared_ptr<CX_SoundBuffer> getSoundBuffer(void);
		CX_Millis getRecordingStartTime(void);
		CX_Millis getRecordingEndTime(void);


		// Misc functions
		unsigned int getOverflowsSinceLastCheck(bool logOverflows = true);

		std::shared_ptr<CX_SoundStream> getSoundStream(void);
		const Configuration& getConfiguration(void) const;

	private:

		struct InputEventData : public std::recursive_mutex {

			InputEventData(void) :
				recording(false),
				startingRecording(false),
				overflowCount(0)
			{}

			bool recording;
			bool startingRecording;

			std::shared_ptr<CX_SoundBuffer> buffer;
			

			bool recordingQueued;
			uint64_t queuedRecordingStartSampleFrame;

			CX_Millis recordingStart;
			CX_Millis recordingEnd;

			unsigned int overflowCount;
			
		} _inData;

		void _inputEventHandler(CX_SoundStream::InputEventArgs& inputData);


		std::shared_ptr<CX_SoundStream> _soundStream;

		void _cleanUpOldSoundStream(void);

		void _listenForEvents(bool listen);
		bool _listeningForEvents;

		Configuration _defaultConfigReference;

		void _prepareRecordBuffer(bool clear, std::string callingFunctionName);
	};

	namespace Instances {
		/*! During CX initialization, `SoundRecorder` is configured to use 
		`CX::Instances::SoundStream` as its `CX_SoundStream`.
		This means that only `SoundStream` needs to be set up to use `SoundRecorder`.
		`SoundRecorder` does not need to have `setup()` called.

		\ingroup sound */
		extern CX_SoundBufferRecorder SoundRecorder;
	}

}