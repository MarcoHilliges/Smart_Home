<script setup lang="ts">
import type { GPIOPin } from "~/models/device";

const props = defineProps<{
  pinNumber: GPIOPin;
  label?: string;
  value: number | string;
  deviceId: string;
  deviceName: string;
}>();
</script>

<template>
  <BasicCard class="flex justify-center w-[150px]">
    <span>
      {{ props.label || `${props.deviceName} - GPIO ${props.pinNumber}` }}
    </span>
    <span>
        {{ props.value }}
    </span>
  </BasicCard>
</template>
