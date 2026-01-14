<script setup lang="ts">
const { t } = useI18n();

const { $mqttConnectionState } = useNuxtApp();
const mqttClientState = $mqttConnectionState;

const connectionStateText = computed(() => {
  switch (mqttClientState.value) {
    case "connected":
      return t("common.mqttClientState.connected");
    case "disconnected":
      return t("common.mqttClientState.disconnected");
    case "error":
      return t("common.mqttClientState.error");
    case "reconnecting":
      return t("common.mqttClientState.reconnecting");
    default:
      return t("common.mqttClientState.unknown");
  }
});

const connectionStateColor = computed(() => {
  switch (mqttClientState.value) {
    case "connected":
      return "bg-success";
    case "disconnected":
      return "bg-gray-500";
    case "error":
      return "bg-error";
    case "reconnecting":
      return "bg-warning";
    default:
      return "bg-gray-500";
  }
});
</script>

<template>
  <BasicTooltip :tooltipText="connectionStateText">
    <div class="w-12 h-12 rounded-full" :class="connectionStateColor"></div>
  </BasicTooltip>
</template>
