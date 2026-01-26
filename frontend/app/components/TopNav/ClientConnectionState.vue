<script setup lang="ts">
import type { MqttClientState } from "~/models/mqtt";

const { t } = useI18n();

const { $mqttConnectionState } = useNuxtApp();
const mqttClientState = $mqttConnectionState;
const toast = useToast();

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

function showMqttClientState(value: MqttClientState) {
  switch (value) {
    case "connected":
      toast.success({
        title: t("common.mqttClientState.title"),
        message: t(`common.mqttClientState.${value}`),
      });
      break;
    case "error":
      toast.error({
        title: t("common.mqttClientState.title"),
        message: t(`common.mqttClientState.${value}`),
      });
      break;
    case "disconnected":
      toast.info({
        title: t("common.mqttClientState.title"),
        message: t(`common.mqttClientState.${value}`),
      });
      break;
    case "reconnecting":
      toast.info({
        title: t("common.mqttClientState.title"),
        message: t(`common.mqttClientState.${value}`),
      });
  }
}

watch(
  () => mqttClientState.value,
  (newValue) => {
    showMqttClientState(newValue);
  },
);

onMounted(() => {
  // Verhindert doppelte Nachricht
  setTimeout(() => showMqttClientState(mqttClientState.value), 300);
});
</script>

<template>
  <BasicTooltip :tooltipText="connectionStateText" class="flex items-center gap-8">
    Client Status:
    <div class="w-12 h-12 rounded-full" :class="connectionStateColor"></div>
  </BasicTooltip>
</template>
