<script lang="ts" setup>
import {
  GPIOSubTopic,
  MessageTopic,
  WifiSubTopic,
  type StatusMessage,
  type WifiScanMessage,
} from "~/models/message";
import ESP32 from "./ESP32.vue";
import type { Device, GPIOPin, GPIOPinState, SetGPIO } from "~/models/device";

const { $mqtt, $mqttConnectionState } = useNuxtApp();

const {
  devices,
  addDevice,
  initializeStore,
  updateDeviceLastSeen,
  addStatusMessage,
  addWifiScanMessage,
  addGpioStateMessage,
  saveDataIntoLocalStorage,
  loadDataFromLocalStorage,
} = useDeviceStore();

onMounted(() => {
  loadDataFromStorage();
  devices.value.forEach((device) => {
    device.lastSeen = null;
  });

  if (!$mqtt) {
    console.error("MQTT Client not injected by plugin.");
    return;
  }

  $mqtt.on("message", (topic, message) => {
    const deviceId = topic.split("/")[1]; // Extrahiere die Geräte-ID aus dem Topic
    const topicType = topic.split("/")[2]; // Extrahiere den Nachrichtentyp aus dem Topic
    const subTopicType = topic.split("/")[3]; // Extrahiere den Sub-Nachrichtentyp aus dem Topic (falls vorhanden)

    if (!deviceId || !topicType) return;
    let deviceEntry = devices.value.find(({ id }) => id === deviceId);

    if (!deviceEntry) {
      const newDevice: Device = {
        id: deviceId,
        name: "",
        lastSeen: null,
        gpios: [],
        messages: [],
      };
      addDevice(newDevice);
      deviceEntry = devices.value.find(({ id }) => id === deviceId);
    }
    if (!deviceEntry) return console.error("Device Entry should exist here.");
    updateDeviceLastSeen(deviceId, Date.now());

    switch (topicType) {
      case MessageTopic.STATUS:
        const statusMessage: StatusMessage = JSON.parse(message.toString());
        statusMessage.timestamp = Date.now();
        deviceEntry.name = statusMessage.deviceName;

        addStatusMessage(deviceId, statusMessage);

        break;

      case MessageTopic.WIFI:
        if (subTopicType === WifiSubTopic.SCAN) {
          const wifiScanMessage: WifiScanMessage = {
            supTopic: WifiSubTopic.SCAN,
            networks: JSON.parse(message.toString()).networks,
            timestamp: Date.now(),
          };
          addWifiScanMessage(deviceId, wifiScanMessage);
        } else console.warn("SubTopicType not supported: ", subTopicType);

        break;

      case MessageTopic.GPIO:
        if (subTopicType === GPIOSubTopic.STATE) {
          const gpioStateMessage = {
            supTopic: GPIOSubTopic.STATE as const,
            gpioStates: JSON.parse(message.toString()).gpioStates,
            timestamp: Date.now(),
          };

          addGpioStateMessage(deviceId, gpioStateMessage);
        } else console.warn("SubTopicType not supported: ", subTopicType);

        break;

      case MessageTopic.SETTINGS:
        break;

      default:
        console.warn("Unknown topic type:", topicType);
    }
    saveDataIntoLocalStorage();
  });
});

onUnmounted(() => {
  $mqtt.removeAllListeners("message");
});

function setGpioPinState(deviceId: string, pin: GPIOPin, value: GPIOPinState) {
  const payload: SetGPIO[] = [{ pinNumber: pin, state: value }];
  const topic = `esp32/${deviceId}/gpio/set`;
  const message = JSON.stringify(payload);
  $mqtt.publish(topic, message);
}

function getStatus(deviceId: string) {
  const topic = `esp32/${deviceId}/status/get`;
  $mqtt.publish(topic, "");
}

function getWifiScan(deviceId: string) {
  const topic = `esp32/${deviceId}/wifi/get`;
  $mqtt.publish(topic, "");
}

function getGpioStates(deviceId: string) {
  const topic = `esp32/${deviceId}/gpio/get`;
  $mqtt.publish(topic, "");
}

function loadDataFromStorage() {
  const data = loadDataFromLocalStorage();
  if (data) {
    initializeStore(JSON.parse(data));
  }
}

const gpioGroups = computed(() => {
  const gpios = devices.value.flatMap((device) => device.gpios);
 
  return gpios;
});
</script>

<template>
  <div class="mt-[92px] flex flex-col items-center">
    <div class="w-full flex flex-wrap justify-center">
      <!-- <pre>{{ gpioGroups }}</pre> -->
      <template v-for="device in devices" :key="device.id">
        <ESP32
          :device="device"
          :clientState="$mqttConnectionState"
          @setGpioPin="
            ({ pin, value }) => setGpioPinState(device.id, pin, value)
          "
          @getStatus="getStatus(device.id)"
          @getWifiScan="getWifiScan(device.id)"
          @getGpioStates="getGpioStates(device.id)"
        />
      </template>
    </div>
  </div>
</template>

<style scoped></style>
