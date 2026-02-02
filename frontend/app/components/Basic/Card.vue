<script setup lang="ts">
const props = defineProps<{
  light?: boolean;
  lightColor?: "gray" | "blue" | "green" | "yellow" | "red";
}>();

const indicatorColor = {
  gray: "#D5D5E2",
  green: "#15D3A5",
  blue: "#1489FE",
  yellow: "#EB7F00",
  red: "#E51F2B",
};

const cardShadow = computed(() => {
  const shadow = ``;
  if (props.light) {
    return `0 0px 8px 2px ${indicatorColor[props.lightColor || "gray"]}`;
  }
  return shadow;
});
</script>

<template>
  <div
    class="rounded-md card-color light-effect"
    :style="{ boxShadow: cardShadow }"
  >
    <slot>Content</slot>
  </div>
</template>
