# Android Audio Recorder
An android audio recorder has been added. As well as starting and 
stopping recording, it allows the selection of various input
sources, encoding types and output formats. The library places 
the following handlers in the message path:
- `androidRecorderStartRecording pFileName`: Start recording an audio file, using the given filename
- `androidRecorderStopRecording`: Stop recording
- `androidRecorderGetMaxAmplitude`: Returns the max amplitude of the recording since last sampled
- `androidRecorderSetRecordCompressionType pType`: Set the record compression type
- `androidRecorderSetRecordFormat pFormat`: Set the record output format
- `androidRecorderSetRecordInput pSource`: Set the record input source

