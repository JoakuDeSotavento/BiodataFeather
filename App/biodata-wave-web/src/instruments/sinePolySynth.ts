import type { MidiVoiceEvent } from "../bleMidiParser";
import type { MidiInstrument } from "./types";

function midiToHz(note: number): number {
  return 440 * Math.pow(2, (note - 69) / 12);
}

type Voice = {
  note: number;
  osc: OscillatorNode;
  gain: GainNode;
};

/** Polifonía simple: una onda sinusoidal por nota, ganancia según velocity MIDI. */
export class SinePolySynth implements MidiInstrument {
  private readonly ctx: AudioContext;
  private readonly master: GainNode;
  private readonly voices: Voice[] = [];
  private readonly maxVoices = 8;
  private readonly attackSec = 0.02;
  private readonly releaseSec = 0.22;
  private started = false;

  constructor(ctx: AudioContext) {
    this.ctx = ctx;
    this.master = ctx.createGain();
    this.master.gain.value = 0.28;
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
        v.osc.stop();
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
    g.setValueAtTime(Math.max(g.value, 0.0001), when);
    g.exponentialRampToValueAtTime(0.0001, when + this.releaseSec);
    try {
      v.osc.stop(when + this.releaseSec + 0.03);
    } catch {
      /* ignore */
    }
    window.setTimeout(() => {
      const i = this.voices.indexOf(v);
      if (i >= 0) this.voices.splice(i, 1);
    }, (this.releaseSec + 0.08) * 1000);
  }

  noteOn(note: number, velocity: number, when?: number): void {
    const t = when ?? this.ctx.currentTime;
    const vel = Math.max(0.001, velocity / 127);
    const existing = this.voices.findIndex((v) => v.note === note);
    if (existing >= 0) {
      this.releaseVoiceAt(existing, t);
    }
    const slot = this.stealVoiceIndex();
    if (slot >= 0) {
      this.releaseVoiceAt(slot, t);
    }

    const osc = this.ctx.createOscillator();
    osc.type = "sine";
    osc.frequency.setValueAtTime(midiToHz(note), t);

    const gain = this.ctx.createGain();
    const peak = 0.14 * Math.pow(vel, 0.85);
    gain.gain.setValueAtTime(0.0001, t);
    gain.gain.exponentialRampToValueAtTime(peak, t + this.attackSec);

    osc.connect(gain);
    gain.connect(this.master);

    osc.start(t);

    this.voices.push({
      note,
      osc,
      gain,
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
