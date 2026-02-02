<script setup lang="ts">
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

const componentRootRef = ref<HTMLElement | null>(null);
const tooltipRef = ref<HTMLElement | null>(null);

const showTooltip = ref(false);
let mouseEnterTimeout: ReturnType<typeof setTimeout> | null = null;
let mouseLeaveTimeout: ReturnType<typeof setTimeout> | null = null;

const tooltipStyle = ref({});

const updateTooltipPosition = () => {
  if (!componentRootRef.value || !tooltipRef.value || !showTooltip.value)
    return;

  const triggerRect = componentRootRef.value.getBoundingClientRect();
  const tooltipRect = tooltipRef.value.getBoundingClientRect();

  let top = 0;
  let left = 0;

  switch (props.position) {
    case "top":
      top = triggerRect.top - tooltipRect.height - 4;
      left = triggerRect.left + triggerRect.width / 2 - tooltipRect.width / 2;
      break;
    case "bottom":
      top = triggerRect.bottom + 4;
      left = triggerRect.left + triggerRect.width / 2 - tooltipRect.width / 2;
      break;
    case "left":
      top = triggerRect.top + triggerRect.height / 2 - tooltipRect.height / 2;
      left = triggerRect.left - tooltipRect.width - 4;
      break;
    case "right":
      top = triggerRect.top + triggerRect.height / 2 - tooltipRect.height / 2;
      left = triggerRect.right + 4;
      break;
  }

  if (top < 0) top = triggerRect.bottom + 4;
  if (left < 0) left = 0;
  if (left + tooltipRect.width > window.innerWidth)
    left = window.innerWidth - tooltipRect.width;

  tooltipStyle.value = {
    position: "absolute",
    top: `${top + window.scrollY}px`,
    left: `${left + window.scrollX}px`,
    opacity: showTooltip.value ? "1" : "0",
    pointerEvents: showTooltip.value ? "auto" : "none",
  };
};

const handleMouseEnter = () => {
  if (mouseLeaveTimeout) clearTimeout(mouseLeaveTimeout);
  mouseEnterTimeout = setTimeout(() => {
    showTooltip.value = !!props.tooltipText;
    nextTick(() => {
      updateTooltipPosition();
    });
  }, 100);
};

const handleMouseLeave = () => {
  if (mouseEnterTimeout) clearTimeout(mouseEnterTimeout);
  mouseLeaveTimeout = setTimeout(() => {
    showTooltip.value = false;
  }, 100);
};

onMounted(() => {
  if (componentRootRef.value) {
    componentRootRef.value.addEventListener("mouseenter", handleMouseEnter);
    componentRootRef.value.addEventListener("mouseleave", handleMouseLeave);
    window.addEventListener("scroll", updateTooltipPosition);
    window.addEventListener("resize", updateTooltipPosition);
  }
});

onUnmounted(() => {
  if (componentRootRef.value) {
    componentRootRef.value.removeEventListener("mouseenter", handleMouseEnter);
    componentRootRef.value.removeEventListener("mouseleave", handleMouseLeave);
    window.removeEventListener("scroll", updateTooltipPosition);
    window.addEventListener("resize", updateTooltipPosition);
  }
  if (mouseEnterTimeout) clearTimeout(mouseEnterTimeout);
  if (mouseLeaveTimeout) clearTimeout(mouseLeaveTimeout);
});
</script>

<template>
  <div class="relative inline-block" ref="componentRootRef">
    <slot></slot>

    <Teleport to="body">
      <div
        v-if="showTooltip"
        ref="tooltipRef"
        class="absolute px-4 py-2 text-sm text-white bg-gray-700 rounded-md whitespace-nowrap z-[9999]"
        :style="tooltipStyle"
      >
        <slot name="tooltipText">
          {{ tooltipText }}
        </slot>
      </div>
    </Teleport>
  </div>
</template>
