<script setup lang="ts">
import type {
  DeviceStatus,
  GPIO,
  GPIOMode,
  GPIOPin,
  GPIOPinState,
} from "~/models/device";

const emit = defineEmits<{
  setGpioPin: [{ deviceId: string; pin: GPIOPin; value: GPIOPinState }];
  getGpioStates: [];
}>();

const props = defineProps<{
  deviceId: string;
  deviceName?: string;
  gpios: GPIO[];
  deviceStatus: DeviceStatus;
}>();

const toast = useToast();
const { t } = useI18n();

const { gpioModesActor, gpioModesSensor } = useDeviceStore();

const isLoadingGpioStates = ref<null | GPIOPin | -1>(null);
const gpioPinStates = ref<GPIO[]>(props.gpios);

function stopGettingGpioStates() {
  isLoadingGpioStates.value = null;
}

function setGpioPinState(pin: GPIOPin, value: GPIOPinState) {
  if (isLoadingGpioStates.value) return;
  isLoadingGpioStates.value = pin;

  emit("setGpioPin", { deviceId: props.deviceId, pin, value });
}

watch(
  () => gpioPinStates.value,
  (newVal) => {
    if (isLoadingGpioStates.value && isLoadingGpioStates.value !== -1) {
      const gpio = newVal.find(
        (g) => g.pinNumber === isLoadingGpioStates.value,
      );
      toast.success({
        title: props.deviceName,
        message: t("device.setGpio.successText", {
          pinName: "PIN " + isLoadingGpioStates.value,
          state: gpio?.state ? t("common.activated") : t("common.deactivated"),
        }),
      });
    }

    stopGettingGpioStates();
  },
  { immediate: true, deep: true },
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
  <div class="flex flex-col h-full">
    <ul class="overflow-y-auto custom-scrollbar">
      <li
        v-for="gpio in gpioPinStates"
        :key="gpio.pinNumber"
        class="flex items-center justify-between gap-12 py-8 border-b last:border-0 px-16"
      >
        <span class="whitespace-nowrap">Pin {{ gpio.pinNumber }}</span>

        <div>
          <input :value="gpio.label" type="text" />
        </div>

        <div>
          <label :for="`gpio-mode-select-${gpio.pinNumber}`">Choose a pet:</label>

          <select name="modes" :id="`gpio-mode-select-${gpio.pinNumber}`">
            <option value="none">--Please choose an option--</option>
            <optgroup label="Actor">
              <option
                v-for="mode in gpioModesActor"
                :key="mode.value"
                :value="mode.value"
              >
                {{ mode.label }}
              </option>
            </optgroup>
            <optgroup label="Sensor">
              <option
                v-for="mode in gpioModesSensor"
                :key="mode.value"
                :value="mode.value"
              >
                {{ mode.label }}
              </option>
            </optgroup>
          </select>
        </div>
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
                isLoadingGpioStates === Number(gpio.pinNumber) &&
                gpio.state === 0,
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
              'text-error':
                isLoadingGpioStates === Number(gpio.pinNumber) &&
                gpio.state === 1,
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
