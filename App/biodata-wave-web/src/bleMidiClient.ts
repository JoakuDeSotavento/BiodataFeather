/** UUIDs del perfil MIDI over BLE (mismo servicio que el firmware Biodata). */
export const BLE_MIDI_SERVICE = "03b80e5a-ede8-4b33-a751-6ce34ec4c700";
export const BLE_MIDI_CHARACTERISTIC = "7772e5db-3868-4112-a1a9-f2669d106bf3";

export type BleMidiHandlers = {
  onPacket: (data: DataView) => void;
  onConnectionChange: (connected: boolean, label: string) => void;
  onError: (message: string) => void;
};

export class BleMidiClient {
  private device: BluetoothDevice | null = null;
  private server: BluetoothRemoteGATTServer | null = null;
  private char: BluetoothRemoteGATTCharacteristic | null = null;
  private unsubDisconnect: (() => void) | null = null;

  constructor(private readonly handlers: BleMidiHandlers) {}

  getDeviceName(): string {
    return this.device?.name ?? "—";
  }

  async connect(): Promise<void> {
    if (!navigator.bluetooth) {
      this.handlers.onError(
        "Web Bluetooth no está disponible. Usa Chrome en Android o escritorio, o sirve la app por HTTPS."
      );
      return;
    }

    try {
      const device = await navigator.bluetooth.requestDevice({
        filters: [{ services: [BLE_MIDI_SERVICE] }],
        optionalServices: [BLE_MIDI_SERVICE],
      });

      this.device = device;
      const label = device.name ?? "Dispositivo BLE";

      this.unsubDisconnect?.();
      const onDisconnect = () => {
        this.handlers.onConnectionChange(false, label);
        this.cleanupGattRefs();
      };
      device.addEventListener("gattserverdisconnected", onDisconnect);
      this.unsubDisconnect = () =>
        device.removeEventListener("gattserverdisconnected", onDisconnect);

      const server = await device.gatt?.connect();
      if (!server) {
        this.handlers.onError("No se pudo obtener GATT.");
        return;
      }
      this.server = server;

      const service = await server.getPrimaryService(BLE_MIDI_SERVICE);
      const characteristic = await service.getCharacteristic(BLE_MIDI_CHARACTERISTIC);
      this.char = characteristic;

      characteristic.addEventListener(
        "characteristicvaluechanged",
        (ev: Event) => {
          const target = ev.target as BluetoothRemoteGATTCharacteristic;
          const v = target.value;
          if (v) this.handlers.onPacket(v);
        }
      );

      await characteristic.startNotifications();
      this.handlers.onConnectionChange(true, label);
    } catch (e) {
      const msg = e instanceof Error ? e.message : String(e);
      if (msg.includes("cancel") || msg.includes("User")) {
        return;
      }
      this.handlers.onError(msg);
    }
  }

  async disconnect(): Promise<void> {
    try {
      await this.char?.stopNotifications();
    } catch {
      /* ignore */
    }
    this.char = null;
    try {
      this.server?.disconnect();
    } catch {
      /* ignore */
    }
    this.server = null;
    this.unsubDisconnect?.();
    this.unsubDisconnect = null;
    this.device = null;
    this.handlers.onConnectionChange(false, "—");
  }

  private cleanupGattRefs(): void {
    this.char = null;
    this.server = null;
  }
}
