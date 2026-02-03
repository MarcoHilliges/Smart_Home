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

const gpioModesActor: { i18nKey: string, value: GPIOModeActor}[] = [
  { i18nKey: "device.actor.light", value: "light" },
  { i18nKey: "device.actor.pump", value: "pump" },
  { i18nKey: "device.actor.fan", value: "fan" },
  { i18nKey: "device.actor.relay", value: "relay" },
  { i18nKey: "device.actor.buzzer", value: "buzzer" },
  { i18nKey: "device.actor.led_strip", value: "led_strip" },
  { i18nKey: "device.actor.valve", value: "valve" },
  { i18nKey: "device.actor.heater", value: "heater" },
]

const gpioModesSensor: { i18nKey: string, value: GPIOModeSensor}[] = [
  { i18nKey: "device.sensor.temperature", value: "temperature" },
  { i18nKey: "device.sensor.humidity", value: "humidity" },
  { i18nKey: "device.sensor.light", value: "light" },
  { i18nKey: "device.sensor.soil_moisture", value: "soil_moisture" },
  { i18nKey: "device.sensor.water_level", value: "water_level" },
  { i18nKey: "device.sensor.motion", value: "motion" },
  { i18nKey: "device.sensor.door_contact", value: "door_contact" },
  { i18nKey: "device.sensor.touch", value: "touch" },
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
