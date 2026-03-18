<script setup lang="ts">
import type {
  DeviceStatus,
  DigitalState,
  GPIO,
  GPIOActor,
  GPIOPin,
  GPIORole,
  GPIOSensor,
} from "~/models/device";

const emit = defineEmits<{
  setGpioPin: [{ deviceId: string; pin: GPIOPin; value: DigitalState }];
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

const { gpioRolesActor, gpioRolesSensor } = useDeviceStore();
function getRoleOptions(roles: GPIORole[]) {
  const actorOptions: ActorOption[] = [];
  const sensorOptions: SensorOption[] = [];
  gpioRolesActor.forEach((role) => {
    if (roles.includes(role.value)) {
      actorOptions.push(role);
    }
  });
  gpioRolesSensor.forEach((role) => {
    if (roles.includes(role.value)) {
      sensorOptions.push(role);
    }
  });
  return { actorOptions, sensorOptions };
}

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

function setGpioPinState(pin: GPIOPin, value: DigitalState) {
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
        originalGpio.role !== gpio.role
      ) {
        const change: Partial<GPIO> = {
          pinNumber: gpio.pinNumber,
        };
        if (gpio.label && originalGpio.label !== gpio.label) {
          change.label = gpio.label;
        }
        if (gpio.role && originalGpio.role !== gpio.role) {
          change.role = gpio.role;
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

        <div class="w-[150px] flex">
          <select
            v-model="gpio.role"
            name="roles"
            :id="`gpio-role-select-${gpio.pinNumber}`"
          >
            <option value="None">{{ t("common.deactivated") }}</option>
            <optgroup
              v-if="getRoleOptions(gpio.roles).actorOptions.length"
              :label="t('device.actor.actor')"
            >
              <option
                v-for="role in getRoleOptions(gpio.roles).actorOptions"
                :key="role.value"
                :value="role.value"
              >
                {{ t(role.i18nKey) }}
              </option>
            </optgroup>
            <optgroup
              v-if="getRoleOptions(gpio.roles).sensorOptions.length"
              :label="t('device.sensor.sensor')"
            >
              <option
                v-for="role in getRoleOptions(gpio.roles).sensorOptions"
                :key="role.value"
                :value="role.value"
              >
                {{ t(role.i18nKey) }}
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
            @click="setGpioPinState(Number(gpio.pinNumber), 'HIGH')"
          >
            ON
          </button>
          <button
            class="flex items-center justify-center w-24 h-20 rounded-r-md hover:text-error"
            :class="{
              'bg-error': gpio.state === 'LOW',
              'bg-gray-300': gpio.state === 'HIGH',
              'pointer-events-none':
                isUpdatingGpioStates ||
                gpio.state === null ||
                gpio.state === undefined ||
                gpio.state === 'LOW' ||
                deviceStatus !== 'online',
              'opacity-50': deviceStatus !== 'online',
              'text-error':
                isUpdatingGpioStates === Number(gpio.pinNumber) &&
                gpio.state === 'HIGH',
            }"
            @click="setGpioPinState(Number(gpio.pinNumber), 'LOW')"
          >
            OFF
          </button>
        </div>
      </li>
    </ul>
    <div class="mt-auto flex justify-end pb-6 mx-12">
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
