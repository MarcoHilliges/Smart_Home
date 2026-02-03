import type { Device, GPIO, GPIOModeActor, GPIOModeSensor } from "~/models/device";
import {
  MessageTopic,
  type GPIOStateMessage,
  type StatusMessage,
  type WifiScanMessage,
} from "~/models/message";

const devices = ref<Device[]>([]);
let isInitialized = false;
const localStorageKey = "Device_Data";

const gpioModesActor: { label: string, value: GPIOModeActor}[] = [
  { label: "device.actor.lamp", value: "lamp" },
  { label: "device.actor.pump", value: "pump" },
  { label: "device.actor.fan", value: "fan" },
  { label: "device.actor.relay", value: "relay" },
  { label: "device.actor.buzzer", value: "buzzer" },
  { label: "device.actor.led strip", value: "led_strip" },
  { label: "device.actor.valve", value: "valve" },
  { label: "device.actor.heater", value: "heater" },
]

const gpioModesSensor: { label: string, value: GPIOModeSensor}[] = [
  { label: "device.sensor.temperature", value: "temperature" },
  { label: "device.sensor.humidity", value: "humidity" },
  { label: "device.sensor.light", value: "light" },
  { label: "device.sensor.soil moisture", value: "soil_moisture" },
  { label: "device.sensor.water level", value: "water_level" },
  { label: "device.sensor.motion", value: "motion" },
  { label: "device.sensor.door contact", value: "door_contact" },
  { label: "device.sensor.touch", value: "touch" },
]

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

  const setGpioState = (deviceId: string, gpioStates: GPIO[]) => {
    const device = devices.value.find((d) => d.id === deviceId);
    if (!device) return;

    gpioStates.forEach((gpioState) => {
      const gpio = device.gpios.find(
        (g) => g.pinNumber === gpioState.pinNumber,
      );
      if (gpio) {
        gpio.state = gpioState.state;
      } else {
        device.gpios.push({
          pinNumber: gpioState.pinNumber,
          state: gpioState.state,
          mode: "none",
          label: "",
        });
      }
    });
  };

  const addStatusMessage = (deviceId: string, message: StatusMessage) => {
    const device = devices.value.find((d) => d.id === deviceId);
    if (!device) return;

    let statusMessages = device.messages.find(
      (msg) => msg.topic === MessageTopic.STATUS,
    );
    if (!statusMessages) {
      statusMessages = { topic: MessageTopic.STATUS, messages: [] };
      device.messages.push(statusMessages);
    }
    if (message.gpioStates?.length) setGpioState(deviceId, message.gpioStates);

    statusMessages.messages = [message, ...statusMessages.messages].slice(
      0,
      10,
    ); // Nur die letzten 10 Nachrichten behalten
  };

  const addWifiScanMessage = (deviceId: string, message: WifiScanMessage) => {
    const device = devices.value.find((d) => d.id === deviceId);
    if (!device) return;
    let wifiMessages = device.messages.find(
      (msg) => msg.topic === MessageTopic.WIFI,
    );
    if (!wifiMessages) {
      wifiMessages = { topic: MessageTopic.WIFI, messages: [] };
      device.messages.push(wifiMessages);
    }
    wifiMessages.messages = [message, ...wifiMessages.messages].slice(0, 10); // Nur die letzten 10 Nachrichten behalten
  };

  const addGpioStateMessage = (deviceId: string, message: GPIOStateMessage) => {
    const device = devices.value.find((d) => d.id === deviceId);
    if (!device) return;
    let gpioMessages = device.messages.find(
      (msg) => msg.topic === MessageTopic.GPIO,
    );
    if (!gpioMessages) {
      gpioMessages = { topic: MessageTopic.GPIO, messages: [] };
      device.messages.push(gpioMessages);
    }

    setGpioState(deviceId, message.gpioStates);

    gpioMessages.messages = [message, ...gpioMessages.messages].slice(0, 10); // Nur die letzten 10 Nachrichten behalten
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
    loadDataFromLocalStorage,
    gpioModesActor,
    gpioModesSensor,
  };
};
