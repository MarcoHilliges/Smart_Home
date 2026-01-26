import { Moon, Sun, Monitor, type LucideProps } from "lucide-vue-next";
import type { FunctionalComponent } from "vue";

type ThemeMode = "light" | "dark" | "system";

export const useThemeMode = () => {
  const modes: {
    value: ThemeMode;
    icon: FunctionalComponent<LucideProps>;
    activeClasses?: String
  }[] = [
    { value: "system", icon: Monitor },
    { value: "light", icon: Sun, activeClasses: 'text-yellow-400' },
    { value: "dark", icon: Moon },
  ];

  const systemIsDarkMode = computed(() => {
    return (
      window.matchMedia &&
      window.matchMedia("(prefers-color-scheme: dark)").matches
    );
  });

  const getThemeModeFromLocalStorage = (): ThemeMode => {
    const themeFromStorage = localStorage.getItem("theme");
    return themeFromStorage === "light" ||
      themeFromStorage === "dark" ||
      themeFromStorage === "system"
      ? themeFromStorage
      : "system";
  };

  const themeModeInLocalStorage = ref(getThemeModeFromLocalStorage());

  const toggleThemeMode = (value: ThemeMode) => {
    setThemeModeInLocalStorage(value);
    themeModeInLocalStorage.value = value;
  };

  const setThemeModeInLocalStorage = (value: ThemeMode) => {
    localStorage.setItem("theme", value);
  };

  const setThemeModeToDocument = (value: ThemeMode) => {
    if (value === "dark") {
      document.documentElement.classList.add("dark");
    } else if (value === "light") {
      document.documentElement.classList.remove("dark");
    } else if (value === "system" && systemIsDarkMode.value) {
      document.documentElement.classList.add("dark");
    } else {
      document.documentElement.classList.remove("dark");
    }
  };

  return { modes, systemIsDarkMode, toggleThemeMode, setThemeModeToDocument, themeModeInLocalStorage };
};
