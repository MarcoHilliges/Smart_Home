<script setup lang="ts">
const props = defineProps<{
  status?: 0 | 1;
}>();

const indicatorColor = {
  0: "#D5D5E2",
  1: "#15D3A5",
};

const cardShadow = computed(() => {
  const shadow = ``;
  if (props.status === 1) {
    return `0 0px 8px 2px ${indicatorColor[props.status]}`;
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
