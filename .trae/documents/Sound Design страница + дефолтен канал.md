## Малка промяна: Дефолтна селекция

* В `MixerPage` след създаване на лентите задавам `activeChannel = "RIGHT 1"` и извиквам селекция върху съответния `MixerStrip`.

* Ефект: при старт винаги е селектиран RIGHT 1.

## Новa страница: Sound Design

* Добавям `SoundDesignPage` (JUCE Component) с панел и контроли.

* Разположение: в `MainComponent` добавям трети режим (като Studio/Settings), който показва Sound Design.

## Контроли и поведение

* Load Sound:

  * Отваря избор от ZEUS-X библиотеката (категории + инструменти) като в Assign логиката (ползвам наличния `CategoryPopup`/списъци от `SoundLibrary`).

  * Зарежда MappingSampler инструмент: `InstrumentHost::setInstrumentProcessor(std::make_unique<MappingSamplerProcessor>())` и `importInstrument(...)`.

* Save Sound:

  * Отваря избор (категория + позиция). Ако позицията е заета → предупреждение/replace.

  * Записва `.zxi` с всички корекции от Sound Design: ефекти, октава, ADSR.

* Ефекти и параметри (всички с кноб + редактируема числова стойност):

  * EQ (Low/Mid/High): 3‑лентов екю върху аудио (LowShelf, Peak, HighShelf). Параметри: gain (дБ), freq, Q.

  * Reverb: JUCE `juce::Reverb` с параметри (room size, damping, width, wet/dry). Типове като presets (Room/Hall/Plate) с падащо меню.

  * Delay: собствен `DelayProcessor` (стерео, tempo‑free). Параметри: time (ms), feedback, mix. Типове presets (Stereo/Ping‑Pong/Tape) с падащо меню.

  * Други JUCE dsp ефекти (по избор в падащо меню): Chorus, Phaser, WaveShaper. Добавям селектор за активиране.

* ADSR:

  * Attack/Release кнобове + редактируеми стойности. За MappingSampler: директно модифицира `adsrParams` в процесора.

* Octave:

  * Кноб + редактируемо поле (‑5..+5). Пер‑канално: ползва `InstrumentHost::setMidiOctave(channel, o)` (RIGHT 1 канал).

## Аудио архитектура

* В `InstrumentHost` между `instNode` и `panNode` добавям ефектна верига (AudioProcessorGraph nodes): EQProcessor → ReverbProcessor → DelayProcessor → \[Optional dsp ефекти].

* Всички параметри се променят плавно (`SmoothedValue`) — без пращене.

* MIDI остава насочен само към инструмента; ефектите са аудио‑базирани.

## Формат `.zxi` разширение

* В `MappingSamplerProcessor::exportInstrument(...)` добавям секция `fx`:

  * `eq`: { low:{gain,freq,q}, mid:{...}, high:{...} }

  * `reverb`: { preset, roomSize, damping, width, wet, dry }

  * `delay`: { preset, timeMs, feedback, mix }

  * `octave`: int, `transpose`: int

  * `adsr`: вече се пази (оставям).

* В `importInstrument(...)` чета `fx` и настройвам ефектната верига и ADSR.

## UI детайли

* Кноб + поле за стойност до всеки параметър; поле е редактираемо (Enter потвърждава).

* Падащи менюта за типове (presets) около съответния кноб.

* Подредба: две колони — отляво Load/Save + ADSR/Octave; отдясно блок ефекти (EQ горе, Reverb/Delay среда, Other долу).

## Тестване

* Зареждане инструмент → промяна EQ/Reverb/Delay/ADSR/Octave → Save → Re‑Load → проверка, че всички параметри са възстановени и звучат идентично.

* Проверка за плавност: въртене на кнобове по време на аудио — без пращене.

## Поетапност

* Фаза 1: работи с MappingSampler и ефектна верига; разширенията се пазят в `.zxi`.

* Фаза 2: добавям sidecar `.zxfx` за външни VST инструменти (ако поискаш подкрепа за тях).

Готов съм да започна реализацията по този план и да доставя работеща Sound Design страница с пълен запис на параметрите. 

ТОВА ВСИЧКОТО ТРЯБВА ДА СЕСЛУЧВА САМО И ЕДИНСТВЕНО В СТРАНИЦАТА САУНД ДИЗАЙН!!!!!

