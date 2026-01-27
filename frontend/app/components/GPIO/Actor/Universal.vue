<script setup lang="ts">
import type { ExtendedGPIO, GPIOPin } from "~/models/device";
import { Power } from "lucide-vue-next";

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
</script>

<template>
  <BasicCard
    :style="{ width: dimensions.width, height: dimensions.height }"
    class="flex justify-center items-center"
  >
    <BasicCardButton
      class="border-r last:border-none"
      :is-active="false"
      :is-selectable="true"
      general-classes="w-full h-full relative"
      @click="console.log('Clicked GPIO Button')"
    >
      <span class="absolute top-8 left-0 right-0 text-12">
        {{ props.gpio.label || `PIN ${props.gpio.pinNumber}` }}
      </span>

      <Power :size="24" />

      <span class="absolute bottom-8 left-0 right-0 text-8">
        {{ props.gpio.deviceName }}
      </span>
    </BasicCardButton>
  </BasicCard>
</template>
