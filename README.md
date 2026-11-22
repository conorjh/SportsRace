# SportsRace

## Windows build

Install Visual Studio 2026 with the C++ workload and CMake. From the repository root, run `BuildDependencies.ps1` in PowerShell to build static SDL3, SDL3_image, SDL3_ttf, and SDL3_mixer libraries for Win32 and x64, in Debug and Release. The script writes the libraries under `external\static`; the Visual Studio project links those archives and does not copy SDL runtime DLLs. SDL_image is configured for the game's image formats, SDL_ttf uses vendored FreeType, and SDL_mixer keeps WAV and MP3 playback enabled.

Open `VisualC\SportsRace.sln` and build the desired configuration. Runtime display settings are available from the main menu's Options screen.

## Career world

Choose **Career** from the main menu to generate the persistent world. A new career visibly simulates its first 30 world days (one in-game month) before player-racer creation, so the AI has real race history at the starting line. The initial shared roster contains 1,000 racers, including two player-owned runners and 998 autonomous racers. The replacement policy keeps 998 AI racers active while retired records remain in the database. The world calendar advances at 24x (one world day per real hour), while races, travel, training, and recovery use normal real time.

- The autonomous racers travel between city locations, train and recover, enter scheduled league and open races, earn and spend cash, and carry activity statuses visible on the world map. Bankrupt racers retire and are replaced one-for-one to keep the AI population stable.
- The world starts with four full 20-runner divisions and assigns its AI population to regional 20-runner amateur leagues, keeping an open lobby available. Join an open amateur lobby in the **League Browser**; the league fills before its season starts, and another lobby opens when it fills. Race your scheduled league fixture from the same screen. Seasons use round-robin fixtures followed by a playoff bracket; the browser shows standings, fixtures, and brackets. Amateur racers work toward $2,000 in savings before applying for the $1,000 racing license needed to progress into Division 4.
- Career-world races always have six entrants, and tracks have no per-track concurrent-race cap. Independent background races advance in parallel, with their shared career results applied after the race ticks complete. Open a track building to see races currently in progress and observe the existing race at its current simulation time.
- The city starts as an 8x5 map with 50 randomly assigned race tracks for the initial 998 AI racers (100, 200, 250, 500, or 1,000 m); tracks grow at one per 20 AI racers and the map scrolls to additional districts as needed. Track buildings allow unlimited concurrent races and their live-race list scrolls to show every race. Its CC0 borough map is loaded from PNG for SDL_image compatibility, with city tiles rendered across the scenery. Left-click an enterable venue and hold right-click on a plot for details; use **Career hub** or Escape to return.
- The bookmaker, gym, license office, breeding clinic, player stable, and racer database each have their own screens. Racer profiles include recent race results. Press **F3** for a temporary debug menu with an option to spawn five AI racers into the live career world. Public races and basic wagers are open; breeding requires its license. Slot-machine and roulette minigames remain available at the bookmaker.
- **SportRaceCore** is the shared simulation/race library. **SportsRaceServer** runs the career world headlessly; `SportsRaceServer --ticks 10` runs a bounded ten-step simulation. Networking and persistence are not implemented yet.
- The borough-map background is an original CC0 vector asset at `media\city\sportsrace-borough-map.svg`, rasterized for runtime use as `media\city\sportsrace-borough-map.png`. Kenney Isometric Tiles City graphics remain in use for city tiles and building backdrops; see `media\city\sportsrace-borough-map-LICENSE.txt` and `media\city\LICENSE.txt` for licensing.

Career-world implementation details and prototype rules are recorded in [GDD.md](GDD.md).
