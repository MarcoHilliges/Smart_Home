<script setup lang="ts">
import { computed } from "vue";

const props = defineProps({
  tooltipText: {
    type: String,
    default: "",
  },
  position: {
    type: String,
    default: "top",
    validator: (value: string) =>
      ["top", "bottom", "left", "right"].includes(value),
  },
});

const positionClasses = computed(() => {
  switch (props.position) {
    case "top":
      return "bottom-full left-1/2 -translate-x-1/2 mb-2";
    case "bottom":
      return "top-full left-1/2 -translate-x-1/2 mt-2";
    case "left":
      return "right-full top-1/2 -translate-y-1/2 mr-2";
    case "right":
      return "left-full top-1/2 -translate-y-1/2 ml-2";
    default:
      return "bottom-full left-1/2 -translate-x-1/2 mb-2";
  }
});
</script>

<template>
  <div
    class="relative inline-block group"
    :class="{ 'cursor-pointer': tooltipText || $slots.tooltipText }"
  >
    <slot></slot>

    <div
      v-if="tooltipText || $slots.tooltipText"
      class="absolute hidden group-hover:block px-3 py-1 text-sm text-white bg-gray-700 rounded-md whitespace-nowrap z-50 transition-opacity duration-300 opacity-0 group-hover:opacity-100"
      :class="positionClasses"
    >
      <slot name="tooltipText">
        {{ tooltipText }}
      </slot>
    </div>
  </div>
</template>
