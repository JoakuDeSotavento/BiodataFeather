export type MidiVoiceEvent =
  | { kind: "noteOn"; channel: number; note: number; velocity: number }
  | { kind: "noteOff"; channel: number; note: number }
  | { kind: "controlChange"; channel: number; controller: number; value: number };

const STATUS_NOTE_OFF = 0x80;
const STATUS_NOTE_ON = 0x90;
const STATUS_CC = 0xb0;

function pushParsed(
  out: MidiVoiceEvent[],
  status: number,
  data1: number,
  data2: number
): void {
  const kind = status & 0xf0;
  const channel = (status & 0x0f) + 1;

  if (kind === STATUS_NOTE_ON) {
    if (data2 === 0) {
      out.push({ kind: "noteOff", channel, note: data1 });
    } else {
      out.push({ kind: "noteOn", channel, note: data1, velocity: data2 });
    }
    return;
  }
  if (kind === STATUS_NOTE_OFF) {
    out.push({ kind: "noteOff", channel, note: data1 });
    return;
  }
  if (kind === STATUS_CC) {
    out.push({ kind: "controlChange", channel, controller: data1, value: data2 });
  }
}

/**
 * Paquetes BLE MIDI del Biodata: 0x80, 0x80, status, data1, data2 (y posibles mensajes extra concatenados).
 * También acepta un mensaje MIDI clásico de 3 bytes sin cabecera BLE.
 */
export function parseBleMidiPacket(data: DataView): MidiVoiceEvent[] {
  const out: MidiVoiceEvent[] = [];
  const u8 = new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
  if (u8.length === 0) return out;

  if (u8.length >= 5 && u8[0] === 0x80 && u8[1] === 0x80) {
    let o = 2;
    while (o + 2 < u8.length) {
      const st = u8[o];
      if ((st & 0x80) === 0) break;
      pushParsed(out, st, u8[o + 1] & 0x7f, u8[o + 2] & 0x7f);
      o += 3;
    }
    return out;
  }

  if (u8.length >= 3 && (u8[0] & 0x80) !== 0) {
    pushParsed(out, u8[0], u8[1] & 0x7f, u8[2] & 0x7f);
  }
  return out;
}
