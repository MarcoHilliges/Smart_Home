<script lang="ts" setup>
import {
  GPIOPin,
  GPIOPinState,
  GPIOSubTopic,
  MessageTopic,
  WifiSubTopic,
  type Device,
  type StatusMessage,
  type WifiScanMessage,
} from "~/models/message";
import ESP32 from "./ESP32.vue";

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
  loadDataFromLocalStorage
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
    const deviceIdAndName = topic.split("/")[1]; // Extrahiere die Geräte-ID aus dem Topic
    const topicType = topic.split("/")[2]; // Extrahiere den Nachrichtentyp aus dem Topic
    const subTopicType = topic.split("/")[3]; // Extrahiere den Sub-Nachrichtentyp aus dem Topic (falls vorhanden)

    const last_dash_index = deviceIdAndName?.lastIndexOf("-");
    if (!last_dash_index || !deviceIdAndName) return;
    const deviceName = deviceIdAndName.substring(0, last_dash_index);
    const deviceId = deviceIdAndName.substring(last_dash_index + 1);

    if (!deviceId || !topicType) return;
    let deviceEntry = devices.value.find(({ id }) => id === deviceId);

    if (!deviceEntry) {
      const newDevice = {
        id: deviceId,
        name: deviceName,
        lastSeen: null,
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
            state: JSON.parse(message.toString()),
            timestamp: Date.now(),
          };
          
          addGpioStateMessage(deviceId, gpioStateMessage)
        } else console.warn('SubTopicType not supported: ', subTopicType)
        
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

function setGpioPinState(
  deviceName: string,
  deviceId: string,
  pin: GPIOPin,
  value: GPIOPinState
) {
  const topic = `esp32/${deviceName}-${deviceId}/gpio/set`;
  const message = JSON.stringify({ [pin]: value });
  $mqtt.publish(topic, message);
}

function getStatus(deviceName: string, deviceId: string) {
  const topic = `esp32/${deviceName}-${deviceId}/status/get`;
  $mqtt.publish(topic, "");
}

function getWifiScan(deviceName: string, deviceId: string) {
  const topic = `esp32/${deviceName}-${deviceId}/wifi/get`;
  $mqtt.publish(topic, "");
}

function getGpioStates(deviceName: string, deviceId: string) {
  const topic = `esp32/${deviceName}-${deviceId}/gpio/get`;
  $mqtt.publish(topic, "");
}

function loadDataFromStorage() {
  console.log("Loading device data from localStorage.");
  const data = loadDataFromLocalStorage()
  if (data) {
    initializeStore(JSON.parse(data));
  }
}
</script>

<template>
  <div id="devices" class="mt-[92px] flex flex-col items-center">
    <div class="w-full flex flex-wrap justify-center">
      <template v-for="device in devices" :key="device.id">
        <ESP32
          :id="device.id"
          :name="device.name"
          :messages="device.messages"
          :lastSeen="device.lastSeen"
          :clientState="$mqttConnectionState"
          @setGpioPin="
            ({ pin, value }) =>
              setGpioPinState(device.name, device.id, pin, value)
          "
          @getStatus="getStatus(device.name, device.id)"
          @getWifiScan="getWifiScan(device.name, device.id)"
          @getGpioStates="getGpioStates(device.name, device.id)"
        />
      </template>
    </div>
  </div>
</template>

<style scoped></style>
