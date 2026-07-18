# AI Arranger Style Factory — Идеален работен план

## 1. Продуктова дефиниция

### 1.1 Какво е продуктът
Програма с вграден AI за производство на **аранжорски audio style packs**:
кристални чисти инструментални аудио фрази/лупове, организирани в структурата на style за аранжорен keyboard/software.

Не е генератор на готови песни.  
Не е stem separator.  
Не е MIDI аранжор.

Това е **контролирана factory** за:
- Intro / Ending
- Variations (градация)
- Breaks / Fills / Common break
- Chord/root families за всеки phrase unit
- Mix-aware ensemble от самостоятелни чисти инструменти

### 1.2 Основен продуктов закон
Всеки изходен файл е **самостоятелно генерирана чиста инструментална фраза**, звучаща като записан жив инструмент в студио:
- без bleed
- без phantom инструменти
- без separation artifacts
- годна за повторна употреба в други стилове

### 1.3 Основен работен закон
Генерацията е винаги:
- инструмент по инструмент
- вариация по вариация
- секция по секция
- approve → memory → следващо

Никога dump от стотици файлове наведнъж.

---

## 2. Какво трябва да може системата

### 2.1 Core capabilities
1. Създаване на Style Project със структура на аранжорен стил
2. Brief за конкретна единица (секция + инструмент + performance + tone)
3. Генерация на **phrase family** (акорди/наклонения + root variants)
4. Tone control с продуцентски език (тембър, mic, preamp, EQ, FX)
5. Reference modes:
   - tone-from-reference → нови фрази със същия звук
   - phrase-from-reference → същият performance с нов тембър
   - комбиниран режим
6. Ensemble memory: одобрените части стават закон за следващите
7. Mix-aware complementary generation между инструментите
8. Export на чисти audio assets + метаданни за style pack

### 2.2 Hard locks (неподлежащи на компромис)
- Rhythm lock
- Timbre/FX lock
- Structure lock
- Crystal-clean solo render
- Chord-family consistency across roots/qualities

### 2.3 Human control loop
За всяка единица:
1. Brief
2. Generate family
3. Preview
4. Tweak / regenerate
5. Approve
6. Lock into Style Memory

---

## 3. Архитектура на системата

### 3.1 Слоеве
1. **App / Workflow Engine**  
   UI, проекти, сесии, approve flow, export
2. **Style Brain / Control Layer**  
   structure templates, chord matrix, locks, mix policy, memory
3. **AI Generation Layer**  
   native solo phrase generators + adapters
4. **Reference Understanding Layer**  
   audio analysis за tone/performance/arrangement cues
5. **Training / Adaptation Layer**  
   datasets, fine-tunes, preference learning

### 3.2 Два „мозъка“ при генерация
- **Ensemble Brain** — мисли аранжимент и микс отношения
- **Solo Render Brain** — рендерира всеки инструмент като абсолютно чиста фраза

Мислят заедно. Записват поотделно.

### 3.3 Generator като сменяем engine
Програмата не се връзва към един модел завинаги.  
Има адаптерен интерфейс:
- вход: unit brief + references + constraints + memory
- изход: clean phrase family + metadata

Това позволява етапно качване на AI качеството без пренаписване на продукта.

---

## 4. Етапен план за създаване (ideal roadmap)

## Фаза 0 — Заключване на визията и scope
**Цел:** да няма размиване на продукта.

### Задачи
- Замразяване на продуктовата дефиниция
- Дефиниране на Style Template schema
- Дефиниране на Unit Brief schema
- Дефиниране на Approve/Memory правила
- Дефиниране на MVP boundary (какво НЕ влиза в първа версия)
- Избор на target export формат(и) за style packs

### Изход
- Product Spec v1
- Data Model v1
- Acceptance criteria за „crystal clean“ и „family consistency“

---

## Фаза 1 — Skeleton на програмата (без сериозен AI)
**Цел:** работещ workflow, преди силен генератор.

### Задачи
1. Създаване на приложението (project-based)
2. Style Project CRUD
3. Структура на стил:
   - Intro
   - Variations
   - Breaks/Fills
   - Ending
   - Common break
4. Избор на текуща единица (section/instrument)
5. Brief форма (performance + tone + constraints)
6. Mock generator (placeholder audio / simple synthesis), за да се върти UI/flow
7. Preview player
8. Approve / reject / regenerate
9. Style Memory store
10. Export на файлове + JSON/YAML метаданни

### Изход
Кликваем продукт с пълен контролен цикъл, още преди силен AI.

### Acceptance
Може да се премине през пълен style workflow ръчно/с mock данни без объркване.

---

## Фаза 2 — Vertical Slice (първа реална стойност)
**Цел:** един пълен вертикал end-to-end.

### Scope на vertical slice
- 1 style project
- 1 section type (напр. Variation)
- 1 instrument role
- chord/root family generation
- preview + approve
- memory lock
- export

### Задачи
1. Generator Adapter v1 към реален audio backend
2. Constraint pipeline:
   - BPM / bars
   - rhythm lock intent
   - tone sheet
