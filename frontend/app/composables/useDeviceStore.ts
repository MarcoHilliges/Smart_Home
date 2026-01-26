import type { Device } from "~/models/device";
import {
  MessageTopic,
  type GPIOStateMessage,
  type StatusMessage,
  type WifiScanMessage,
} from "~/models/message";

const devices = ref<Device[]>([]);
let isInitialized = false;
const localStorageKey = "Device_Data";

export const useDeviceStore = () => {
  const initializeStore = (initialData: Device[] = []) => {
    if (!isInitialized) {
      devices.value = initialData;
      isInitialized = true;
    }
  };

  const addDevice = (device: Device) => {
    if (devices.value.find((d) => d.id === device.id)) {
      console.warn(`Device ${device.id} already exists in store.`);
      return;
    }
    devices.value.push(device);
  };

  const updateDeviceName = (deviceId: string, newName: string) => {
    const device = devices.value.find((d) => d.id === deviceId);
    if (device) {
      device.name = newName;
    }
  };

  const updateDeviceLastSeen = (deviceId: string, timestamp: number) => {
    const device = devices.value.find((d) => d.id === deviceId);
    if (device) {
      device.lastSeen = timestamp;
    }
  };

  const addStatusMessage = (deviceId: string, message: StatusMessage) => {
    const device = devices.value.find((d) => d.id === deviceId);
    if (device) {
      let statusMessages = device.messages.find(
        (msg) => msg.topic === MessageTopic.STATUS
      );
      if (!statusMessages) {
        statusMessages = { topic: MessageTopic.STATUS, messages: [] };
        device.messages.push(statusMessages);
      }
      statusMessages.messages = [message, ...statusMessages.messages].slice(
        0,
        10
      ); // Nur die letzten 10 Nachrichten behalten
    }
  };

  const addWifiScanMessage = (deviceId: string, message: WifiScanMessage) => {
    const device = devices.value.find((d) => d.id === deviceId);
    if (device) {
      let wifiMessages = device.messages.find(
        (msg) => msg.topic === MessageTopic.WIFI
      );
      if (!wifiMessages) {
        wifiMessages = { topic: MessageTopic.WIFI, messages: [] };
        device.messages.push(wifiMessages);
      }
      wifiMessages.messages = [message, ...wifiMessages.messages].slice(0, 10); // Nur die letzten 10 Nachrichten behalten
    }
  };

  const addGpioStateMessage = (deviceId: string, message: GPIOStateMessage) => {
    const device = devices.value.find((d) => d.id === deviceId);
    if (device) {
      let gpioMessages = device.messages.find(
        (msg) => msg.topic === MessageTopic.GPIO
      );
      if (!gpioMessages) {
        gpioMessages = { topic: MessageTopic.GPIO, messages: [] };
        device.messages.push(gpioMessages);
      }
      gpioMessages.messages = [message, ...gpioMessages.messages].slice(0, 10); // Nur die letzten 10 Nachrichten behalten
    }
  };

  const saveDataIntoLocalStorage = () => {
    localStorage.setItem(localStorageKey, JSON.stringify(devices.value));
  };

  const loadDataFromLocalStorage = () => {
    const data = localStorage.getItem(localStorageKey);
    return data;
  };

  return {
    devices: computed(() => devices.value),
    initializeStore,
    addDevice,
    updateDeviceName,
    updateDeviceLastSeen,
    addStatusMessage,
    addWifiScanMessage,
    addGpioStateMessage,
    saveDataIntoLocalStorage,
    loadDataFromLocalStorage
  };
};
