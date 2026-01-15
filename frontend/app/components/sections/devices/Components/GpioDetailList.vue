<script setup lang="ts">
import type { DeviceStatus, GPIOPin, GPIOPinState, GPIOStateMessage } from "~/models/message";

const { t } = useI18n();

const emit = defineEmits<{
  setGpioPin: [{ pin: GPIOPin; value: GPIOPinState }];
  getGpioStates: [];
}>();

const props = defineProps<{
  gpioStateMessages: GPIOStateMessage[];
  deviceStatus: DeviceStatus;
}>();

const lastGpioStatesMessage = computed(() => {
  return props.gpioStateMessages?.[0] || null;
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

watch(
  () => props.deviceStatus,
  (newVal) => {
    if (
      newVal === "online" &&
      (lastGpioStatesMessage.value === null ||
        Date.now() - (lastGpioStatesMessage.value?.timestamp || 60000) > 45000)
    ) {
      isLoadingGpioStates.value = -1;
      emit("getGpioStates");
    }
  },
  { immediate: true }
);

// Helpers
function formatTimestamp(timestamp: number | undefined | null) {
  if (!timestamp) return "-";
  const date = new Date(timestamp);
  return date.toLocaleString();
}
</script>

<template>
  <div class="flex flex-col h-full">
    <div class="w-full flex justify-center items-center relative">
      <span>{{ t('common.lastUpdate') }}: {{ lastGpioStatesTimestamp }}</span>
      <BasicSpinner
        v-if="isLoadingGpioStates"
        class="absolute right-0"
        :size="12"
      />
    </div>

    <ul class="overflow-y-auto">
      <li
        v-for="(value, pin) in gpioPinStates"
        :key="pin"
        class="flex items-center justify-between gap-12 py-8 border-b last:border-0 px-16"
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
                value === 1 ||
                deviceStatus !== 'online',
              'opacity-50': deviceStatus !== 'online',
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
                value === 0 ||
                deviceStatus !== 'online',
              'opacity-50': deviceStatus !== 'online',
              'text-error': isLoadingGpioStates === Number(pin) && value === 1,
            }"
            @click="setGpioPinState(Number(pin), 0)"
          >
            OFF
          </button>
        </div>
      </li>
    </ul>
  </div>
</template>
