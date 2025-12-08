# ZEUS-X VAW – Архитектура и връзки

## Директории
- `src`: UI, аудио и ядро. Вход: `src/Main.cpp:33–59`. Прозорец: `src/Main.cpp:21–31`.
- `assets`: икони; копиране към Resources (`CMakeLists.txt:74–77`).
- `build`: артефакти и активен `compile_commands.json`.
- `Vendor/JUCE`: JUCE модули.
- `.trae/documents`: продуктови спецификации.

## Основни компоненти
- `MainComponent`: UI контейнери, инициализация на хоста (`src/MainComponent.cpp:11–52`).
- `TopBar`: темпо, избор на MIDI устройство, режим (`src/ui/TopBar.h:6–38`, `49–70`).
- `MixerPage`: канали, категории, режим MIDI/Audio (`src/ui/MixerPage.cpp:24–60`, `167–169`).
- `MixerStrip`: пан/фейдър/mute/solo, кодове MSB/LSB/PC, VST/VST Bus (`src/ui/MixerStrip.h:45–56`, `150–166`, `259–263`).
- `CategoryPopup`: избор на инструменти (`src/ui/CategoryPopup.h:104–133`, `196–197`).

## Аудио граф
- Хост: `InstrumentHost` – `AudioProcessorGraph`, MIDI, overlay редактори (`src/audio/InstrumentHost.h:110–146`, `147–179`, `646–669`).
- Верига: inst → EQ → Reverb → Delay → Pan/Vol → Output.
- Bus-и 1–8: регистър и състояние (`src/audio/InstrumentHost.h:499–550`, `src/audio/BusRegistry.h:9–26`).

### Диаграма (логическа)
- Вход MIDI → `MidiTransposeProcessor` → `instNode`
- `instNode` → `EQProcessor` → `ReverbProcessor` → `DelayProcessor` → `PanVolumeProcessor` → Output
- Sends: `instNode` → `GainProcessor` ×N → `BusNode[i]` → Output

```
MIDI In ──▶ MidiTranspose ──▶ instNode ──▶ EQ ──▶ Rev ──▶ Del ──▶ PanVol ──▶ Audio Out
                          └──────────────▶ Gain(1) ─▶ Bus1 ─▶ Audio Out
                          └──────────────▶ Gain(2) ─▶ Bus2 ─▶ Audio Out
                          └──────────────▶ ...
```

## Самплер и формати
- `MappingSamplerProcessor`: зони, редактор, `.zxi` export/import (`src/audio/MappingSamplerProcessor.cpp:109–188`, `733–807`, `809–895`).
- Рендер и MIDI филтър (`src/audio/MappingSamplerProcessor.cpp:563–612`).

## Библиотека и активи
- `SoundLibrary`: директории, манифести `.voice.json`, assign/lookup (`src/core/SoundLibrary.cpp:58–95`, `165–197`, `287–325`, `326–335`).
- `Assets`: икони от Documents или Resources (`src/core/Assets.cpp:13–19`, `20–47`).

## UI↔Аудио събития
- `MixerStrip`:
  - Пан/фейдър → `InstrumentHost::setChannelPan/Volume` (`src/ui/MixerStrip.h:309–314`).
  - Bank/Program → `InstrumentHost::sendBankSelect/ProgramChange` (`src/ui/MixerStrip.h:261–263`).
  - BUS Sends → `InstrumentHost::setSends` (`src/ui/MixerStrip.h:295–305`).
- `MixerPage`:
  - Избор на категория → задава MSB/LSB/PC и зарежда `.zxi` (`src/ui/MixerPage.cpp:24–41`, `29–31`).
- `StudioPage` Sound Design:
  - EQ/Rev/Del/Octave → `InstrumentHost::setEQ/Reverb/Delay/setMidiOctave` и синхронизация с самплера (`src/ui/StudioPage.h:116–139`).

- `TopBar`:
  - Смяна на MIDI вход → `InstrumentHost::setMidiInput` (`src/MainComponent.cpp:16–18`).
  - Смяна на режим → показва Live/Studio/Settings (`src/MainComponent.cpp:18–38`).

## Build
- `CMakeLists.txt`: JUCE модули (`55–65`), AU/VST3 хост (`67–72`), пост-копиране на assets (`74–77`).
- `compile_commands.json`: активен в `build/`; предложено доп. post-build копиране към source root.

## Бележки
- Custom Voice API в `SoundLibrary.cpp` ще бъде имплементиран за copy/paste/rename/move и slot операции.
