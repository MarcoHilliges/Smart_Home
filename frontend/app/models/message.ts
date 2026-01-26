import type { DeviceStatus, GPIO, GPIOPin, GPIOPinState, WLANNetwork } from "./device";

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

export interface StatusMessage {
  status: DeviceStatus;
  wifi: string;
  rssi: number;
  uptime: number;
  timestamp: number;
  deviceName: string;
  gpioStates: GPIO[];
}

export interface WifiScanMessage {
  supTopic: WifiSubTopic.SCAN;
  networks: WLANNetwork[];
  timestamp: number;
}

export interface GPIOStateMessage {
  supTopic: GPIOSubTopic.STATE;
  gpioStates: GPIO[];
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
