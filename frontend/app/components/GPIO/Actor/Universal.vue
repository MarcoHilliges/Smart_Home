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
</script>

<template>
  <BasicCard
    :device-status="props.gpio.deviceStatus"
    :style="{ width: dimensions.width, height: dimensions.height }"
    class="flex justify-center items-center"
  >
    <BasicCardButton
      :is-active="gpioState"
      :is-selectable="true"
      general-classes="w-full h-full"
      active-classes="text-success"
      @click="
        $emit('setGpioPin', {
          deviceId: props.gpio.deviceId,
          pin: props.gpio.pinNumber,
          value: gpioState ? 0 : 1,
        })
      "
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
</template>
