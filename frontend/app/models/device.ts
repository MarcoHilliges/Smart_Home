import type { DeviceMessage, SettingsMessage } from "./message";

export type DeviceStatus = "online" | "error" | "offline";

export enum GPIOPinState {
  LOW = 0,
  HIGH = 1,
}

export type GPIOGroupId = "lamp" | "pump" | "none"

export interface GPIO {
  pinNumber: GPIOPin
  group?: GPIOGroupId
  label?: string
  state: GPIOPinState
}

export interface ExtendedGPIO extends GPIO {
  deviceId: string;
  deviceName: string;
}

export type SetGPIO = Pick<GPIO, 'pinNumber' | 'state'>;

export interface Device {
  id: string;
  name: string;
  lastSeen: number | null;
  gpios: GPIO[]
  messages: DeviceMessage[];
}

export interface WLANNetwork {
  ssid: string;
  rssi: number;
  encryption: number;
}

// Settings
export interface StringSettingsItem {
  key: keyof SettingsMessage;
  label: string;
  description: string;
  value: string | null;
  valueType: "string";
  min?: number;
  max?: number;
  inactive?: boolean;
}

export interface NumberSettingsItem {
  key: keyof SettingsMessage;
  label: string;
  description: string;
  value: number | null;
  valueType: "number";
  min?: number;
  max?: number;
  inactive?: boolean;
}

export type SettingsItem = StringSettingsItem | NumberSettingsItem;

// ESP-32
export enum ESP32GPIOPin {
  PIN_2 = 2,
  PIN_4 = 4,
  PIN_16 = 16,
  PIN_17 = 17,
  PIN_18 = 18,
  PIN_19 = 19,
  PIN_21 = 21,
  PIN_22 = 22,
  PIN_23 = 23,
  PIN_25 = 25,
  PIN_26 = 26,
  PIN_27 = 27,
}

// Union type for GPIO pins - can be extended with other microcontroller pin types (STM32, Arduino, etc.)
export type GPIOPin = ESP32GPIOPin;
