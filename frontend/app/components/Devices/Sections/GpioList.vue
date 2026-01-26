<script setup lang="ts">
import type { GPIO, GPIOPin, GPIOPinState } from "~/models/device";
import type { GPIOStateMessage } from "~/models/message";

const emit = defineEmits<{
  setGpioPin: [{ pin: GPIOPin; value: GPIOPinState }];
  getGpioStates: [];
}>();

const props = defineProps<{
  deviceName: string;
  gpios: GPIO[];
  gpioStateMessages: GPIOStateMessage[];
  deviceStatus: string;
}>();

const toast = useToast();
const { t } = useI18n();

// TODO: Funktionalität zusammenfügen und auslagern => useDeviceGpioControl()
const lastGpioStatesMessage = computed(() => {
  return props.gpioStateMessages?.[0] || null;
});

const lastGpioStatesTimestamp = computed(() => {
  return lastGpioStatesMessage.value?.timestamp;
});

const formatedLastGpioStateTimestamp = computed(() => {
  return formatTimestamp(lastGpioStatesTimestamp.value);
});

const isLoadingGpioStates = ref<null | GPIOPin | -1>(null);
const gpioPinStates = ref<GPIO[]>(props.gpios);

let intervalGetGpioStates: NodeJS.Timeout | null = null;

function getGpioStates() {
  emit("getGpioStates");
}

function startGettingGpioStates() {
  if (!intervalGetGpioStates) {
    isLoadingGpioStates.value = -1;
    intervalGetGpioStates = setInterval(getGpioStates, 5000);
  }
}

function stopGettingGpioStates() {
  if (intervalGetGpioStates) {
    clearInterval(intervalGetGpioStates);
  }
  intervalGetGpioStates = null;
  isLoadingGpioStates.value = null;
}

function setGpioPinState(pin: GPIOPin, value: GPIOPinState) {
  if (isLoadingGpioStates.value) return;
  isLoadingGpioStates.value = pin;

  emit("setGpioPin", { pin, value });
}

watch(
  () => lastGpioStatesTimestamp.value,
  () => {
    if (lastGpioStatesMessage.value?.gpioStates)
    gpioPinStates.value = {...lastGpioStatesMessage.value.gpioStates};

    if (isLoadingGpioStates.value && isLoadingGpioStates.value !== -1) {
      toast.success({
        title: props.deviceName,
        message: t("device.setGpio.successText", {
          pinName: "PIN " + isLoadingGpioStates.value,
          state: gpioPinStates.value?.[isLoadingGpioStates.value]
            ? t("common.activated")
            : t("common.deactivated"),
        }),
      });
    }

    stopGettingGpioStates();
  },
  { immediate: true },
);

watch(
  () => props.deviceStatus,
  (newVal) => {
    if (
      newVal === "online" &&
      (lastGpioStatesMessage.value === null ||
        Date.now() - (lastGpioStatesMessage.value?.timestamp || 60000) > 45000)
    ) {
      startGettingGpioStates();
    }
  },
  { immediate: true },
);

// Helpers
function formatTimestamp(timestamp: number | undefined | null) {
  if (!timestamp) return "-";
  const date = new Date(timestamp);
  return date.toLocaleString();
}

// Lifecycle
onBeforeUnmount(() => {
  stopGettingGpioStates();
});
</script>

<template>
  <div class="flex flex-col items-center gap-6">
    <span>GPIO</span>
    <div class="w-full flex justify-center items-center relative">
      <span>{{ formatedLastGpioStateTimestamp }}</span>
      <BasicSpinner
        v-if="isLoadingGpioStates"
        class="absolute right-0"
        :size="12"
      />
    </div>

    <ul class="flex flex-col gap-6">
      <li
        v-for="gpio in gpioPinStates"
        :key="gpio.pinNumber"
        class="flex gap-12 justify-between items-center"
      >
        <span class="whitespace-nowrap">Pin {{ gpio.pinNumber }}</span>

        <div class="flex items-center justify-between text-10">
          <button
            class="flex items-center justify-center w-24 h-20 rounded-l-md hover:text-success-active"
            :class="{
              'bg-success': gpio.state === 1,
              'bg-gray-300': gpio.state === 0,
              'pointer-events-none':
                isLoadingGpioStates ||
                gpio.state === null ||
                gpio.state === undefined ||
                gpio.state === 1 ||
                deviceStatus !== 'online',
              'opacity-50': deviceStatus !== 'online',
              'text-success-active':
                isLoadingGpioStates === Number(gpio.pinNumber) && gpio.state === 0,
            }"
            @click="setGpioPinState(Number(gpio.pinNumber), 1)"
          >
            ON
          </button>
          <button
            class="flex items-center justify-center w-24 h-20 rounded-r-md hover:text-error"
            :class="{
              'bg-error': gpio.state === 0,
              'bg-gray-300': gpio.state === 1,
              'pointer-events-none':
                isLoadingGpioStates ||
                gpio.state === null ||
                gpio.state === undefined ||
                gpio.state === 0 ||
                deviceStatus !== 'online',
              'opacity-50': deviceStatus !== 'online',
              'text-error': isLoadingGpioStates === Number(gpio.pinNumber) && gpio.state === 1,
            }"
            @click="setGpioPinState(Number(gpio.pinNumber), 0)"
          >
            OFF
          </button>
        </div>
      </li>
    </ul>
  </div>
</template>
