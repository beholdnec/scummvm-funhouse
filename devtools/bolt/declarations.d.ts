// TypeScript declarations for WebAudio

interface AudioContextConstructor {
  new(): AudioContext;
}

interface Window {
  AudioContext: AudioContextConstructor;
}
