<script setup lang="ts">
import {
  MessageTopic,
  type DeviceStatus,
  type SettingsMessage,
} from "~/models/message";
import { Info } from "lucide-vue-next";

interface SettingsItem {
  key: keyof SettingsMessage;
  label: string;
  description: string;
  value: string | number | null;
  valueType: "string" | "number";
  inactive?: boolean;
}

const { $mqtt } = useNuxtApp();
const { t } = useI18n();

const props = defineProps<{
  deviceStatus: DeviceStatus;
  deviceId: string;
  deviceName: string;
}>();

const { updateDeviceName } = useDeviceStore();

const settingsItems = ref<SettingsItem[]>([
  {
    key: "deviceName",
    label: t("device.settings.deviceName"),
    description: t("device.settings.deviceNameDescription"),
    value: null,
    valueType: "string",
    inactive: true,
  },
  {
    key: "wifiScanInterval",
    label: t("device.settings.wifiScanInterval"),
    description: t("device.settings.wifiScanIntervalDescription"),
    value: null,
    valueType: "number",
  },
]);

const defaultValues = ref<SettingsItem[]>([]);

const valuesAreChanged = computed(() => {
  return (
    !isLoadingSettings.value &&
    JSON.stringify(defaultValues.value) !== JSON.stringify(settingsItems.value)
  );
});

const isLoadingSettings = ref(false);

onMounted(() => {
  $mqtt.on("message", (topic, message) => {
    const deviceIdAndName = topic.split("/")[1]; // Extrahiere die Geräte-ID aus dem Topic
    const topicType = topic.split("/")[2]; // Extrahiere den Nachrichtentyp aus dem Topic
    const last_dash_index = deviceIdAndName?.lastIndexOf("-");

    if (
      !last_dash_index ||
      !deviceIdAndName ||
      topicType !== MessageTopic.SETTINGS
    )
      return;

    const deviceId = deviceIdAndName.substring(last_dash_index + 1);

    if (deviceId !== props.deviceId) return;
    const messageData: SettingsMessage = JSON.parse(message.toString());

    settingsItems.value.forEach((item) => {
      const dataValue = messageData[item.key as keyof SettingsMessage];
      if (dataValue !== undefined && typeof dataValue === item.valueType) {
        switch (item.key) {
          case "deviceName":
            item.value = String(dataValue);
            updateDeviceName(props.deviceId, String(dataValue));
            break;

          default:
            item.value = dataValue;
        }
      }
    });

    defaultValues.value = JSON.parse(JSON.stringify(settingsItems.value));

    isLoadingSettings.value = false;
  });

  getSettings();
});

function getSettings() {
  isLoadingSettings.value = true;
  $mqtt.publish(`esp32/${props.deviceName}-${props.deviceId}/settings/get`, "");
}

function saveChanges() {
  if (!valuesAreChanged.value || isLoadingSettings.value) return;

  const topic = `esp32/${props.deviceName}-${props.deviceId}/settings/set`;
  let message: Partial<SettingsMessage> | null = null;
  settingsItems.value.forEach((item) => {
    if (!item.value) return;
    const defaultValue = defaultValues.value.find(
      ({ key }) => key === item.key
    )?.value;
    if (defaultValue && defaultValue !== item.value) {
      if (!message) message = { [item.key]: item.value };
      else {
        const key = item.key as keyof SettingsMessage;
        message[key] = item.value as any
      }
    }
  });

  isLoadingSettings.value = true;
  $mqtt.publish(topic, JSON.stringify(message));
}
</script>

<template>
  <div class="relative h-full flex flex-col">
    <ul
      class="overflow-y-auto"
      :class="{
        'opacity-70': isLoadingSettings || props.deviceStatus === 'offline',
      }"
    >
      <li
        v-for="item in settingsItems"
        :key="item.key"
        class="flex items-center gap-12 py-8 border-b last:border-0 px-16 text-12"
        :class="{ 'opacity-70': item.inactive }"
      >
        <span class="font-semibold">{{ item.label }}</span>
        <BasicTooltip :tooltipText="item.description">
          <Info class="w-14 h-14 text-gray-500" />
        </BasicTooltip>
        <span v-if="item.inactive">({{ t("common.commingSoon") }})</span>
        <input
          v-model="item.value"
          :type="item.valueType"
          class="w-[100px] ml-auto"
          :class="{ 'pointer-events-none': item.inactive }"
        />
      </li>
    </ul>
    <div class="mt-auto flex justify-end pb-6 mx-12">
      <button
        class="px-12 py-6 border rounded"
        :class="
          valuesAreChanged
            ? 'border-success hover:border-success-hover active:border-success-active'
            : 'border-border pointer-events-none'
        "
        @click="saveChanges()"
      >
        {{ t("common.save") }}
      </button>
    </div>
    <div
      v-if="isLoadingSettings || props.deviceStatus === 'offline'"
      class="absolute top-0 left-0 right-0 bottom-0 flex flex-col justify-center items-center"
    >
      <template v-if="isLoadingSettings">
        <BasicSpinner :size="24" />
        <span>{{ t("device.settings.loading") }}</span>
      </template>
      <template v-else>
        <span>{{ t("device.deviceIsOffline") }}</span>
      </template>
    </div>
  </div>
</template>
