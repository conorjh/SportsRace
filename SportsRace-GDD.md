# SportsRace — Reverse-Engineered Game Design Document

> **Document status:** Prototype reconstruction from the current C++ implementation. This document describes implemented behavior first, then calls out inferred intent and incomplete systems. It is not a statement of a finished or fully playable product.

## 1. High concept

**SportsRace** is a mouse-driven, arcade-style sprint-racing game. The player enters short foot races, watches procedurally generated racers compete, and—through the career mode—manages one featured racer, views that racer's attributes, enters races for cash prizes, and inspects rankings.

The current build is primarily a race simulation and UI prototype. Racers move automatically; there is no observed in-race player input, steering, timing input, upgrade system, save system, or implemented league standings system.

## 2. Player fantasy and design pillars

### Player fantasy

- Discover a randomly generated runner with a distinctive name and stats.
- Watch a field of runners accelerate, sprint, and finish in a ranked order.
- Make a career out of entering races and earning prize money.
- Compare the player's runner with other generated competitors.

### Design pillars inferred from the code

1. **Spectator-style racing:** race outcomes are simulated automatically rather than controlled directly.
2. **Readable race phases:** a pre-race pause, starter's-orders pause, active race, finishing phase, and completed state are represented explicitly.
3. **Randomized competitors:** names, identities, starting speeds, skills, and race performance are generated from the random number generator.
4. **Light career management:** the player has a racer profile and a cash balance; race entry fees and top-three prizes are modeled.

## 3. Target experience and platform

- **Platform:** Windows desktop.
- **Input:** mouse for buttons; Escape for back/cancel/exit from most screens; window close exits the application.
- **Default game window:** 1024 × 768 pixels.
- **Rendering/audio stack:** SDL3, SDL3_image, SDL3_ttf, and SDL3_mixer.
- **Build target:** Visual Studio C++ project using C++20 and the v143 toolset, with Win32 and x64 configurations.
- **Presentation:** 2D sprite and background rendering with text overlays, hover/click sound effects, race music, and career-hub music.

## 4. Core game loop

### Intended loop

1. Launch the game.
2. Choose a free race or career mode from the main menu.
3. In career mode, create or inspect the player's runner.
4. From the career hub, choose a race, training, racer details, or rankings.
5. Watch the race resolve automatically.
6. Review the leaders, times, and possible winnings.
7. Return to the career hub and repeat while managing cash.

### Current executable loop

1. The application initializes SDL, audio, the renderer, and the screen state machine.
2. The main menu is pushed as the first screen.
3. **Race** creates a six-racer demonstration race and opens the race screen.
4. **Career** opens the career hub. The application data constructor already creates a profile, so the intended first-time character-creation branch is not normally reached in the current build.
5. Career mode can open the racer profile, ranking screen, or a six-racer career race.
6. The race simulation advances in fixed 33 ms steps while the race screen updates.
7. Escape leaves the race. In a career race, leaving after the player's racer has finished settles the entry fee/prize and copies the result into application data.
8. Exit or window close halts the application.

## 5. Main menu

The main menu contains three mouse buttons:

| Button | Position/size in current UI | Behavior |
|---|---:|---|
| Race | `(100, 100)`, `210 × 80` | Starts a six-racer demonstration race. |
| Career | `(100, 200)`, `290 × 80` | Opens the career hub; intended first-run character creation is present as a branch but is bypassed by the current initialization. |
| Exit | `(100, 400)`, `180 × 80` | Sets the application to halted. |

The menu also contains a background image and a placeholder/commented-out league standings area. Six generated racers are prepared when the screen is constructed.

## 6. Career mode

### Career profile

A career profile contains:

- **MainFella:** the player's `Racer`.
- **Cash:** initialized to 5,000.

The career data also owns a `RacerDB` for generated racers. `CareerOrchestrator::Init()` can generate 1,000 racers, but the current screen flow does not call `Init()`. The orchestrator update method is empty.

### Career hub

The career hub presents four icon buttons:

| Option | Current behavior |
|---|---|
| Racer | Opens the player's racer profile in view-only mode. |
| Training | Opens a race screen using a new race, but does not currently add the selected racer to that race; this is incomplete/likely nonfunctional. |
| Race | Creates a career race containing the player plus five generated opponents. |
| Ranking | Opens a ranking screen, but no standings data is rendered. |

The hub plays the `Catering` soundtrack on entry and fades music out on exit. Escape returns to the previous screen.

### Career race economy

Career races are assigned random financials when launched:

- First-place prize: `500 × (random value from 0 through 9)`; current possible values are 0–4,500.
- Second-place prize: half of first place, using integer division.
- Third-place prize: one quarter of first place, using integer division.
- Entrance fee: `50 × (random value from 0 through 3)`; current possible values are 0, 50, 100, or 150.
- Starting cash: 5,000.

