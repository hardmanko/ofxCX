#pragma once

#include "CX_InputManager.h"
#include "CX_SoundBuffer.h"
#include "CX_SoundBufferPlayer.h"
#include "CX_SlideBuffer.h"

#include "CX_SynchronizationUtils.h"

namespace CX {

	class CX_AVP {
	public:

		enum class Mode {
			RenderMain_SwapMain,
			RenderMain_SwapThread,
			RenderThread_SwapThread,
			RM_SM_Helped,
			RM_ST_Helped,
			RT_ST_Helped
		};

		struct SyncConfig {
			CX_Millis requiredSwapDuration; // amount of time swapping must go on for

			double displayTolerance;
			double soundTolerance;

			CX_Millis readyTimeout;
		};


		struct Configuration {
			Mode mode;

			CX_Display* display;
			CX_SoundStream* soundStream;

			SyncConfig sync;

			unsigned int displayExtraLeadFrames;
			CX_Millis audioLatencyOffset;

			bool deallocateCompletedSlides;

			bool releaseRenderingContext; // automatically. how will you implement this?

			CX_Millis preSwapSafetyBuffer;
		};

		bool setup(const Configuration& config);

		bool play(void);

		bool startPlaying(void);
		bool isPlaying(void);
		void updatePlayback(void); // only if Mode::SingleThreadedDisplay
		bool stopPlaying(void);

		

		bool releaseRenderingContext(void);

		CX_SoundBuffer sounds;
		CX_SlideBuffer slides;


		CX_SoundBufferPlayer* getSoundBufferPlayer(void);

	private:

		std::recursive_mutex _mutex;
		Configuration _config;
		CX_DisplayThread* _displayThread;
		std::unique_ptr<Sync::DataContainer::PolledSwapListener> _dispSwapListener;

		CX_SoundBufferPlayer _soundPlayer;
		CX_SlideBufferPlaybackHelper _slideHelper;
		CX_SlideBufferPredicatePlayback _slidePP;

		void _configureDomainSync(void);
		Sync::DomainSynchronizer _domainSync;
		Sync::DataClient _dispClient;
		Sync::DataClient _ssClient;

	
		// Mode::RenderMain_SwapMain
		bool RMSM_startPlaying(void);
		bool RMSM_isPlaying(void);
		void RMSM_updatePlayback(void);
		bool RMSM_stopPlaying(void);

		// Mode::RenderMain_SwapThread
		bool RMST_startPlaying(void);
		bool RMST_isPlaying(void);
		void RMST_updatePlayback(void);
		bool RMST_stopPlaying(void);

		// Mode::RenderThread_SwapThread
		bool RTST_startPlaying(void);
		bool RTST_isPlaying(void);
		void RTST_updatePlayback(void);
		bool RTST_stopPlaying(void);
		void RTST_queuedFrameCompleteCallback(CX_DisplayThread::QueuedFrameResult&& qfr);

		void _RM_postSwapCheck(void);
		Sync::SyncPoint _All_getDesiredStartSyncPoint(void);
		struct DesiredStart {
			bool valid;
			CX_Millis time;
			FrameNumber frameNumber;
			SampleFrame sampleFrame;
		};
		DesiredStart _All_getDesiredStart(void);


		Util::DisplaySwapper _dispSwapper;

		Util::ofEventHelper<void> _threadUpdateEventHelper;

		bool SM_swapDisplayUntilReady(void);

		bool FrameCounted_renderNextPredicate(const CX_SlideBufferPredicatePlayback::PredicateArgs& args);
		bool Timed_renderNextPredicate(const CX_SlideBufferPredicatePlayback::PredicateArgs& args);

		bool SM_shouldSwapPredicate(void);
		bool ST_hasSwappedPredicate(void);
		//void ST_something(CX_Millis desiredTime, CX_Millis timeout);



		// Mode::RM_SM_Helped
		bool RMSMH_setup(void);
		bool RMSMH_startPlaying(void);
		bool RMSMH_isPlaying(void);
		void RMSMH_updatePlayback(void);
		bool RMSMH_stopPlaying(void);

		// RM_ST_Helped
		bool RMSTH_setup(void);
		bool RMSTH_startPlaying(void);
		bool RMSTH_isPlaying(void);
		void RMSTH_updatePlayback(void);
		bool RMSTH_stopPlaying(void);

		// RT_ST_Helped
		bool RTSTH_setup(void);
		bool RTSTH_startPlaying(void);
		bool RTSTH_isPlaying(void);
		void RTSTH_updatePlayback(void);
		bool RTSTH_stopPlaying(void);

	};

}