3. Family expander:
   - roots
   - major/minor/sevenths (според матрицата)
4. Consistency checks (дължина, loudness range, loop seam basics)
5. Human review UI за family listening

### Изход
Реално ползваема генерация на едно phrase family.

### Acceptance
От един brief се получава консистентно семейство чисти фрази, годни за слушане и approve.

---

## Фаза 3 — Multi-instrument ensemble memory
**Цел:** вторият инструмент да се съобразява с одобрения първи.

### Задачи
1. Memory packaging на одобрени units
2. Ensemble brief builder
3. Complementary mix policy engine:
   - low-end ownership
   - spectral roles
   - transient ownership
4. Generation conditioned on approved instruments
5. Variation role controls (foundation / movement / counter-melody / slap accents и т.н. като общи behavior switches)
6. Section-to-section inheritance rules

### Изход
Инкрементално градене на банда: инструмент върху инструмент.

### Acceptance
Нов инструмент явно следва groove/mix логиката на вече одобрените части.

---

## Фаза 4 — Reference-driven workflows
**Цел:** работа с аудио референси като първокласен вход.

### Режим A — Tone from reference
- вход: mixed/ref audio + target instrument focus
- изход: tone fingerprint
- ползване: нови фрази със заключен звук

### Режим B — Phrase from reference
- вход: audio с желано свирене
- изход: performance fingerprint (ритъм/артикулация/акценти)
- ползване: същият performance с нов тембър

### Режим C — Combined
- tone от референс X
- performance от референс Y
- нови chord-family разгъвки

### Задачи
1. Reference ingest
2. Instrument-focused analysis
3. Tone fingerprint schema
4. Performance fingerprint schema
5. Apply fingerprints into unit brief
6. Explicit UX: „lock sound“ / „lock playing“ / „swap either“

### Важна политика
Не се обещава 100% хирургическо изваждане от микс.  
Обещава се **usable reference profile** за контролирана генерация.

### Изход
Референсът става работен инструмент, не пожелание.

---

## Фаза 5 — Style completeness tools
**Цел:** от отделни units → пълним style pack.

### Задачи
1. Template wizard за пълен style chassis
2. Variation grading planner (Var1…VarN escalation)
3. Break/Fill planner спрямо variation context
4. Intro/Ending planners
5. Coverage map: кое липсва още в пакета
6. Batch family completion само след approve на DNA-то
7. Pack validation:
   - missing roots/qualities
   - inconsistent lengths
   - naming/metadata integrity
   - loudness/phase sanity basics

### Изход
Системата води производството на цял стил, не само единични лупове.

---

## Фаза 6 — Data стратегия и обучение
**Цел:** AI-то да става по-добро по правилния начин.

### 6.1 Три кошници данни
**A. Clean targets (златен стандарт)**
- истински чисти solo phrases / мултитракове
- учат crystal-clean generation

**B. Arrangement references**
- mixed tracks и/или стемове (дори с артефакти)
- учат стил, роли, градация, section logic
- НЕ се ползват като pure-tone truth

**C. Human preference loop**
- approve / reject / tweak история
- учи вкуса на продуцента

### 6.2 Какво се учи от музика
- groove/style DNA
- arrangement escalation
- instrument roles
- section behavior
- mix relationships
- performance language

### 6.3 Роля на MIDI
MIDI/patterns са **помощни controls**, не краен продукт:
- accents
- rhythmic skeleton
- role templates

Крайният изход е audio.

### 6.4 Роля на стемове с артефакти
Позволени като weak supervision за аранжимент.  
Забранени като единствен teacher за clean solo sound.

### 6.5 Обучителен ред
1. General clean-instrument competence
2. Role/accompaniment behavior
3. Style specialization
4. Preference alignment към продуцентския вкус
5. По-късно: по-сериозен custom model, ако данните/бюджетът позволяват

### Изход
Повторяем training/adaptation pipeline, вързан към реалната употреба на програмата.

---

## Фаза 7 — Quality systems (критично за продукта)
**Цел:** „яко“ да стане измеримо и повторяемо.

### Автоматични проверки
- solo purity heuristics (чужди инструменти / bleed risk)
- loop seam quality
- timing/BPM adherence
- family consistency (rhythm/timbre across roots)
- loudness consistency
- spectrum role collisions (bass vs kick и т.н.)

### Човешки QA протокол
- A/B слушане внутри family
- cross-variation continuity
- ensemble stack test (всички заедно)
- reuse test (фраза в друг контекст/стил)

### Taste profile
Системата трупа какво минава approve при конкретния продуцент и го ползва като prior.

---

## Фаза 8 — Production hardening
**Цел:** програмата да е годна за редовна работа.

### Задачи
1. Job queue / progress / resumable sessions
2. Versioning на units и цели стилове
3. Reproducibility (seeds, briefs, model versions)
4. Preset libraries (tone sheets, role behaviors)
5. Project backup/export/import
6. Performance optimization (local/cloud hybrid ако трябва)
7. Legal/data provenance tracking за training/reference assets

