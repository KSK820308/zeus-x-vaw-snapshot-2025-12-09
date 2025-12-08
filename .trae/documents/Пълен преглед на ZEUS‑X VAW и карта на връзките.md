## Директории и Артефакти
- `src`: основен код (UI, аудио, ядро). Входна точка `src/Main.cpp:33–59`, главен прозорец `src/Main.cpp:21–31`.
- `assets`: икони; копира се към app ресурсите след билд (`CMakeLists.txt:74–77`).
- `build`: артефакти и `compile_commands.json` (активен екземпляр тук); `ZeusXVawApp_artefacts/Debug/ZEUS-X VAW.app` съдържа Resources.
- `Vendor/JUCE`: изходния код на JUCE (модули, примери, инструменти).
- `.trae/documents`: продуктови спецификации и задачи.
- `FILES`: макети/Swift UI идеи + изображения.

## Основна Архитектура
- JUCE GUI App: конфиг в `CMakeLists.txt:20–24`, модули в `CMakeLists.txt:55–65`, плъгин хост дефиниции `CMakeLists.txt:67–72`.
- `MainComponent` управлява страници и initialisation: MIDI/Audio хост, библиотеки и assets (`src/MainComponent.cpp:11–52`). Навигация между Live/Studio/Settings през `TopBar` (`src/MainComponent.cpp:18–38`).
- `TopBar` доставя събития (темпо, MIDI вход, режим) (`src/ui/TopBar.h:6–38`, `49–70`).

## UI → Аудио Връзки
- Mixer страници: `MixerPage` създава канали и панели, избира категории/инструменти и праща Bank/Program към хоста (`src/ui/MixerPage.cpp:24–41`, `54–60`, `167–169`).
- Канали: `MixerStrip` държи пан, фейдър, mute/solo и MIDI кодове; извиква `InstrumentHost` за пан/volume и отваря плъгини/инструменти (`src/ui/MixerStrip.h:45–56`, `150–166`, `259–263`, `294–307`).
- Категории/инструменти: `CategoryPopup` показва 10×2 грид и извиква избор (`src/ui/CategoryPopup.h:104–133`, `196–197`).

## Аудио Хост и Граф
- `InstrumentHost` инкапсулира `AudioProcessorGraph`, MIDI вход/изход и overlay UI за редактори (`src/audio/InstrumentHost.h:71–86`, `110–146`, `147–179`, `184–219`).
- Верига: inst → EQ → Reverb → Delay → Pan/Vol → Output; превключване през `setEffectsEnabled` (`src/audio/InstrumentHost.h:646–669`).
- Bus плъгини 1–8: регистър, състояние и зареждане в графа (`src/audio/InstrumentHost.h:499–550`, `BusRegistry.h:9–26`, `27–36`).

## Плъгини и Сканиране
- Плъгин мениджър: формати AU/VST3, сканиране и кеш (`src/ui/PluginManager.h:8–17`, `19–39`, `70–121`).
- Браузър панел: списък + избор (`src/ui/PluginBrowserPanel.h:7–23`, `42–60`).

## Синтез и Сампли
- Лек engine за тест (`src/audio/AudioEngine.h:6–38`).
- Мапинг Самплер: `MappingSamplerProcessor` с editor за drag&drop зони, ключови/velocity диапазони, ADSR, зум, и `.zxi` export/import (`src/audio/MappingSamplerProcessor.h:24–39`, `src/audio/MappingSamplerProcessor.cpp:109–188`, `733–807`, `809–895`).
- Възпроизвеждане: рендер на синтезатора и MIDI филтър с sustain/volume (`src/audio/MappingSamplerProcessor.cpp:563–612`).

## Ядро: Библиотеки и Активи
- SoundLibrary: директории в Documents, voice манифести `.voice.json`, assign/lookup на `.zxi` (`src/core/SoundLibrary.cpp:58–95`, `165–197`, `287–325`, `326–335`). Непълни custom voice операции (ред/копиране/преместване) – методи са placeholders (`src/core/SoundLibrary.cpp:199–255`).
- Assets: откриване на икони (Documents или Resources), нормализиране на имена (`src/core/Assets.cpp:13–19`, `20–47`, `48–57`).

## Build и Dev Детайли
- `compile_commands.json`: включено (`CMakeLists.txt:3`), но копирането към source става по време на configure (`CMakeLists.txt:5–8`); в root в момента е празен, активният е в `build/compile_commands.json`.
- Post‑build копира икони към app ресурси (`CMakeLists.txt:74–77`).

## Наблюдения и Рискове
- Custom Voice API е частично имплементиран; UI очаква операции (copy/paste/rename/move), които към момента връщат `false`/празно.
- Копирането на `compile_commands.json` вероятно не се изпълнява след generate; по‑надеждно е да се мигрира към post‑build команда.
- Много overlay UI операции разчитат на `MessageManager::callAsync` и SafePointer – добра практика за избягване на crash при затваряне.

## Предложен Доставим Изход
- Архитектурен документ с карта на връзките (UI↔Audio↔Плъгини↔Библиотеки), със секции и препратки `file_path:line`.
- Диаграма на `AudioProcessorGraph` (inst/EQ/Rev/Del/Pan, Sends към Bus 1–8).
- Списък на всички събития/обаждания между компоненти (например `MixerStrip → InstrumentHost`, `MixerPage → SoundLibrary/InstrumentHost`).

## Предложени Следващи Стъпки (след потвърждение)
- Добавяне на post‑build копиране за `compile_commands.json` към source root, аналогично на assets.
- Имплементация на Custom Voice операции в `SoundLibrary.cpp` за да оживее UI‑то на CustomCategoryPopup.
- Малки unit тестове за импорт/експорт `.zxi` и за манифестите на файловете.
- По желание: генериране на `docs/architecture.md` в репото с горните карти и навигационни линкове.

Моля потвърди дали да продължа с генериране на архитектурния документ и предложените подобрения.