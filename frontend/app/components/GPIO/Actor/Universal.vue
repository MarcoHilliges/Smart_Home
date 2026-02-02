<script setup lang="ts">
import type { ExtendedGPIO, GPIOPin, GPIOPinState } from "~/models/device";
import { Power } from "lucide-vue-next";

const emits = defineEmits<{
  setGpioPin: [{ deviceId: string; pin: GPIOPin; value: GPIOPinState }];
}>();

const props = defineProps<{
  gpio: ExtendedGPIO;
  cardWidth?: number | string;
  cardHeight?: number | string;
}>();

const toast = useToast();
const { t } = useI18n();

const dimensions = computed(() => {
  return {
    width: props.cardWidth
      ? typeof props.cardWidth === "number"
        ? `${props.cardWidth}px`
        : props.cardWidth
      : "100px",
    height: props.cardHeight
      ? typeof props.cardHeight === "number"
        ? `${props.cardHeight}px`
        : props.cardHeight
      : "100px",
  };
});

const gpioState = computed(() => {
  return !!props.gpio.state;
});

const isChangingState = ref<boolean>(false);

function setGpioPin() {
  if (isChangingState.value) return;
  isChangingState.value = true;
  emits("setGpioPin", {
    deviceId: props.gpio.deviceId,
    pin: props.gpio.pinNumber,
    value: gpioState.value ? 0 : 1,
  });
}

watch(
  () => props.gpio.state,
  () => {
    isChangingState.value = false;
          toast.success({
        title: props.gpio.deviceName,
        message: t("device.setGpio.successText", {
          pinName: "PIN " + props.gpio.pinNumber,
          state: props.gpio.state
            ? t("common.activated")
            : t("common.deactivated"),
        }),
      });
  },
);
</script>

<template>
  <BasicTooltip
    :tooltip-text="
      props.gpio.deviceStatus !== 'online'
        ? t(`common.status.${props.gpio.deviceStatus}`)
        : ''
    "
  >
    <BasicCard
      :light="gpioState && isChangingState === false"
      light-color="green"
      :style="{ width: dimensions.width, height: dimensions.height }"
      class="flex justify-center items-center"
      :class="{
        'opacity-70 pointer-events-none': props.gpio.deviceStatus === 'offline',
      }"
    >
      <BasicCardButton
        :is-active="gpioState"
        :is-selectable="true"
        general-classes="w-full h-full"
        active-classes="text-success"
        @click="setGpioPin()"
      >
        <template #top>
          <span class="text-12 text-primary">
            {{ props.gpio.label || `PIN ${props.gpio.pinNumber}` }}
          </span>
        </template>

        <Power :size="30" />

        <template #bottom>
          <span class="text-8 text-primary">
            {{ props.gpio.deviceName }}
          </span>
        </template>
      </BasicCardButton>
    </BasicCard>
  </BasicTooltip>
</template>
