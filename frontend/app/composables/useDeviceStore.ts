import {
  BellOff,
  BellRing,
  Droplet,
  DropletOff,
  Fan,
  Flame,
  Lightbulb,
  LightbulbOff,
  Snowflake,
  ToggleLeft,
  ToggleRight,
  type LucideProps,
} from "lucide-vue-next";
import type { FunctionalComponent } from "vue";
import type {
  Device,
  GPIO,
  GPIOActorRole,
  GPIOSensorRole,
} from "~/models/device";
import {
  MessageTopic,
  type GPIOStateMessage,
  type StatusMessage,
  type WifiScanMessage,
} from "~/models/message";

const devices = ref<Device[]>([]);
let isInitialized = false;
const localStorageKey = "Device_Data";

const gpioRolesActor: {
  i18nKey: string;
  value: GPIOActorRole;
  symbolOn?: FunctionalComponent<LucideProps>;
  symbolOff?: FunctionalComponent<LucideProps>;
  colorOn?: string;
}[] = [
  {
    i18nKey: "device.actor.light",
    value: "Light",
    symbolOn: Lightbulb,
    symbolOff: LightbulbOff,
    colorOn: "text-yellow-400",
  },
  {
    i18nKey: "device.actor.pump",
    value: "Pump",
    symbolOn: Droplet,
    symbolOff: DropletOff,
    colorOn: "text-blue-400",
  },
  {
    i18nKey: "device.actor.fan",
    value: "Fan",
    symbolOn: Fan,
    symbolOff: Fan,
    colorOn: "text-green-400",
  },
  {
    i18nKey: "device.actor.relay",
    value: "Relay",
    symbolOn: ToggleLeft,
    symbolOff: ToggleRight,
    colorOn: "text-green-400",
  },
  {
    i18nKey: "device.actor.buzzer",
    value: "Buzzer",
    symbolOn: BellRing,
    symbolOff: BellOff,
    colorOn: "text-yellow-400",
  },
  {
    i18nKey: "device.actor.led_strip",
    value: "LedStrip",
    symbolOn: Lightbulb,
    symbolOff: LightbulbOff,
    colorOn: "text-purple-400",
  },
  {
    i18nKey: "device.actor.valve",
    value: "Valve",
    symbolOn: Droplet,
    symbolOff: DropletOff,
    colorOn: "text-blue-400",
  },
  {
    i18nKey: "device.actor.heater",
    value: "Heater",
    symbolOn: Flame,
    symbolOff: Snowflake,
    colorOn: "text-red-400",
  },
];

const gpioRolesSensor: { i18nKey: string; value: GPIOSensorRole }[] = [
  { i18nKey: "device.sensor.temperature", value: "Temperature" },
  { i18nKey: "device.sensor.humidity", value: "Humidity" },
  { i18nKey: "device.sensor.light_sensor", value: "LightSensor" },
  { i18nKey: "device.sensor.soil_moisture", value: "SoilMoisture" },
  { i18nKey: "device.sensor.water_level", value: "WaterLevel" },
  { i18nKey: "device.sensor.motion", value: "Motion" },
  { i18nKey: "device.sensor.door_contact", value: "DoorContact" },
  { i18nKey: "device.sensor.touch", value: "Touch" },
];

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
        for (const key in gpioState) {
          if (key === "label") gpio.label = gpioState.label;
          else if (key === "mode") gpio.mode = gpioState.mode;
          else if (key === "role") gpio.role = gpioState.role;
          else if (key === "state") gpio.state = gpioState.state;
        }
      } else {
        if (typeof gpioState.state === "number") {
          device.gpios.push({
            pinNumber: gpioState.pinNumber,
            state: gpioState.state,
            mode: gpioState.mode || "None",
            role: gpioState.role || "None",
            label: gpioState.label || "",
          });
        } else {
          device.gpios.push({
            pinNumber: gpioState.pinNumber,
            state: gpioState.state,
            mode: gpioState.mode || "None",
            role: gpioState.role,
            label: gpioState.label || "",
          });
        }
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
    gpioRolesActor,
    gpioRolesSensor,
  };
};