### Изход
Инструмент за ежедневно производство, не само demo.

---

## Фаза 9 — Advanced intelligence
**Цел:** системата започва да „мисли“ като style producer.

### Възможности
- auto-proposed style recipe от няколко референса
- automatic complementary suggestions (bass/drums options)
- smart variation escalation proposals
- detect when new take чупи locked DNA
- cross-style reuse recommendations за чисти фрази
- assisted packing към конкретни arranger targets

### Изход
AI не само генерира звук — води производствения процес.

---

## 5. Препоръчителен ред на имплементация (практически)

### Sprint логика
1. Spec + data model
2. App skeleton + mock flow
3. Real generator adapter + one family
4. Memory + second instrument
5. Reference tone/performance locks
6. Full style coverage tools
7. Data/training loop
8. QA hardening
9. Advanced producer assist

### Правило за приоритет
Винаги първо:
1. контрол
2. чистота
3. консистентност
4. convenience automation

Никога обратното.

---

## 6. MVP дефиниция (първа истинска версия)

### Влиза в MVP
- Style Project
- Unit-by-unit workflow
- One real instrument family generation
- Roots + basic chord qualities
- Tone sheet basics
- Approve/Memory
- Export clean audio + metadata
- Basic consistency checks

### Не влиза в MVP
- Перфектен custom foundation model от нулата
- Пълен автоматичен цял style в един клик
- 100% perfect extraction от mixed commercial tracks
- Всички инструменти/всички стилове на максимално качество
- Пълен auto-mix mastering suite

### MVP success criteria
Продуцентът може да създаде и одобри реално ползваемо phrase family за style work по-контролирано и по-чисто, отколкото с generic music generators.

---

## 7. Роли в процеса

### Продуцент / визионер (ти)
- дефинира „яко“
- дава референси и критерии
- approve/reject
- решава style targets и приоритети

### Engineering / AI systems (агентът / екипът)
- архитектура и програма
- workflow engine
- adapters и pipelines
- training/adaptation infrastructure
- QA tooling
- итерации по quality

### Разделение на отговорността
Програмата и дисциплината се строят първи.  
AI качеството се качва етапно върху реалния workflow.

---

## 8. Рискове и как се управляват

| Риск | Защо е опасен | Управление |
|---|---|---|
| Artifact learning от стемове | Чупи crystal-clean стандарт | Стемове само за arrangement weak supervision |
| One-click dump UX | Губй се контрол и вкус | Unit-by-unit approve law |
| Family inconsistency across roots | Стилът става неизползваем | Hard locks + family QA |
| Mix collisions | Бандата не сяда заедно | Ensemble memory + mix policy |
| Training too early | Гори бюджет без продукт | App/workflow first |
| Over-promising reference extraction | Разочарование | Usable fingerprint, not surgical stem rip |
| Scope explosion | Проектът не стига до ползваема версия | Strict MVP vertical slice |

---

## 9. Definition of Done по нива

### DoD — Workflow Done
Може да се води цял проект стъпка по стъпка с memory и export.

### DoD — Generation Done
Една единица дава clean, consistent chord/root family по brief.

### DoD — Ensemble Done
Новите инструменти се съобразяват с одобрените и микс логиката.

### DoD — Style Pack Done
Пълен pack минава coverage + consistency + ensemble stack test.

### DoD — Learning Done
Системата се подобрява от clean data + arrangement refs + human preference без да се отравя от artifacts.

---

## 10. Оперативен чеклист за старт

### Веднага
- [ ] Заключване на Product Spec v1
- [ ] Style Template schema
- [ ] Unit Brief schema
- [ ] Memory/Approve rules
- [ ] MVP boundary
- [ ] Избор на tech stack за app + audio preview + project storage

### Първа инженерна цел
- [ ] Работещ skeleton с mock generator
- [ ] Пълен approve loop
- [ ] Export pipeline

### Първа AI цел
- [ ] Adapter към реален generator
- [ ] Един vertical slice: unit → family → approve → memory

### Първа data цел
- [ ] Кошници A/B/C
- [ ] Минимален clean corpus
- [ ] Provenance/tracking за всеки asset

---

## 11. Финална рамка

Този проект се създава като:

> **AI-powered Arranger Style Factory**  
> с human-gated, unit-based производство на кристално чисти audio phrases,  
> заключени в style template, chord families, tone/performance locks и mix-aware ensemble memory.

Успешният път не е „да обучим всичко наведнъж“.  
Успешният път е:

1. построи машината  
2. вкарай контрола  
3. вържи генерацията  
4. учи от правилните данни  
5. качвай качеството върху реални style проекти

---

## 12. Следващ конкретен ход след този план
1. Product Spec v1 (къс, строг, без размиване)
2. Data Model v1 (Style Project / Unit / Family / Memory)
3. MVP screen flow
4. Tech stack decision
5. Skeleton implementation kickoff
