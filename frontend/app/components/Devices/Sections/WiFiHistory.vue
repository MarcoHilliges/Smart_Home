<script setup lang="ts">
import { WifiZero, WifiLow, WifiHigh, Wifi } from "lucide-vue-next";
import type { WifiScanMessage } from "~/models/message";

const props = defineProps<{
  deviceId: string;
  wiFiScanMessages: WifiScanMessage[];
}>();

const currentListTimestamp = ref(
  props.wiFiScanMessages?.[0]?.timestamp || null,
);

const currentList = computed(() => {
  return (
    props.wiFiScanMessages?.find(
      (msg) => msg.timestamp === currentListTimestamp.value,
    ) || null
  );
});

const tabs = computed(() => {
  return props.wiFiScanMessages
    .map((msg) => ({
      label: new Date(msg.timestamp).toLocaleString(),
      value: msg.timestamp,
    }))
    .sort((a, b) => b.value - a.value);
});

watch(
  () => props.deviceId,
  () => {
    currentListTimestamp.value = props.wiFiScanMessages?.[0]?.timestamp || null;
  },
);
</script>

<template>
  <div class="flex flex-col h-full">
    <div class="flex flex-col p-16 border-b border-primary">
      <h2>
        {{ $t("device.tabs.wifi") }}
      </h2>
      <div>
        <BasicTooltip
          v-for="(tab, index) in tabs"
          :key="index"
          :tooltipText="tab.label"
          :style="'width: ' + 100 / tabs.length + '%;'"
          :class="{
            'border-b border-primary': currentListTimestamp === tab.value,
            'text-gray-500': currentListTimestamp !== tab.value,
          }"
        >
          <button class="w-full" @click="currentListTimestamp = tab.value">
            {{ index + 1 }}
          </button>
        </BasicTooltip>
      </div>
    </div>
    <ul class="flex-grow overflow-y-auto custom-scrollbar">
      <li
        v-for="(network, index) in currentList?.networks || []"
        :key="index"
        class="flex items-center gap-12 py-8 border-b last:border-0 px-16"
      >
        <component
          :is="
            network.rssi >= -50
              ? Wifi
              : network.rssi < -50 && network.rssi >= -65
                ? WifiHigh
                : network.rssi < -65 && network.rssi >= -75
                  ? WifiLow
                  : WifiZero
          "
          class="w-16 h-16"
        />
        <div class="truncate">
          <span class="ml-12"
            >{{ network.ssid }} | {{ network.rssi }} dBm | Encryption:
            {{ network.encryption }}</span
          >
        </div>
      </li>
    </ul>
  </div>
</template>
