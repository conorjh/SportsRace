# SportsRace Core

`SportRaceCore` is the C++20 static library for SportsRace's race simulation and career-world rules. Its Visual Studio project compiles the shared implementation from `src/`; the SDL screens and rendering code live outside this library.

## Main modules

- `Race.h` / `Race.cpp` — race, track, event, result, and timeline types. A seeded race precomputes entrant timelines, then supports playback and arbitrary-time position/state queries. Completed races can be captured as pointer-free history snapshots and reconstructed for result or node viewing.
- `Racer.h` / `Racer.cpp` — racer attributes, conditions, deterministic performance curves, career statistics, recent-result summaries, and the racer database.
- `Career.h` / `Career.cpp` — career data and profile types, race preparation and prediction, league-entry rules, race-history archive helpers, and the `CareerOrchestrator` API.
- `CareerWorldSimulation.cpp` — generated cities, regions and country; racer activities and travel; open and league race scheduling; league progression; race settlement and history links.
- `Core/Competition.h` / `Core/Competition.cpp` — round-robin fixtures and knockout playoff brackets.
- `Core/WorldClock.h` / `Core/WorldClock.cpp` — world-time scaling and safe clock advancement.
- `RaceState.h` / `RaceState.cpp` — race state-machine data and transitions used by the application.
- `Appraisal.h` / `Appraisal.cpp` — deterministic racer-rating estimates used by career scouting.
- `CareerSave.cpp` — versioned binary career persistence. It is included by `Career.cpp` because the core project uses an explicit source-file list.

## Data flow

1. `CareerOrchestrator::Init` creates the career roster and world; its update methods advance world time, activities, scheduled races, and league state.
2. Race preparation selects entrants and a track, then creates a `Race` whose seed and inputs determine its timelines and results.
3. Successful world and player races update racer records, fixtures or playoff matches, and the bounded career race archive. Racer-history entries and competition results reference archived races by ID.
4. Screens rebuild a replay from a saved snapshot, so historic results do not depend on transient live-race objects.

The race archive retains at most 128 completed replays. Older summary rows may remain in racer histories after their replay has aged out of the archive; only entries with an available archive can be opened as a replay.

## Career saves

`SaveCareer` and `LoadCareer` are declared in `Career.h`. The binary format is identified by `CurrentCareerSaveFormatVersion` and validates its header, payload, checksum, and serialized data. Loading requires an exact current-version match; incompatible older formats are rejected rather than migrated.

## Build and tests

Build the static library from a Visual Studio Developer Command Prompt at the repository root:

```powershell
msbuild "VisualC\SportRaceCore\SportRaceCore.vcxproj" /p:Configuration=Release /p:Platform=x64
```

The native regression tests are in `tests/Main.cpp` and are built by `VisualC\SportsRaceTest\SportsRaceTest.vcxproj`.

Follow the repository's `CODING_STYLE.md` for naming, formatting, and namespace rules.
