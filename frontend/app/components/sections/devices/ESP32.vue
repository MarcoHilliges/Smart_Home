<script setup lang="ts">
import { WifiZero, WifiLow, WifiHigh, Wifi } from "lucide-vue-next";
import {
  GPIOPin,
  GPIOPinState,
  MessageTopic,
  WifiSubTopic,
  type DeviceMessage,
  type MqttClientState,
} from "~/models/message";

const emit = defineEmits<{
  setGpioPin: [{ pin: GPIOPin; value: GPIOPinState }];
  getStatus: [];
  getWifiScan: [];
  getGpioStates: [];
}>();

const props = defineProps<{
  id: string;
  name: string;
  messages: DeviceMessage[];
  lastSeen: number | null;
  clientState: MqttClientState;
}>();

const { t } = useI18n();

// Status
const lastStatusMessage = computed(() => {
  return (
    props.messages.find((msg) => msg.topic === MessageTopic.STATUS)
      ?.messages[0] || null
  );
});

const deviceStatus = computed(() => {
  return props.clientState === "connected" &&
    props.lastSeen &&
    lastStatusMessage.value?.status
    ? lastStatusMessage.value?.status
    : "offline";
});

const statusColor = computed(() => {
  switch (deviceStatus.value) {
    case "online":
      return "bg-success";

    case "error":
      return "bg-error";

    case "offline":
      return "bg-gray-300";

    default:
      return "bg-gray-300";
  }
});

// Wifi
const wifiScanMessages = computed(() => {
  return (
    props.messages
      .find((msg) => msg.topic === MessageTopic.WIFI)
      ?.messages.filter((m) => m.supTopic === WifiSubTopic.SCAN) || []
  );
});

// GPIO
const gpioMessages = computed(() => {
  return (
    props.messages.find((msg) => msg.topic === MessageTopic.GPIO)?.messages ||
    []
  );
});

// Helpers
function formatTimestamp(timestamp: number | undefined | null) {
  if (!timestamp) return "-";
  const date = new Date(timestamp);
  return date.toLocaleString();
}

watch(
  () => props.lastSeen,
  (newVal) => {
    if (
      props.clientState === "connected" &&
      Date.now() - (newVal || 60000) > 45000
    ) {
      emit("getStatus");
    }
  },
  { immediate: true }
);
</script>

<template>
  <div
    class="max-w-[400px] min-w-[300px] sm:min-w-[400px] min-h-[300px] flex flex-col border p-16 m-24 rounded-md card-color light-effect"
  >
    <div class="flex justify-between gap-10">
      <h2>{{ props.name }}</h2>
      <div class="flex gap-12 items-center">
        <BasicTooltip v-if="lastStatusMessage && deviceStatus === 'online'">
          <Wifi v-if="lastStatusMessage.rssi >= -50" class="w-16 h-16" />
          <WifiHigh
            v-else-if="
              lastStatusMessage.rssi < -50 && lastStatusMessage.rssi >= -65
            "
            class="w-16 h-16"
          />
          <WifiLow
            v-else-if="
              lastStatusMessage.rssi < -65 && lastStatusMessage.rssi >= -75
            "
            class="w-16 h-16"
          />
          <WifiZero v-else class="w-16 h-16" />
          <template #tooltipText>
            <div class="flex gap-6">
              <span>{{ lastStatusMessage.wifi }}</span>
              |
              <span>RSSI: {{ lastStatusMessage.rssi }} dBm</span>
            </div>
          </template>
        </BasicTooltip>
        <BasicTooltip :tooltipText="t(`common.status.${deviceStatus}`)">
          <div class="w-12 h-12 rounded-full" :class="statusColor"></div>
        </BasicTooltip>
      </div>
    </div>
    <span class="text-10">#{{ props.id }}</span>

    <div class="flex justify-between gap-12 text-12">
      <SectionsDevicesComponentsWiFiScanList
        :wiFiScanMessages="wifiScanMessages"
        :deviceStatus="deviceStatus"
        :connectedWiFi="lastStatusMessage?.wifi || ''"
        class="w-1/2"
        @getWifiScan="emit('getWifiScan')"
      />
      <SectionsDevicesComponentsGpioList
        :gpioStateMessages="gpioMessages"
        :deviceStatus="deviceStatus"
        class="w-1/2"
        @getGpioStates="emit('getGpioStates')"
        @setGpioPin="(params) => emit('setGpioPin', params)"
      />
    </div>
  </div>
</template>
