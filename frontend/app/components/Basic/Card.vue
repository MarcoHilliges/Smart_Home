<script setup lang="ts">
import type { DeviceStatus, GPIOPinState } from "~/models/device";

const props = defineProps<{
  status?: GPIOPinState;
}>();

const { $mqttConnectionState } = useNuxtApp();

const indicatorColor = {
  0: "#D5D5E2",
  1: "#15D3A5",
};

const cardShadow = computed(() => {
  const shadow = `0 0px 12px 2px ${indicatorColor[0]}`;
  const clientState = $mqttConnectionState.value;
  if (props.status !== undefined) {
    return `0 0px 12px 2px ${clientState === "connected" ? indicatorColor[props.status] : indicatorColor[0]}`;
  }
  return shadow;
});
</script>

<template>
  <div
    class="rounded-md card-color light-effect"
    :style="{ boxShadow: cardShadow }"
  >
    <slot>Content</slot>
  </div>
</template>
