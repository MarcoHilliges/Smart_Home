<script setup>
const { locale, locales, setLocale } = useI18n();

const availableLocales = computed(() => {
  return locales.value.filter((i) => i.code !== locale.value);
});
</script>

<template>
  <BasicCard class="flex justify-center w-[100px]">
    <BasicCardButton
      v-for="language in locales"
      :key="language.code"
      class="border-r last:border-none"
      :is-active="locale === language.code"
      :is-selectable="locale !== language.code"
      general-classes="w-1/3 text-10"
      :active-classes="`!w-2/3 font-bold text-16`"
      @click="setLocale(language.code)"
    >
      {{ language.code.toUpperCase() }}
    </BasicCardButton>
  </BasicCard>
</template>
