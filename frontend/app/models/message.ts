export type MqttClientState =
  | "connected"
  | "disconnected"
  | "error"
  | "reconnecting";

export type DeviceStatus = "online" | "error" | "offline";

export enum MessageTopic {
  STATUS = "status",
  WIFI = "wifi",
  GPIO = "gpio",
  SETTINGS = "settings",
}

export enum StatusSubTopic {
  GET = "get",
}

export enum WifiSubTopic {
  SCAN = "scan",
  GET = "get",
}

export enum GPIOSubTopic {
  STATE = "state",
  GET = "get",
}

export enum GPIOPin {
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

export enum GPIOPinState {
  LOW = 0,
  HIGH = 1,
}

export interface StatusMessage {
  status: DeviceStatus;
  wifi: string;
  rssi: number;
  uptime: number;
  timestamp: number;
  deviceName: string;
  gpioStates: Record<GPIOPin, GPIOPinState>;
}

export interface WLANNetwork {
  ssid: string;
  rssi: number;
  encryption: number;
}

export interface WifiScanMessage {
  supTopic: WifiSubTopic.SCAN;
  networks: WLANNetwork[];
  timestamp: number;
}

export interface GPIOStateMessage {
  supTopic: GPIOSubTopic.STATE;
  state: Record<GPIOPin, GPIOPinState>;
  timestamp: number;
}

export interface SettingsMessage {
  deviceName: string;
  wifiScanInterval: number;
}

export type DeviceMessage =
  | {
      topic: MessageTopic.STATUS;
      messages: StatusMessage[];
    }
  | {
      topic: MessageTopic.WIFI;
      messages: WifiScanMessage[];
    }
  | {
      topic: MessageTopic.GPIO;
      messages: GPIOStateMessage[];
    }
  | {
      topic: MessageTopic.SETTINGS;
      messages: SettingsMessage;
    };

export interface Device {
  id: string;
  name: string;
  lastSeen: number | null;
  messages: DeviceMessage[];
}
