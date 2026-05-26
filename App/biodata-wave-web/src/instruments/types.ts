import type { MidiVoiceEvent } from "../bleMidiParser";

/** Motor de audio que reacciona a mensajes MIDI de voz (note on/off, CC opcional). */
export interface MidiInstrument {
  readonly running: boolean;
  resume(): Promise<void>;
  dispose(): void;
  handleMidi(events: MidiVoiceEvent[]): void;
}
