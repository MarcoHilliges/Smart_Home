<script setup lang="ts">
const props = defineProps<{
  isActive: boolean;
  isSelectable: boolean;
  generalClasses: string;
  activeClasses?: string;
  selectableClasses?: string;
}>();

const cssClasses = computed(() => {
    let classes = props.generalClasses || ''
    if (props.isSelectable) classes += ` selectable ${props.selectableClasses}`
    if (props.isActive) classes += ` is-active ${props.activeClasses}`
    return classes
});
</script>

<template>
  <button
    :class="cssClasses"
  >
    <slot></slot>
  </button>
</template>

<style lang="scss" scoped>
button {
  transition: all 300ms;

  @apply border-primary flex justify-center items-center;

  &.selectable {
    @apply active:text-primary-active hover:text-primary-hover hover:bg-blue-200 dark:hover:bg-blue-100;
  }
  &.is-active {
    @apply pointer-events-none;
  }
}
</style>