When the player leaves a career race after the player's racer has finished:

1. The race is simulated to completion if needed.
2. The entrance fee is subtracted.
3. The first-, second-, or third-place prize is added if the player occupies that position.
4. The complete `RaceResult` is copied to the application result buffer.

There is no observed payout for fourth place or lower, no bankruptcy rule, no experience gain, and no persistent save/load behavior.

### Proposed career expansion: world map and race schedule

**Design status:** Proposed. The current build has a career hub, but no world map, event calendar, schedule progression, or minigames. The following is a design direction, not implemented behavior.

The career should give the player a reason to choose what to do between races while keeping the existing spectator-race format. Keep the career hub as the player's home screen and make a new **World Map** the main way to discover and enter career events.

#### World map

- Show venues as selectable nodes grouped into regions. A node identifies its venue and the events available there.
- Give each event a clear state: **Upcoming**, **Entry Open**, **Entered**, **Completed**, or **Locked**.
- Start with a small local circuit. Unlock further regions or higher-level meets through race results or season goals; exact regions and unlock requirements are open design decisions.
- Selecting an available event opens its details before the player commits, including date/round, distance, entry fee, prize breakdown, and any eligibility rules.
- Keep the map readable at the default 1024 × 768 window. A stylized route map with clickable venue markers is preferred over a large navigable simulation map.

#### Scheduled races

Use a simple, discrete calendar rather than real-time deadlines. The calendar presents upcoming events in the player's region; the player chooses an event, enters it before its scheduled round, and advances the calendar after competing or explicitly skipping ahead. Completing an event shows the result, settles the fee and prize, updates career progress, and reveals the next schedule state.

Recommended event flow:

1. Review available events on the map and compare distance, entry cost, prize, and opponents.
2. Enter one event and confirm the fee and race details.
3. Watch the race resolve using the existing automatic race simulation.
4. Review placing, time, payout, and any season or unlock progress.
5. Return to the hub or map to choose the next scheduled activity.

For a first playable version, reuse the current entry-fee and top-three prize model, but assign financials to the event rather than rolling them each time the player opens the race. Event fields, schedule length, season scoring, and the handling of missed events still need balancing decisions.

### Proposed career minigames

Minigames are optional diversions between scheduled races. They should be short, mouse-friendly, and reward in-game career currency or progression without becoming mandatory gates for entering races. Their rewards, entry costs, and effect on racer attributes are not yet defined.

#### Trackside slot machine

- Present a three-reel, sports-themed slot machine from the career hub or map. Example symbols: running shoes, a starter pistol, medals, and a trophy.
- The player chooses a wager in in-game cash, spins once, and sees the payout or loss before returning to the career screen.
- Publish the wager limits and payout table in the UI. Keep the activity optional, use no real-money purchases, and do not make a spin necessary for career progression.
- Treat the wager range, reel odds, payout table, and any per-round spin limit as tuning decisions; do not use the current random race prizes as the slot-machine paytable by default.

#### Other minigame candidates

- **Starter reaction:** click when the starter signal appears; award a small cash or season-point bonus for accurate timing.
- **Training drill:** complete a short sequence or target-tapping challenge; use it for a modest training/XP reward if racer progression is adopted.
- **Sponsor challenge:** complete a simple event-themed objective for a fixed reward, giving players a lower-variance alternative to the slot machine.

Keep rewards capped and separate minigame outcomes from race results. Decide whether minigames affect only cash/season points or can also improve racer skills before implementing them.

## 7. Racer design

### Identity

Racers have a generated GUID and a generated full name made from two random name lists. The name pool is intentionally comedic and includes names such as “Larry Routledge,” “Fat Steve,” and “Pong Whiffy.” A caller can also provide an explicit name.

### Attributes

Each racer receives randomized skills at construction:

| Attribute | Implemented initialization | Gameplay role inferred from implementation |
|---|---:|---|
| Base Speed | 1–15 | Baseline velocity used when a speed roll occurs. |
| Standard Velocity | 1–9 | Random amount added during normal speed rolls. |
| Sprint | 0–4 | Additional random amount during the early race phase. |
| Luck | 0–1 | Intended random bonus, but the current `rand() % Luck` expression produces no positive bonus when Luck is 1. |
| Sprint Cutoff | 0 to half of Standard Cutoff | Distance threshold for leaving the early/sprint phase. |
| Standard Cutoff | 1–50,000 | Distance threshold for leaving the normal phase. |
| XP | 0 | Declared but not currently modified. |

