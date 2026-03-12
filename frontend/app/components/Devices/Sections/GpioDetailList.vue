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
  setGpioConfigs: [{ deviceId: string; gpioConfigs: Partial<GPIO>[] }];
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

const isUpdatingGpioStates = ref<null | GPIOPin | -1>(null);
const gpioPinStates = ref<GPIO[]>([]);

const valuesAreValid = computed(() => {
  return true;
  // return !gpioPinStates.value.find((gpio) => gpio.state === null);
});
const valuesAreChanged = computed(() => {
  return (
    !isUpdatingGpioStates.value &&
    JSON.stringify(props.gpios) !== JSON.stringify(gpioPinStates.value)
  );
});

function saveChanges() {
  if (isUpdatingGpioStates.value) return;
  isUpdatingGpioStates.value = -1;

  emit("setGpioConfigs", {
    deviceId: props.deviceId,
    gpioConfigs: getChanges(),
  });
}

function cloneGpioStates() {
  gpioPinStates.value = JSON.parse(JSON.stringify(props.gpios));
}

function stopGettingGpioStates() {
  isUpdatingGpioStates.value = null;
}

function setGpioPinState(pin: GPIOPin, value: GPIOPinState) {
  if (isUpdatingGpioStates.value) return;
  isUpdatingGpioStates.value = pin;

  emit("setGpioPin", { deviceId: props.deviceId, pin, value });
}

function getChanges() {
  const originalValues = props.gpios;
  const currentValues = gpioPinStates.value;

  const changedGpios: Partial<GPIO>[] = [];
  currentValues.forEach((gpio) => {
    const originalGpio = originalValues.find(
      (g) => g.pinNumber === gpio.pinNumber,
    );
    if (originalGpio) {
      if (
        originalGpio.label !== gpio.label ||
        originalGpio.mode !== gpio.mode
      ) {
        const change: Partial<GPIO> = {
          pinNumber: gpio.pinNumber,
        };
        if (gpio.label && originalGpio.label !== gpio.label) {
          change.label = gpio.label;
        }
        if (gpio.mode && originalGpio.mode !== gpio.mode) {
          change.mode = gpio.mode;
        }
        changedGpios.push(change);
      }
    }
  });

  return changedGpios;
}

watch(
  () => props.gpios,
  (newVal) => {
    if (isUpdatingGpioStates.value && isUpdatingGpioStates.value !== -1) {
      const gpio = newVal.find(
        (g) => g.pinNumber === isUpdatingGpioStates.value,
      );
      toast.success({
        title: t("device.setGpio.successText", {
          pinName: gpio?.label || "PIN " + isUpdatingGpioStates.value,
          state: gpio?.state ? t("common.activated") : t("common.deactivated"),
        }),
        message: props.deviceName,
      });
    }

    if (isUpdatingGpioStates.value === -1)
      toast.success({
        title: props.deviceName,
        message: t("common.saveSuccessfully"),
      });

    cloneGpioStates();
    stopGettingGpioStates();
  },
  { deep: true },
);

// Lifecycle
onMounted(() => {
  cloneGpioStates();
});

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
        class="flex items-center gap-12 py-8 border-b last:border-0 px-16"
      >
        <div class="w-[100px]">
          <span class="whitespace-nowrap">Pin {{ gpio.pinNumber }}</span>
        </div>

        <div class="w-[200px]">
          <input
            :id="`gpio-label-input-${gpio.pinNumber}`"
            v-model="gpio.label"
            type="text"
          />
        </div>

        <div class="w-[150px]">
          <label :for="`gpio-mode-select-${gpio.pinNumber}`">
            {{ t("device.gpioMode") }}
          </label>

          {{ gpio.mode }} - {{ JSON.stringify(gpio) }}

          <select
            v-model="gpio.mode"
            name="modes"
            :id="`gpio-mode-select-${gpio.pinNumber}`"
          >
            <option value="none">{{ t("common.deactivated") }}</option>
            <optgroup :label="t('device.actor.actor')">
              <option
                v-for="mode in gpioModesActor"
                :key="mode.value"
                :value="mode.value"
              >
                {{ t(mode.i18nKey) }}
              </option>
            </optgroup>
            <optgroup :label="t('device.sensor.sensor')">
              <option
                v-for="mode in gpioModesSensor"
                :key="mode.value"
                :value="mode.value"
              >
                {{ t(mode.i18nKey) }}
              </option>
            </optgroup>
          </select>
        </div>
        <div class="flex items-center justify-between text-10 ml-auto">
          <button
            class="flex items-center justify-center w-24 h-20 rounded-l-md hover:text-success-active"
            :class="{
              'bg-success': gpio.state === 1,
              'bg-gray-300': gpio.state === 0,
              'pointer-events-none':
                isUpdatingGpioStates ||
                gpio.state === null ||
                gpio.state === undefined ||
                gpio.state === 1 ||
                deviceStatus !== 'online',
              'opacity-50': deviceStatus !== 'online',
              'text-success-active':
                isUpdatingGpioStates === Number(gpio.pinNumber) &&
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
                isUpdatingGpioStates ||
                gpio.state === null ||
                gpio.state === undefined ||
                gpio.state === 0 ||
                deviceStatus !== 'online',
              'opacity-50': deviceStatus !== 'online',
              'text-error':
                isUpdatingGpioStates === Number(gpio.pinNumber) &&
                gpio.state === 1,
            }"
            @click="setGpioPinState(Number(gpio.pinNumber), 0)"
          >
            OFF
          </button>
        </div>
      </li>
    </ul>
    <div class="mt-auto flex justify-end pb-6 mx-12">
      {{ valuesAreChanged }}
      {{ valuesAreValid }}
      <button
        class="px-12 py-6 border rounded"
        :class="
          valuesAreChanged && valuesAreValid
            ? 'border-success hover:border-success-hover active:border-success-active'
            : 'border-border pointer-events-none'
        "
        @click="saveChanges()"
      >
        {{ t("common.save") }}
      </button>
    </div>
  </div>
</template>
