<script lang="ts" setup>
const props = defineProps({
  showNav: Boolean,
});
const { t } = useI18n();
</script>

<template>
  <div class="fixed top-0 left-0 right-0 z-50">
    <div
      class="flex items-center justify-between sm:px-24 py-16 bg-page50 dark:bg-pageDark50 gradient"
    >
      <div class="md:w-1/4">
        <NuxtLink to="/">
          <img
            class="h-32 sm:h-[60px] dark:hidden"
            src="~/assets/images/logo.png"
            alt="Logo"
          />
          <img
            class="h-32 sm:h-[60px] hidden dark:block"
            src="~/assets/images/logo-dark.png"
            alt="Logo"
          />
        </NuxtLink>
      </div>

      <TopNavNavigation v-if="props.showNav" />

      <div
        class="flex gap-6 items-center h-full"
      >
        <TopNavClientConnectionState />

        <TopNavLanguageSwitches />

        <TopNavThemeSwitches />
      </div>
    </div>
  </div>
</template>

<style lang="scss">
.gradient {
  -webkit-mask-image: linear-gradient(
    to top,
    rgba(255, 250, 235, 0) 0%,
    rgba(255, 250, 235, 0.8) 16px,
    rgba(255, 250, 235, 1) 24px
  );
  mask-image: linear-gradient(
    to top,
    rgba(255, 250, 235, 0) 0%,
    rgba(255, 250, 235, 0.8) 16px,
    rgba(255, 250, 235, 1) 24px
  );
}
</style>