The racer profile displays name, Base Speed, Standard Velocity, Sprint Boost, Luck, and cash. It does not display the cutoff values or XP.

### Movement model

- Racers begin at position 0.
- Each simulation tick calls `Racer::Tick(33)`.
- Position increases by the current velocity once per tick.
- Speed is rerolled probabilistically based on distance:
  - Before both cutoffs: a roll occurs when `rand() % 8 == 1`; sprint and luck can be included.
  - After the sprint cutoff but before the standard cutoff: a roll occurs when `rand() % 10 == 1`; the sprint bonus is no longer included.
  - After both cutoffs: a roll occurs when `rand() % 20 == 1`; the base speed is used, with the current luck expression.
- The `Ms` argument advances internal time and animation timing, but displacement is not scaled by elapsed milliseconds.
- The running animation cycles through four frames. During starter's orders, racers use a waiting sprite.

## 8. Race rules

### Track

A `Track` has a configurable length. The default track is `Standard100m`, represented internally by 5,000 distance units. Other declared lengths are:

- Standard 100 m: 5,000 units
- Standard 200 m: 10,000 units
- Standard 250 m: 12,500 units
- Standard 500 m: 25,000 units
- Standard 1,000 m: 50,000 units

The current UI constructs the default track and does not expose track selection.

### Field size

- Main-menu demonstration race: six generated racers.
- Career race: one player racer plus five generated opponents.
- The race model itself supports an arbitrary vector of racer pointers.

### Race phases

The state machine exposes these phases:

1. **PreRace:** lasts just over one second; racers are positioned at the start.
2. **StartersOrders:** lasts just over one second; racers remain at the start and display the waiting sprite.
3. **Racing:** racers move until at least one crosses the finish line. The starter pistol and race soundtrack play on transition into this phase.
4. **Finishing:** remaining racers continue moving until all have crossed the finish line. Racers are clamped to no more than 1,000 units beyond the finish line.
5. **Finished:** the simulation is complete and no further state changes occur.

### Ranking and results

- A racer finishes when position exceeds the track length.
- Finish order is recorded using the current race tick as the result time in milliseconds.
- Position/rank is determined by descending distance while a race is in progress.
- Results record GUID, racer pointer, position, and elapsed milliseconds.
- The result panel displays up to the first three finishers, their names, times in seconds, and the corresponding prize amount.

## 9. Race screen and HUD

The race screen is a side-scrolling 2D presentation:

- The camera follows the player's racer when the player GUID is present; otherwise it follows the current leader.
- The background uses repeating track, mountain, cloud, stadium, and starting-block imagery with different horizontal parallax rates.
- Racers are displayed on staggered lanes with their names overlaid.
- A bottom progress bar shows each racer's distance; the player's marker is drawn larger when a player GUID is available.
- A debug overlay shows screen type, race state, finished count, leader, entrance fee, and race timing data.
- A results panel appears when result entries exist and shows the top three.

There is no observed pause button, speed control, betting action, lane control, or interactive race decision.

## 10. Racer creation and profile UX

The intended creation screen supports:

- **Regen:** generate a new name and a new randomized skill set.
- **Apply:** accept the generated racer and return to the prior screen.

The view-only profile supports:

- Viewing the generated racer and displayed attributes.
- Exit via button or Escape.

The profile renderer shows the runner sprite, name, four visible stats, and cash. The current application initialization creates a profile before the main menu, so the creation flow should be considered intended design rather than reliably reachable current behavior.

## 11. Controls and feedback

| Input | Result |
|---|---|
| Left mouse button over a button/icon | Activates the control. |
| Mouse hover | Changes button text color for text buttons and slightly offsets career icons; plays a hover sound on entering hover. |
| Escape | Back/exit on career, racer, ranking, and race screens; behavior is screen-dependent. |
| Window close | Halts the application. |

Audio assets currently referenced:

- Hover sound
- Click sound
- Starter pistol
- `catering.mp3` career soundtrack
- `race.mp3` race soundtrack

## 12. Game states and screen map

```text
Application launch
		|
		v
	Main Menu
	 /  |   \
   Race Career Exit
	|     |
	v     v
 Race   Career Hub
		  |  |  |  \
	   Racer Race Training Ranking
		  |    |
		  v    v
	   Profile Race Screen
```

The screen stack supports push, pop, and replace-top operations. Screens call `Entry()`/`Exit()` when entering or leaving; current implementation mainly uses these hooks for music control.

The diagram describes the current screen types. In the current build, the Career Hub's **Training** button opens a race screen that does not add the player's racer, and **Ranking** is a visual shell with no standings. The proposed World Map and minigames are future additions, not current screen types.

## 13. Technical implementation summary

