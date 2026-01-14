<script setup lang="ts">
import { WifiZero, WifiLow, WifiHigh, Wifi } from "lucide-vue-next";
import type { DeviceStatus, WifiScanMessage } from "~/models/message";

const { t } = useI18n();
const props = defineProps<{
  wiFiScanMessages: WifiScanMessage[];
  deviceStatus: DeviceStatus;
  connectedWiFi: string;
}>();

const lastWifiScan = computed(() => {
  return props.wiFiScanMessages?.[0];
});

const isLoadingWifiScan = ref(false);
const lastWifiScanTimestamp = computed(() => {
  return formatTimestamp(lastWifiScan.value?.timestamp);
});

const lastWifiScanNetworks = computed(() => {
  return lastWifiScan.value
    ? lastWifiScan.value.networks.sort((a, b) => b.rssi - a.rssi)
    : [];
});

// Helpers
function formatTimestamp(timestamp: number | undefined | null) {
  if (!timestamp) return "-";
  const date = new Date(timestamp);
  return date.toLocaleString();
}
</script>

<template>
  <div class="flex flex-col items-center gap-6">
    <span>{{ t("common.NetworksNearby") }}</span>
    <div class="flex items-center">
      <span>{{ lastWifiScanTimestamp }}</span>
      <BasicSpinner v-if="isLoadingWifiScan" :size="12" />
    </div>

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
                  props.connectedWiFi === wifi.ssid &&
                  ['online', 'error'].includes(props.deviceStatus)
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
</template>
