<script lang="ts" setup>
enum MessageTopic {
  STATUS = "status",
  WIFI = "wifi",
}

enum WifiSubTopic {
  SCAN = "scan",
}

type StatusMessage = {
  status: "online" | "error";
  uptime: number;
  timestamp: number;
};

type WLANNetwork = {
  ssid: string;
  rssi: number;
  encryption: number;
};

type WifiScanMessage = {
  supTopic: WifiSubTopic.SCAN;
  networks: WLANNetwork[];
  timestamp: number;
};

type Device = {
  id: string;
  messages: (
    | {
        topic: MessageTopic.STATUS;
        messages: StatusMessage[];
      }
    | {
        topic: MessageTopic.WIFI;
        messages: WifiScanMessage[];
      }
  )[];
};

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
    const deviceId = topic.split("/")[1]; // Extrahiere die Geräte-ID aus dem Topic
    const topicType = topic.split("/")[2]; // Extrahiere den Nachrichtentyp aus dem Topic
    const supTopicType = topic.split("/")[3]; // Extrahiere den Sub-Nachrichtentyp aus dem Topic (falls vorhanden)

    if (!deviceId || !topicType) return;
    let deviceEntry = devices.value.find(({ id }) => id === deviceId);

    if (!deviceEntry) {
      const entry = { id: deviceId, messages: [] };
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

      default:
        console.warn("Unknown topic type:", topicType);
    }
  });
});

onUnmounted(() => {
  // Optional: Clean up listeners or disconnect
  // $mqtt.end(); // Normalerweise will man den globalen Client nicht beenden, wenn eine Komponente unmountet wird.
  // Hier nur die spezifischen Komponent-Listener entfernen.
  $mqtt.removeAllListeners("message"); // Beispiel: Nur Message-Listener entfernen.
});

function getStatusColor(device: Device) {
  const statusMessage = device.messages.find(
    (msg) => msg.topic === MessageTopic.STATUS
  );
  switch (statusMessage?.messages[0]?.status) {
    case "online":
      return "bg-success";

    case "error":
      return "bg-error";

    default:
      return "bg-gray-300";
  }
}
</script>

<template>
  <div id="start" class="mt-[92px] flex flex-col items-center">
    <span> mqttIsConnected: {{ mqttIsConnected }} </span>
    <div v-for="(device, index) in devices" :key="index">
      <h2>Device ID: {{ device.id }}</h2>

      <div
        class="w-12 h-12 rounded-full"
        :class="getStatusColor(device)"
      ></div>

      <div v-for="(message, msgIndex) in device.messages" :key="msgIndex">
        <pre>{{ message }}</pre>
      </div>
    </div>
  </div>
</template>

<style scoped></style>
