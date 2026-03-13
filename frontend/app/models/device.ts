import type { DeviceMessage, SettingsMessage } from "./message";

export type ContentTab = "overview" | "wifi" | "gpio" | "settings";

export type DeviceStatus = "online" | "error" | "offline";

export type DigitalState = "HIGH" | "LOW";

export type PinMode =
  | "Digital_Input"
  | "Digital_Output"
  | "PWM"
  | "Analog_Input"
  | "Analog_Output"
  | "Touch_Sensor"
  | "None";

export type GPIOActorRole =
  | "Light" // Digital
  | "Pump" // Digital
  | "Relay" // Digital
  | "Buzzer" // Digital
  | "Heater" // Digital
  | "Fan" // PWM
  | "LedStrip" // PWM
  | "Valve"; // PWM

export type GPIOSensorRole =
  | "Temperature" // Analog
  | "Humidity" // Analog
  | "LightSensor" // Analog
  | "SoilMoisture" // Analog
  | "WaterLevel" // Analog
  | "Motion" // Digital
  | "DoorContact" // Digital
  | "Switch" // Digital
  | "Touch" // Touch
  | "None"; // Für abgeschaltete Pins

export type GPIORole = GPIOActorRole | GPIOSensorRole;

export interface GPIOActor {
  pinNumber: GPIOPin;
  mode?: PinMode;
  role?: GPIOActorRole;
  label?: string;
  state: DigitalState;
}

export interface GPIOSensor {
  pinNumber: GPIOPin;
  mode?: PinMode;
  role?: GPIOSensorRole;
  label?: string;
  state: number;
}

export type GPIO = GPIOActor | GPIOSensor;

export type ExtendedGPIO = GPIO & {
  deviceId: string;
  deviceName: string;
  deviceStatus: DeviceStatus;
};

export type SetGPIO = Pick<GPIO, "pinNumber" | "state">;

export interface Device {
  id: string;
  name: string;
  lastSeen: number | null;
  gpios: GPIO[];
  deviceStatus: DeviceStatus;
  messages: DeviceMessage[];
  settings: DeviceSettings | null;
}

export interface WLANNetwork {
  ssid: string;
  rssi: number;
  encryption: number;
}

// Settings
export interface DeviceSettings {
  wifiScanInterval: number;
}

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
