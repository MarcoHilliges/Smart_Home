<script setup lang="ts">
const props = defineProps<{
  isActive: boolean;
  isSelectable: boolean;
  generalClasses?: string;
  activeClasses?: string;
  selectableClasses?: string;
}>();

const cssClasses = computed(() => {
  let classes = props.generalClasses || "";
  if (props.isSelectable)
    classes += ` selectable ${props.selectableClasses || ""}`;
  else classes += ` pointer-events-none`;
  if (props.isActive) classes += ` ${props.activeClasses || ""}`;
  return classes;
});
</script>

<template>
  <button class="relative" :class="cssClasses">
    <div v-if="$slots.top" class="absolute top-8 left-0 right-0">
      <slot name="top"></slot>
    </div>
    <slot></slot>
    <div v-if="$slots.bottom" class="absolute bottom-8 left-0 right-0">
      <slot name="bottom"></slot>
    </div>
  </button>
</template>

<style lang="scss" scoped>
button {
  transition: all 300ms;

  @apply border-primary flex justify-center items-center;

  &.selectable {
    @apply hover:bg-blue-200 dark:hover:bg-blue-100;
  }
}
</style>
