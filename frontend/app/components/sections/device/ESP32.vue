<script setup lang="ts">
import { WifiZero, WifiLow, WifiHigh, Wifi } from "lucide-vue-next";
import {
    GPIOPin,
  GPIOPinState,
  GPIOSubTopic,
  MessageTopic,
  WifiSubTopic,
  type DeviceMessage,
} from "~/models/message";

const emit = defineEmits<{
  setGpioPin: [{ pin: GPIOPin; value: GPIOPinState }];
}>();

const props = defineProps<{
  id: string;
  name: string;
  messages: DeviceMessage[];
}>();

const { t } = useI18n();

const lastWifiScan = computed(() => {
  const wifiMessage = props.messages
    .find((msg) => msg.topic === MessageTopic.WIFI)
    ?.messages.filter((m) => m.supTopic === WifiSubTopic.SCAN)?.[0];

  return wifiMessage || null;
});

const lastWifiScanTimestamp = computed(() => {
  return lastWifiScan.value ? new Date(lastWifiScan.value.timestamp) : null;
});

const lastWifiScanNetworks = computed(() => {
  return lastWifiScan.value
    ? lastWifiScan.value.networks.sort((a, b) => b.rssi - a.rssi)
    : [];
});

const lastGpioMessage = computed(() => {
  return (
    props.messages.filter((msg) => msg.topic === MessageTopic.GPIO)?.[0] || null
  );
});

const lastGpioStatesMessage = computed(() => {
  return (
    lastGpioMessage.value?.messages.filter(
      (msg) => msg.supTopic === GPIOSubTopic.STATE
    )?.[0] || null
  );
});

function getStatusColor(messages: DeviceMessage[]) {
  const statusMessage = messages.find(
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

function handleChange(event: Event) {
  // Ändere den Wert basierend auf dem Status der Checkbox
  const target = event.target as HTMLInputElement;
  console.log("test", target.checked);
  return target.checked;
}

function setGpioPinState(pin: GPIOPin, value: GPIOPinState) {
  // Hier würde die Logik implementiert werden, um den GPIO-Pin-Zustand zu setzen
  console.log(`Set GPIO Pin ${pin} to value ${value}`);
  emit("setGpioPin", { pin, value });
}
</script>

<template>
  <div
    class="w-[400px] min-h-[300px] flex flex-col border p-16 rounded-md card-color light-effect"
  >
    <div class="flex justify-between gap-10">
      <h2>{{ props.name }}</h2>
      <div
        class="w-12 h-12 rounded-full"
        :class="getStatusColor(props.messages)"
      ></div>
    </div>
    <span class="text-10">#{{ props.id }}</span>

    <div class="flex justify-between gap-12 text-12">
      <div class="w-1/2">
        <div
          v-for="(wifi, index) in lastWifiScanNetworks.slice(0, 5)"
          :key="index"
          class="flex gap-12"
        >
          <Wifi v-if="wifi.rssi >= -50" class="w-16 h-16" />
          <WifiHigh
            v-else-if="wifi.rssi < -50 && wifi.rssi >= -65"
            class="w-16 h-16"
          />
          <WifiLow
            v-else-if="wifi.rssi < -65 && wifi.rssi >= -75"
            class="w-16 h-16"
          />
          <WifiZero v-else class="w-16 h-16" />
          <div class="truncate">{{ wifi.ssid }}</div>
        </div>
        <div v-if="lastWifiScanNetworks.length > 5">
          <span>{{
            t("common.andValueMore", { count: lastWifiScanNetworks.length - 5 })
          }}</span>
        </div>
      </div>
      <div>
        <span>GPIO</span>
        <span>{{ lastGpioStatesMessage?.timestamp }}</span>
        <div
          v-for="(value, pin) in lastGpioStatesMessage?.state"
          :key="pin"
          class="flex gap-12"
        >
          <span>Pin {{ pin }}: {{ value }}</span>
          <input
            type="checkbox"
            :checked="value === 1"
            @change="
              (event) => setGpioPinState(pin, handleChange(event) ? 1 : 0)
            "
          />
        </div>
      </div>
    </div>
  </div>
</template>
