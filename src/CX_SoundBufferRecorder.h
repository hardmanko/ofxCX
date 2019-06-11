#pragma once

#include "CX_SoundStream.h"
#include "CX_SoundBuffer.h"
#include "CX_ThreadUtils.h"

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

		CX_SoundBufferRecorder(void);
		~CX_SoundBufferRecorder(void);

		// 1. Set up the recorder (choose one)
		bool setup(CX_SoundStream* ss);
		bool setup(std::shared_ptr<CX_SoundStream> ss);
		//bool shutdown(void);

		// 2. Set up a buffer to record to (choose one)
		void createNewSoundBuffer(void);
		void resetSoundBuffer(bool createBufferIfNeeded = true);
		bool setSoundBuffer(std::shared_ptr<CX_SoundBuffer> buffer);
		bool setSoundBuffer(CX_SoundBuffer* buffer);


		// 3. Record or queue recording
		bool record(bool clear = false);
		bool queueRecording(CX_Millis startTime, CX_Millis timeout, bool clear = false);
		bool queueRecording(SampleFrame sampleFrame, bool clear = false);

		// (Optional) Check recording status
		bool isRecording(void);
		bool isRecordingQueued(void);
		bool isRecordingOrQueued(void);


		// 4. Stop recording
		void stop(void);
		bool setAutoStopLength(CX_Millis recordingLength);

		void pause(void);

		


		// 5. Get the recorded sound buffer and recording metadata
		bool isRecordingComplete(void);
		std::shared_ptr<CX_SoundBuffer> getSoundBuffer(void);
		CX_Millis getRecordingStartTime(void);
		CX_Millis getRecordingEndTime(void); // remove
		CX_Millis getRecordingLength(void); // remove


		// Misc functions
		unsigned int getOverflowsSinceLastCheck(bool logOverflows = true);

		std::shared_ptr<CX_SoundStream> getSoundStream(void);

	private:

		enum class RecordingState : int {
			Ready,
			Recording,
			PreparingToRecord,
			RecordingQueued,
			RecordingComplete
		};

		struct InputEventData : public std::recursive_mutex {

			InputEventData(void) :
				//state(RecordingState::Ready),
				recording(false),
				startingRecording(false),
				recordingQueued(false),
				queuedRecordingStartSampleFrame(std::numeric_limits<SampleFrame>::max()),
				overflowCount(0)
			{}

			//RecordingState state;

			bool recording;
			bool startingRecording;			
			bool recordingQueued;
			SampleFrame queuedRecordingStartSampleFrame;

			std::shared_ptr<CX_SoundBuffer> buffer;

			CX_Millis recordingStart;
			CX_Millis recordingEnd;

			unsigned int overflowCount;
			
		} _inData;


		void _inputEventHandler(const CX_SoundStream::InputEventArgs& inputData);
		CX::Util::ofEventHelper<const CX_SoundStream::InputEventArgs&> _inputEventHelper;

		std::shared_ptr<CX_SoundStream> _soundStream;

		void _cleanUpOldSoundStream(void);


		void _prepareRecordBuffer(bool clear, std::string callingFunctionName);
	};

}