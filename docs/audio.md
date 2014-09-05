Audio Input and Output {#audioIO}
======================

Audio input and output in CX is based on a number of classes. The two most important are CX_SoundStream and CX_SoundBuffer. 
Additionally, CX_SoundBufferPlayer and CX_SoundBufferRecorder combine together a CX_SoundStream and a CX_SoundBuffer to play back or record audio, respectively.
Finally, for people wanting to synthesize audio in real time (or ahead of time), the CX::Synth namespace provides a multitude of ways to synthesize audio.
We will go through these components in order of importance.

TODO: Keep going and complete this.