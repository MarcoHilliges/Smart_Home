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

type ContentTab = "overview" | "wifi" | "gpio" | "settings";

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

const content = ref<ContentTab>("overview");
const tabs: { label: string; value: ContentTab }[] = [
  { label: t("device.tabs.overview"), value: "overview" },
  { label: t("device.tabs.wifi"), value: "wifi" },
  { label: t("device.tabs.gpio"), value: "gpio" },
  { label: t("device.tabs.settings"), value: "settings" },
];

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
    class="max-w-[400px] min-w-[300px] sm:min-w-[400px] h-[300px] flex flex-col border pt-16 m-24 rounded-md card-color light-effect"
  >
    <div class="mx-16">
      <div class="flex justify-between gap-10">
        <h2>{{ props.name }}</h2>
        <div class="flex gap-12 items-center">
          <BasicTooltip v-if="lastStatusMessage && deviceStatus === 'online'">
            <component
              :is="
                lastStatusMessage.rssi >= -50
                  ? Wifi
                  : lastStatusMessage.rssi < -50 &&
                    lastStatusMessage.rssi >= -65
                  ? WifiHigh
                  : lastStatusMessage.rssi < -65 &&
                    lastStatusMessage.rssi >= -75
                  ? WifiLow
                  : WifiZero
              "
              class="w-16 h-16"
            />

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
      <div class="text-10">
        <span> #{{ props.id }} </span>
      </div>
    </div>

    <div class="flex-grow overflow-hidden text-12">
      <div
        v-if="content === 'overview'"
        class="flex justify-between gap-12 h-full px-16"
      >
        <DevicesSectionsWiFiScanList
          :wiFiScanMessages="wifiScanMessages"
          :deviceStatus="deviceStatus"
          :connectedWiFi="lastStatusMessage?.wifi || ''"
          class="w-1/2"
          @getWifiScan="emit('getWifiScan')"
        />
        <DevicesSectionsGpioList
          :gpioStateMessages="gpioMessages"
          :deviceStatus="deviceStatus"
          class="w-1/2"
          @getGpioStates="emit('getGpioStates')"
          @setGpioPin="(params) => emit('setGpioPin', params)"
        />
      </div>

      <DevicesSectionsWiFiHistory
        :wiFiScanMessages="wifiScanMessages"
        v-else-if="content === 'wifi'"
      />

      <DevicesSectionsGpioDetailList
        v-else-if="content === 'gpio'"
        :gpioStateMessages="gpioMessages"
        :deviceStatus="deviceStatus"
        @getGpioStates="emit('getGpioStates')"
        @setGpioPin="(params) => emit('setGpioPin', params)"
      />

      <DevicesSectionsSettings
        v-else-if="content === 'settings'"
        :deviceStatus="deviceStatus"
        :deviceId="props.id"
        :deviceName="props.name"
      />
    </div>

    <div class="mt-auto">
      <button
        v-for="tab in tabs"
        :key="tab.value"
        @click="content = tab.value"
        :style="'width: ' + 100 / tabs.length + '%;'"
        class="py-4 text-12"
        :class="{
          'border-t border-primary': content === tab.value,
          'text-gray-500': content !== tab.value,
        }"
      >
        {{ tab.label }}
      </button>
    </div>
  </div>
</template>
