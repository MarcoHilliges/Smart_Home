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

const { t } = useI18n();

const { $mqtt } = useNuxtApp();

const devices = ref<Device[]>([]);

const mqttIsConnected = computed(() => {
  return $mqtt && $mqtt.connected;
});

onMounted(() => {
  if (!$mqtt) {
    console.error("MQTT Client not injected by plugin.");
    return;
  }

  $mqtt.on("message", (topic, message) => {
    console.log("Received message:", topic, message.toString());
    const deviceIdAndName = topic.split("/")[1]; // Extrahiere die Geräte-ID aus dem Topic
    const topicType = topic.split("/")[2]; // Extrahiere den Nachrichtentyp aus dem Topic
    const supTopicType = topic.split("/")[3]; // Extrahiere den Sub-Nachrichtentyp aus dem Topic (falls vorhanden)

    const last_dash_index = deviceIdAndName?.lastIndexOf("-");
    if (!last_dash_index || !deviceIdAndName) return;
    const deviceName = deviceIdAndName.substring(0, last_dash_index);
    const deviceId = deviceIdAndName.substring(last_dash_index + 1);

    if (!deviceId || !topicType) return;
    let deviceEntry = devices.value.find(({ id }) => id === deviceId);

    if (!deviceEntry) {
      const entry = { id: deviceId, name: deviceName, messages: [] };
      devices.value.push(entry);
      deviceEntry = devices.value.find(({ id }) => id === deviceId);
    }
    if (!deviceEntry) return console.error("Device Entry should exist here.");

    switch (topicType) {
      case MessageTopic.STATUS:
        const statusMessage: StatusMessage = JSON.parse(message.toString());
        statusMessage.timestamp = Date.now();

        let statusTopicEntry = deviceEntry.messages.find(
          (entry) => entry.topic === MessageTopic.STATUS
        );
        if (!statusTopicEntry) {
          statusTopicEntry = { topic: MessageTopic.STATUS, messages: [] };
          deviceEntry.messages.push(statusTopicEntry);
        }

        // Füge die neue Nachricht hinzu und beschränke die Liste auf die letzten 10 Einträge
        statusTopicEntry.messages = [
          statusMessage,
          ...statusTopicEntry.messages,
        ].slice(0, 10);

        break;

      case MessageTopic.WIFI:
        let wifiTopicEntry = deviceEntry.messages.find(
          (entry) => entry.topic === MessageTopic.WIFI
        );

        if (!wifiTopicEntry) {
          wifiTopicEntry = { topic: MessageTopic.WIFI, messages: [] };
          deviceEntry.messages.push(wifiTopicEntry);
          wifiTopicEntry = deviceEntry.messages.find(
            (entry) => entry.topic === MessageTopic.WIFI
          );
        }
        if (!wifiTopicEntry)
          return console.error("WIFI Topic Entry should exist here.");

        if (supTopicType === WifiSubTopic.SCAN) {
          const wifiScanMessage: WifiScanMessage = {
            supTopic: WifiSubTopic.SCAN,
            networks: JSON.parse(message.toString()).networks,
            timestamp: Date.now(),
          };

          // Füge die neue Nachricht hinzu und beschränke die Liste auf die letzten 10 Einträge
          wifiTopicEntry.messages = [
            wifiScanMessage,
            ...wifiTopicEntry.messages,
          ].slice(0, 10);
        }
        break;

      case MessageTopic.GPIO:
        let gpioTopicEntry = deviceEntry.messages.find(
          (entry) => entry.topic === MessageTopic.GPIO
        );
        if (!gpioTopicEntry) {
          gpioTopicEntry = { topic: MessageTopic.GPIO, messages: [] };
          deviceEntry.messages.push(gpioTopicEntry);
          gpioTopicEntry = deviceEntry.messages.find(
            (entry) => entry.topic === MessageTopic.GPIO
          );
        }
        if (!gpioTopicEntry)
          return console.error("GPIO Topic Entry should exist here.");
        if (supTopicType === GPIOSubTopic.STATE) {
          const gpioStateMessage = {
            supTopic: GPIOSubTopic.STATE,
            state: JSON.parse(message.toString())["gpio_states"],
            timestamp: Date.now(),
          };

          // Füge die neue Nachricht hinzu und beschränke die Liste auf die letzten 10 Einträge
          gpioTopicEntry.messages = [
            gpioStateMessage,
            ...gpioTopicEntry.messages,
          ].slice(0, 10);
        }
        break;

      default:
        console.warn("Unknown topic type:", topicType);
    }
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
</script>

<template>
  <div id="start" class="mt-[92px] flex flex-col items-center">
    <span> mqttIsConnected: {{ mqttIsConnected }} </span>

    <div>
      <template v-for="device in devices" :key="device.id">
        <SectionsDeviceESP32
          :id="device.id"
          :name="device.name"
          :messages="device.messages"
          @setGpioPin="({pin, value}) => setGpioPinState(device.name, device.id, pin, value)"
        />
      </template>
    </div>
  </div>
</template>

<style scoped></style>
