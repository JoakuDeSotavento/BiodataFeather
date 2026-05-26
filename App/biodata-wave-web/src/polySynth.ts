import type { MidiVoiceEvent } from "./bleMidiParser";
import type { MidiInstrument } from "./instruments/types";

function midiToHz(note: number): number {
  return 440 * Math.pow(2, (note - 69) / 12);
}

type Voice = {
  note: number;
  oscA: OscillatorNode;
  oscB: OscillatorNode;
  gain: GainNode;
  filter: BiquadFilterNode;
  stopAt: number | null;
};

export class PolyPadSynth implements MidiInstrument {
  private readonly ctx: AudioContext;
  private readonly master: GainNode;
  private readonly voices: Voice[] = [];
  private readonly maxVoices = 8;
  private started = false;

  constructor(ctx: AudioContext) {
    this.ctx = ctx;
    this.master = ctx.createGain();
    this.master.gain.value = 0.35;
    this.master.connect(ctx.destination);
  }

  async resume(): Promise<void> {
    if (this.ctx.state === "suspended") {
      await this.ctx.resume();
    }
    this.started = true;
  }

  get running(): boolean {
    return this.started;
  }

  dispose(): void {
    for (const v of this.voices) {
      try {
        v.oscA.stop();
        v.oscB.stop();
      } catch {
        /* ignore */
      }
    }
    this.voices.length = 0;
    this.master.disconnect();
  }

  private stealVoiceIndex(): number {
    if (this.voices.length < this.maxVoices) return -1;
    let oldest = 0;
    let oldestNote = this.voices[0].note;
    for (let i = 1; i < this.voices.length; i++) {
      if (this.voices[i].note < oldestNote) {
        oldestNote = this.voices[i].note;
        oldest = i;
      }
    }
    return oldest;
  }

  private releaseVoiceAt(index: number, when: number): void {
    const v = this.voices[index];
    const g = v.gain.gain;
    g.cancelScheduledValues(when);
    g.setValueAtTime(g.value, when);
    g.exponentialRampToValueAtTime(0.0001, when + 0.45);
    v.stopAt = when + 0.46;
    try {
      v.oscA.stop(when + 0.47);
      v.oscB.stop(when + 0.47);
    } catch {
      /* ignore */
    }
    window.setTimeout(() => {
      const i = this.voices.indexOf(v);
      if (i >= 0) this.voices.splice(i, 1);
    }, 500);
  }

  noteOn(note: number, velocity: number, when?: number): void {
    const t = when ?? this.ctx.currentTime;
    const vel = Math.max(0.001, velocity / 127);
    const existing = this.voices.findIndex((v) => v.note === note);
    if (existing >= 0) {
      this.releaseVoiceAt(existing, t);
    }
    let slot = this.stealVoiceIndex();
    if (slot >= 0) {
      this.releaseVoiceAt(slot, t);
    }

    const oscA = this.ctx.createOscillator();
    const oscB = this.ctx.createOscillator();
    oscA.type = "triangle";
    oscB.type = "sine";
    const detune = 7;
    oscA.frequency.setValueAtTime(midiToHz(note), t);
    oscB.frequency.setValueAtTime(midiToHz(note + detune / 100), t);
    oscA.detune.setValueAtTime(-detune, t);
    oscB.detune.setValueAtTime(detune, t);

    const filter = this.ctx.createBiquadFilter();
    filter.type = "lowpass";
    filter.frequency.setValueAtTime(2200 + vel * 1800, t);
    filter.Q.setValueAtTime(0.7, t);

    const gain = this.ctx.createGain();
    gain.gain.setValueAtTime(0.0001, t);
    gain.gain.exponentialRampToValueAtTime(0.12 * vel, t + 0.04);
    gain.gain.exponentialRampToValueAtTime(0.08 * vel, t + 0.35);

    oscA.connect(filter);
    oscB.connect(filter);
    filter.connect(gain);
    gain.connect(this.master);

    oscA.start(t);
    oscB.start(t);

    this.voices.push({
      note,
      oscA,
      oscB,
      gain,
      filter,
      stopAt: null,
    });
  }

  noteOff(note: number, when?: number): void {
    const t = when ?? this.ctx.currentTime;
    const idx = this.voices.findIndex((v) => v.note === note);
    if (idx < 0) return;
    this.releaseVoiceAt(idx, t);
  }

  handleMidi(events: MidiVoiceEvent[]): void {
    const t = this.ctx.currentTime;
    for (const ev of events) {
      if (ev.kind === "noteOn") {
        this.noteOn(ev.note, ev.velocity, t);
      } else if (ev.kind === "noteOff") {
        this.noteOff(ev.note, t);
      }
    }
  }
}
