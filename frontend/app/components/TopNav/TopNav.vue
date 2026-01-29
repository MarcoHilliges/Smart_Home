<script lang="ts" setup>
const props = defineProps({
  showNav: Boolean,
});
const { t } = useI18n();
</script>

<template>
  <div
    class="flex items-center justify-between py-12 sm:px-24 border-b border-primary dark:border-primary-light h-[72px]"
  >
    <div class="flex items-center gap-24 h-full">
      <NuxtLink to="/">
        <img
          class="h-[60px] dark:hidden"
          src="~/assets/images/logo.png"
          alt="Logo"
        />
        <img
          class="h-[60px] hidden dark:block"
          src="~/assets/images/logo-dark.png"
          alt="Logo"
        />
      </NuxtLink>

      <TopNavClientConnectionState />
    </div>

    <div class="flex gap-24 items-center h-full">
      <TopNavLanguageSwitches />

      <TopNavThemeSwitches />
    </div>
  </div>
</template>