- **Language:** C++20.
- **Application framework:** SDL3 main loop and event polling.
- **Rendering:** SDL renderer, surfaces, textures, SDL_ttf text.
- **Audio:** SDL_mixer chunks and music.
- **Randomness:** process-wide C `rand()` seeded from current time at application construction; tests seed it explicitly.
- **Timing:** render loop runs continuously; race simulation is advanced in fixed 33 ms increments.
- **Data ownership:** much of the prototype uses raw pointers and screen-owned state transitions.
- **Assets:** loaded from a `media/` directory copied beside the executable during the Visual Studio post-build step.

### Application and screen system

- `Application` initializes SDL and audio, pushes the main menu, and updates input and the active screen.
- `AppData` holds the screen stack, render context, race-result buffer, and career/profile pointers. Its constructor currently creates a career and profile immediately, which bypasses the main menu's intended first-career creation branch.
- `AppIO` polls SDL events and exposes Escape, mouse position/buttons, and the audio player to screens.
- `AppScreen` is the shared screen base. `AppScreenStateMachine` updates the top screen and supports push, pop, and swap-top transitions, calling the screen `Entry()` and `Exit()` hooks during transitions.

### Implemented screens

| Screen | Current responsibility | Status/notes |
|---|---|---|
| Main Menu | Start a six-racer demonstration race, enter career, or exit. | Implemented. |
| Career Hub | Open racer profile, training, race, or ranking. | Implemented; it does not yet show career cash or scheduled events. |
| Racer Screen | Regenerate/apply a career racer or view the current profile. | Both modes are represented; first-run creation is bypassed by current initialization. |
| Race Screen | Advance race phases and show the race presentation/results. | Implemented; career settlement occurs when leaving after the player has finished. |
| Ranking Screen | Display a ranking view and provide an exit. | Screen shell exists; no standings are rendered. |

The proposed **World Map** and minigames will require new screen or panel flows. They are not present in the current screen state enum or renderer dispatch.

### Renderer

`BaseRenderer` provides common image/text drawing, presentation, and frame-time helpers. The `AssetStore` registers the prototype's fonts and image assets. `AppRenderer` checks the active screen type and delegates to the matching menu, career hub, racer, ranking, or race renderer; each screen renderer draws that screen's UI. New map or minigame views will need corresponding rendering and asset work.

### Race class

`Race` owns the track reference, racer list, financials, current tick, and `RaceResult`. It advances racer movement, records finish order and elapsed simulation time, and can run the simulation to completion. `RaceStateMachine` wraps the race with the pre-race, starter's-orders, racing, finishing, and finished presentation phases. The race model supports a vector of racers; the current main-menu and career flows each populate six.

## 14. Verified behavior from tests

The test project contains eight executable tests covering:

- Application-path command-line parsing.
- Parameterized command-line parsing for `log` and `cfg`.
- Optional `showfps` parsing.
- Racer reset restoring tick, animation, and position state.
- Racer database creation, lookup, and containment.
- Ranking by descending racer distance.
- A racer crossing the track length producing a finishing result.
- Second- and third-place prize calculations from first-place prize.

These tests verify model-level behavior only. They do not verify rendering, screen navigation, audio, career settlement, or end-to-end playability.

## 15. Known gaps and likely next design decisions

The following items are visible from the current implementation and should be resolved before treating this as a complete production GDD:

1. Define the intended first-run career flow and fix profile initialization so racer creation is reachable.
2. Decide whether the game is a spectator simulation, an interactive timing game, or a management game; current code only supports spectator simulation.
3. Implement or remove training mode; it currently creates a race without adding the selected racer.
4. Define world-map regions, venue unlocks, event eligibility, and how the player advances the scheduled-race calendar.
5. Define the slot-machine wager limits, odds, payouts, and how minigame rewards interact with career progression.
6. Define ranking/league rules, persistence, season structure, and standings UI.
7. Define XP progression and whether attributes can be trained or upgraded.
8. Define the result-screen interaction and explicit continue/back controls instead of relying on Escape.
9. Define what happens when the player finishes outside the top three or cannot afford an entry fee.
10. Make time-based movement deterministic and elapsed-time aware if race results need to be reproducible.
11. Add save/load behavior if career progression is intended to persist between launches.
12. Add end-to-end tests for screen transitions, career payout, and a complete race lifecycle.

## 16. Source basis

This reconstruction was derived from the current files under `src/`, the test files under `tests/`, and the Visual Studio project files in the repository. Implemented behavior is treated as authoritative; wording such as “proposed,” “intended,” and “inferred” marks behavior that is suggested by class names, UI branches, or design recommendations but is not complete or consistently reachable in the current build.
