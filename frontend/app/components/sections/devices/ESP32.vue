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
  getStatus: [];
  getWifiScan: [];
  getGpioStates: [];
}>();

const props = defineProps<{
  id: string;
  name: string;
  messages: DeviceMessage[];
  lastSeen: number | null;
}>();

const { t } = useI18n();

// Status
const lastStatusMessage = computed(() => {
  return (
    props.messages.find((msg) => msg.topic === MessageTopic.STATUS)
      ?.messages[0] || null
  );
});

const currentTime = ref(Date.now());
let timerInterval: NodeJS.Timeout | null = null;

const deviceHasTimeout = computed(() => {
  if (!props.lastSeen) return true;
  const timeoutDuration = 45 * 1000;
  return currentTime.value - props.lastSeen > timeoutDuration;
});

const deviceStatus = computed(() => {
  return deviceHasTimeout.value
    ? "timeout"
    : lastStatusMessage.value?.status
    ? lastStatusMessage.value?.status
    : "offline";
});

const statusColor = computed(() => {
  switch (deviceStatus.value) {
    case "online":
      return "bg-success";

    case "error":
      return "bg-error";

    case "timeout":
      return "bg-warning";

    case "offline":
      return "bg-gray-300";

    default:
      return "bg-gray-300";
  }
});

// Wifi
const lastWifiScan = computed(() => {
  const wifiMessage = props.messages
    .find((msg) => msg.topic === MessageTopic.WIFI)
    ?.messages.filter((m) => m.supTopic === WifiSubTopic.SCAN)?.[0];

  return wifiMessage || null;
});

const lastWifiScanTimestamp = computed(() => {
  return formatTimestamp(lastWifiScan.value?.timestamp);
});

const lastWifiScanNetworks = computed(() => {
  return lastWifiScan.value
    ? lastWifiScan.value.networks.sort((a, b) => b.rssi - a.rssi)
    : [];
});

// GPIO
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

const lastGpioStatesTimestamp = computed(() => {
  return formatTimestamp(lastGpioStatesMessage.value?.timestamp);
});

const isLoadingGpioStates = ref(null as null | number);
const gpioPinStates = ref<Record<GPIOPin, GPIOPinState> | null>(null);

function setGpioPinState(pin: GPIOPin, value: GPIOPinState) {
  if (isLoadingGpioStates.value) return;
  isLoadingGpioStates.value = pin;

  emit("setGpioPin", { pin, value });
}

watch(
  () => lastGpioStatesTimestamp.value,
  () => {
    gpioPinStates.value = lastGpioStatesMessage.value
      ? { ...lastGpioStatesMessage.value.state }
      : null;
    isLoadingGpioStates.value = null;
  },
  { immediate: true }
);

// Lifecycle
onMounted(() => {
  timerInterval = setInterval(() => {
    currentTime.value = Date.now();
  }, 1000);

  if (Date.now() - (props.lastSeen || 60000) > 45000) {
    emit("getStatus");
    emit("getWifiScan");
    emit("getGpioStates");
  } else {
    if (Date.now() - (lastWifiScan.value?.timestamp || 120000) > 90000) {
      emit("getWifiScan");
    }
    if (
      Date.now() - (lastGpioStatesMessage.value?.timestamp || 120000) >
      90000
    ) {
      emit("getGpioStates");
    }
  }
});

onUnmounted(() => {
  if (timerInterval) {
    clearInterval(timerInterval);
  }
});

// Helpers
function formatTimestamp(timestamp: number | undefined | null) {
  if (!timestamp) return "-";
  const date = new Date(timestamp);
  return date.toLocaleString();
}
</script>

<template>
  <div
    class="max-w-[400px] min-w-[300px] sm:min-w-[400px] min-h-[300px] flex flex-col border p-16 m-24 rounded-md card-color light-effect"
  >
    <div class="flex justify-between gap-10">
      <h2>{{ props.name }}</h2>
      <div class="flex gap-12 items-center">
        <BasicTooltip v-if="lastStatusMessage">
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
      <div class="w-1/2 flex flex-col items-center gap-6">
        <span>{{ t('common.NetworksNearby') }}</span>
        <span>{{ lastWifiScanTimestamp }}</span>
        <div class="flex flex-col gap-6 w-full">
          <ul class="flex flex-col gap-6">
            <li
              v-for="(wifi, index) in lastWifiScanNetworks.slice(0, 5)"
              :key="index"
            >
              <BasicTooltip class="flex gap-12">
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
                <div class="truncate">
                  {{ wifi.ssid }}
                  <span
                    v-if="
                      lastStatusMessage?.wifi === wifi.ssid &&
                      ['online', 'error'].includes(deviceStatus)
                    "
                    class="text-success-active"
                  >
                    ({{ t("common.connected") }})
                  </span>
                </div>

                <template #tooltipText>
                  <div class="flex gap-6">
                    <span>RSSI: {{ wifi.rssi }} dBm</span>
                    |
                    <span>Encryption: {{ wifi.encryption }}</span>
                  </div>
                </template>
              </BasicTooltip>
            </li>
          </ul>

          <div v-if="lastWifiScanNetworks.length > 5">
            <span class="whitespace-nowrap">
              {{
                t("common.andValueMore", {
                  count: lastWifiScanNetworks.length - 5,
                })
              }}
            </span>
          </div>
        </div>
      </div>
      <div class="w-1/2 flex flex-col items-center gap-6">
        <span>GPIO</span>
        <div class="w-full flex justify-center items-center relative">
          <span>{{ lastGpioStatesTimestamp }}</span>
          <BasicSpinner
            v-if="isLoadingGpioStates"
            class="absolute right-0"
            :size="12"
          />
        </div>

        <ul class="flex flex-col gap-6">
          <li
            v-for="(value, pin) in gpioPinStates"
            :key="pin"
            class="flex gap-12 justify-between items-center"
          >
            <span class="whitespace-nowrap">Pin {{ pin }}</span>

            <div class="flex items-center justify-between text-10">
              <button
                class="flex items-center justify-center w-24 h-20 rounded-l-md hover:text-success-active"
                :class="{
                  'bg-success': value === 1,
                  'bg-gray-300': value === 0,
                  'pointer-events-none':
                    isLoadingGpioStates ||
                    value === null ||
                    value === undefined ||
                    value === 1,
                  'text-success-active':
                    isLoadingGpioStates === Number(pin) && value === 0,
                }"
                @click="setGpioPinState(Number(pin), 1)"
              >
                ON
              </button>
              <button
                class="flex items-center justify-center w-24 h-20 rounded-r-md hover:text-error"
                :class="{
                  'bg-error': value === 0,
                  'bg-gray-300': value === 1,
                  'pointer-events-none':
                    isLoadingGpioStates ||
                    value === null ||
                    value === undefined ||
                    value === 0,
                  'text-error':
                    isLoadingGpioStates === Number(pin) && value === 1,
                }"
                @click="setGpioPinState(Number(pin), 0)"
              >
                OFF
              </button>
            </div>
          </li>
        </ul>
      </div>
    </div>
  </div>
</template>
