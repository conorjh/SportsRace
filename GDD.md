# SportsRace — Game Design Document

> **Document status:** Draft
> **Owner:** TBD
> **Version:** 0.1
> **Last updated:** 2026-12-01
> **Repository / project:** SportsRace

> **How to use this template:** Replace bracketed text with project-specific information. Remove sections that do not apply, or mark them **Not applicable**. Clearly label ideas as **confirmed**, **proposed**, or **open** so the document distinguishes decisions from unresolved ideas.

## 1. Game summary

### High concept

SportsRace is a satirical sports-business sandbox built around stats-driven foot races presented through the spectacle and culture of British horse racing: odds, betting, bookies, and commentary. The competitors are foot racers, not horses.

### Elevator pitch

Players inhabit a continuously simulated racing world and choose how to make money: own, breed, develop, buy, sell, and race runners; bet on or fix races; or own businesses such as bookies, tracks, and other venues. Races are shared events at the center of the world: players can enter them, bet on races, or watch an active race at its track. Single-player uses AI agents; a longer-term multiplayer version adds network clients to the same simulation engine.

| Item | Description |
|---|---|
| Genre | Sports-business sandbox / racing management simulation |
| Platform(s) | PC |
| Target audience | Mature audience; formal rating TBD |
| Player count | Single-player against AI agents initially; multiplayer later adds network clients to the same engine |
| Business / release model | Free-to-play; dedicated server software supplied for community self-hosting |
| Target session length | Player-directed; typical duration not yet defined |
| Target total playtime | TBD |

## 2. Vision and goals

### Vision statement

SportsRace is a persistent satirical world built around stats-driven foot races and framed by British horse-racing culture. Races anchor an economy where players earn money through the activities they choose, from owning, breeding, trading, and racing runners to betting, fixing races, or operating bookies, tracks, and other venues. Multiplayer is a longer-term goal.

### Design pillars

1. **Races as shared events** — Make stats-driven races the center of the world: players can enter them, bet on them, or watch them in progress.
2. **Player-chosen money-making careers** — Support distinct and mixable paths through runner ownership, breeding, racing, betting, bookmaking, venue ownership, and illicit schemes.
3. **Activity access** — Acquire licenses at dedicated buildings to unlock gated activities; licenses do not improve player stats.
4. **Satirical racing culture** — Draw on British horse-racing culture, seaside mechanical horse-racing games, and bookmaker virtual racing while keeping the competition itself foot racing.

### Goals

- Make races shared world events whose outcomes are driven by runners' skills, talents, and stats, without requiring every player to enter a runner.
- Support multiple ways to earn money, including runner ownership, breeding, racing, trading, betting, bookmaking, venue ownership, and illicit schemes.
- Make larger leagues and championships offer greater financial rewards.
- Support meaningful development and care for players who choose to own or race runners.
- Keep the game free-to-play without selling gameplay advantages.

### Non-goals

- Pay-to-win progression or purchases that provide gameplay advantages.
- Requiring every player to take part in every activity or to own and enter runners; players may specialize in other careers.
- Simulating horse racing literally: the athletes compete in foot races, while the surrounding culture borrows from British horse racing.
- A straight-faced, purely realistic presentation of the racing and gambling business; satire is central to the game's identity.

## 3. Player experience

### Player fantasy

Make a living in a satirical foot-racing world framed by British horse-racing culture. Develop, breed, buy, sell, and race runners; bet on or fix races; or own bookies, tracks, and other venues. Races connect these careers, but a player need not own or enter a runner.

### Intended emotions

- **Amusement:** Enjoy the parody of sports commentary, bookies, racing personalities, sportsmen's interview clichés, and the business around the sport.
- **Agency and curiosity:** Feel free to explore different roles and discover how the parts of the racing world connect.
- **Strategic satisfaction:** Feel rewarded for making plans and seeing the consequences play out.
- **Economic satisfaction:** Build wealth through a chosen specialty and see it shape the player's career.
- **Tension and surprise:** Feel uncertainty around races, bets, and risky choices, including illicit schemes, without losing sight of the game's satirical tone.

### Audience assumptions

- **Prior knowledge — assumption to confirm:** The game should be approachable without specialist knowledge of British horse racing or betting. Explain odds and other terminology in context.
- **Play style:** Players may prefer different activities and levels of involvement; the game should not assume everyone wants to manage every system.
- **Tone:** Players understand that the racing, gambling, and corruption themes are presented as satire. The writing also targets the brain-dead, cliché-ridden patter sportsmen use in interviews: stock phrases, empty motivational talk, and earnest non-answers. The exact limits and treatment of mature content remain to be defined.

## 4. Game structure and loops

### Game modes

| Mode | Description | Availability / unlock condition |
|---|---|---|
| Persistent world sandbox | Pursue any mix of money-making activities in the racing world against AI agents. | Single-player initially; multiplayer later adds network clients to the same engine |

### Core gameplay loop

**Target world gameplay loop (proposed):**

1. Pursue one or more ways to make money: own, breed, develop, buy, sell, or race runners; bet on or fix races; or operate bookies, tracks, gyms, and other venues.
2. Use the world and its buildings to act: enter a scheduled or open race, bet on a race, or watch an active race at its track.
3. Resolve every race type as a stats-driven simulation; race results determine placings, prizes, and betting outcomes.
4. Earn and spend money on runner purchases, development and care, race entries, performance-enhancing drugs, and businesses or venues. Larger leagues and championships pay more.
5. Purchase activity licenses at dedicated buildings to unlock restricted activities.

### Win, loss, and failure conditions

- **Success:** There is no formal win condition; players pursue self-directed goals through their chosen activities.
- **Failure:** Running out of money is the only failure condition.
- **Retry / continue:** The world continues; after running out of money, all of the player's owned assets are liquidated before they restart from the bottom. Licenses and progression reset; their character is the only thing retained and does not need to be remade. Exact liquidation mechanics and restart finances remain TBD.

## 5. Controls, camera, and interaction

### Control scheme

Players navigate and interact primarily with the mouse. Left-click enters an available building or activates a visible control; actions trigger once on the click edge. Hold right-click over a city plot for 300 ms to reveal its information panel. At city scale, move the pointer to a viewport edge to pan and use the mouse wheel to zoom around the pointer; zooming farther out opens the region and country views, where city and region markers are selectable. Escape or a visible Back control returns to the previous screen.

### Camera and view

The world map uses an isometric city view with edge scrolling, plus selectable region and country overview maps. Mouse-wheel zoom and explicit zoom controls navigate between these scales. Race simulations are shown in a side-on 2D view.

### Interaction rules

City buildings open with a left-click; vacant and scenery plots do not. A held right-click reveals a plot's name, type, and track length or availability. Building actions use single click-edge activation. Gym and dance activities accept their on-screen mouse controls. Escape or Back closes the current screen without selecting another action.

## 6. Gameplay systems and mechanics

The gameplay has two connected systems. **Race simulation is the primary gameplay system** and resolves races. **World simulation** maintains the randomly generated region, runner roster, buildings, and activities around them. The world simulation supplies entrants and venues to races and acts on race results; it does not define a second race resolver. Unresolved details remain open, and numerical tuning is deferred to prototyping.

### Race simulation

This is the main gameplay feature. Its subsystems are runner attributes, the Acceleration and Endurance curves, items, and match fixing, together with the race flow that resolves entrants into results.

#### Runner attributes and conditions

The following are current runner inputs and persistent conditions; exact scales and formulas remain open.

Spawned 0–100 skill ratings (Reflex, Composure, Acceleration, Endurance, Speed, Trainability, and Luck) use a bounded normal distribution centered at 50 with a standard deviation of 15, rounded to whole rating points and clamped to valid ranges. Developable ratings remain capped by Potential. Potential is a growth ceiling in the existing 70–100 range and uses a normal distribution centered at 85 (the midpoint of that range) with a standard deviation of 7.5.

**Mental stats**

| Stat | Intended role |
|---|---|
| Reflex | Controls response delay after the gun; the current range is 1000 ms at rating 0 to 10 ms at rating 100, before condition modifiers. A higher rating means a quicker response. |
| Composure | Influences confidence changes and performance under pressure. |

**Physical stats**

| Stat | Intended role |
|---|---|
| Acceleration | Controls pace buildup; see Acceleration and Endurance curves. |
| Endurance | Governs the runner's Endurance reserve. |
| Speed | Sets the runner's maximum velocity. |

**Hidden stats**

| Stat | Intended role |
|---|---|
| Potential | Sets the runner's hard ceiling for developable stats. |
| Trainability | Affects the rate of gym training and race-based development toward Potential ceilings. |
| Luck | Adds a uniform bonus of up to 5 percentage points to any event chance that uses Luck, based on the normalized Luck rating. The same bonus applies wherever Luck affects odds; it is not a direct speed bonus. |

**Runner details**

| Detail | Intended role |
|---|---|
| Name | The runner's display name. |
| Height | Uses a research-informed bell-curve distribution for the single unsexed runner population; reference data informs the shape, not hard bounds. |
| Weight | Uses a research-informed bell-curve distribution, sampled jointly with or conditioned on height to preserve their empirical relationship; exact fit TBD. |
| Age | Stored runner detail; growth and decline effects TBD. |

Runner sex is not a gameplay attribute; initial character sprites may be male-presenting.

**Persistent runner conditions**

| Condition | Intended role |
|---|---|
| Fatigue | Increases with racing, affects the runner's pace, and persists between races. |
| Confidence | Persists between races and starts at the midpoint (proposed 0.5 on a 0-1 scale). Wins raise it and losses lower it. Composure's influence on Confidence changes and performance under pressure remains TBD. |
| Injury | May affect performance and persists between races. Injuries have a small random chance in any race, with higher risk when racing fatigued. |

#### Acceleration and Endurance curves

| Curve | Behavior |
|---|---|
| Acceleration | Begins after the Reflex response delay. Its curve raises baseline pace from zero to Speed over a duration controlled by Acceleration; Endurance affects the resulting pace separately and does not change this curve. |
| Endurance | Begins at race time zero and runs concurrently with Acceleration. Generated Endurance ratings range from 20 to each runner's Potential (70–100). Before condition modifiers, ratings below 80 receive a duration-only bonus of `rating * (80 - rating) / 80` (a rating of 40 maps to a duration rating of 60 and lasts 162 seconds); ratings of 80 and above are unchanged (80 lasts 186 seconds), and a rating of zero keeps the 90-second baseline. At neutral conditions, reserve duration is 90 seconds plus 1,200 ms per duration rating point, four times the previous duration so Endurance depletes at one-quarter the previous rate. Generated values therefore span approximately 132–210 seconds. Endurance determines reserve depletion duration, independent of track length; the curve shapes how reserve drains. The remaining reserve linearly scales acceleration-curve pace from 100% at full reserve to 20% at empty reserve. Gassing begins when reserve reaches 10%, starts from 28% of pre-endurance pace, and holds reserve at 10% for its duration. |

Acceleration uses a cubic Bézier curve with four control points, P0–P3. Endurance uses the same curve construction. P0 and P3 are fixed for each curve type; P1 and P2 are the randomized handles. Coordinates use normalized time (`x`) and normalized pace progress or reserve (`y`). Acceleration ascends from P0 `(0, 0)` to P3 `(1, 1)`; Endurance descends from P0 `(0, 1)` to P3 `(1, 0)`. Endurance reserve scales the acceleration-curve pace by `0.2 + 0.8 * reserve`: full reserve applies no reduction, and zero reserve leaves 20% of the pre-endurance pace. Gassing begins at 10% reserve, where pace is 28% of the pre-endurance pace, and holds reserve at 10% while active.

For rising curves, constrain the handles to `0 <= x1 <= x2 <= 1` and `0 <= y1 <= y2 <= 1`; for Endurance, use `0 <= x1 <= x2 <= 1` and `1 >= y1 >= y2 >= 0`. Draw x handles uniformly from `[0, 1]` and sort them ascending. Draw y handles uniformly and sort them for each curve's monotonic direction; for Acceleration, raise P1.y into `[0.4, 1]` with `y1 = 0.4 + 0.6 * y1`, then raise P2.y if needed to preserve `y1 <= y2`. Endurance y handles remain sorted descending. Generate P1/P2 once per runner and curve type from a deterministic runner-specific seed and reuse them across races. P1 primarily shapes the curve's early progress; P2 shapes its later progress. At normalized elapsed time `u`, find `t` such that `B_x(t) = u`, where `B(t) = (1 - t)^3 P0 + 3(1 - t)^2 t P1 + 3(1 - t) t^2 P2 + t^3 P3`; use `B_y(t)` to interpolate between the curve's pace or reserve endpoints. Randomized handles affect shape only, not endpoints or duration.

#### Race-only effects

These are temporary race effects; persistent runner conditions are listed under Runner attributes and conditions.

| State / event | Intended role |
|---|---|
| Confidence modifier | During a race, Confidence provides a small temporary bonus to all effective runner stats without changing base stats; exact bonus scaling remains TBD. |
| Stumble | Race-only event that temporarily reduces pace for a short, randomly determined duration and plays the stumble animation while active. At the end, the runner continues from the reduced pace along their acceleration curve instead of snapping back to their pre-stumble pace. Its trigger rate is increased; higher Fatigue and lower Confidence increase its odds. Exact odds, pace loss, and duration remain TBD. |
| Second Wind | While Gassing, Second Wind is sampled as a continuous-time event with the stated per-second chance: the base chance is 0.01%; Confidence adds up to 0.125 percentage points and Luck up to 0.25 percentage points, for a maximum of 0.385% per second. On success, Gassing ends, pace is overridden to its pre-Gassing level for 50 ms, and Endurance reserve is restored to 35–50% of its full amount. Luck adds up to 5 percentage points and Confidence up to 10 percentage points to the 35% base restoration. The “Second wind” label remains visible for one second. A later depletion can start another Gassing period and allow another Second Wind; there is no race-wide restoration cap. |
| Pulling up | A runner pulls up if a continuous Gassing period reaches its limit: 1 second when both Composure and Confidence are zero, up to 5 seconds when both are at their maximum. The limit interpolates linearly from the mean of normalized Composure (0–100) and Confidence (0–1). A Second Wind ends that Gassing period and its timer; a later Gassing period starts a new timer. A pull-up stops the runner, records a “Pulled-up” result without a numeric placing or placing prize, and does not stop the remaining runners. |
| Lost Belief (working name) | Race-only condition that lowers pace; it can occur only below 0.2 Confidence, with lower Confidence increasing its odds. |
| Believes in Self (working name) | Positive race-only trait that raises pace through a small stat boost; it can occur only above 0.8 Confidence, with higher Confidence increasing its odds. |

#### Items

Players can buy and give steroids to runners; certain steroids can raise persistent Confidence. Steroids are the current item example, not a complete item catalog, and more items can be added as they are designed. Steroid use is not detected in amateur or unlicensed races. Licensed leagues may randomly check before or after selected races and issue temporary bans; the catalog, other effects, check rules, and penalties remain open.

#### Match fixing

Players can fix races. Amateur and unlicensed races have no chance of detecting race fixing. Enforcement in licensed competition is deferred; methods, detection, and penalties remain to be designed.

#### Race flow and resolution

1. **Entry and lock:** Career-world regular-season league and open races start with six runners; all playoff and knockout matches start with only their two bracketed competitors. Other race modes close entry at their configured start. Entrants, track, finish distance, and race seed lock at that point. Racer stats, conditions, and curve handles remain unchanged during the race and are read directly from each entrant.
2. **Runner timeline:** Initialize each runner at distance zero and evaluate the Acceleration and Endurance curves using the entrant's existing stats, conditions, and curves. Derive effective stats for the race without changing base stats. Use the race seed for event rolls; trigger timing and simultaneous-effect priority remain open.
3. **Movement and finish or pull-up:** At each event boundary, update active effects and calculate effective pace from the curve state and those effects. Integrate effective pace between boundaries to update distance; a runner finishes when its distance reaches the track's finish distance. A runner that remains Gassing beyond its Composure/Confidence-based limit pulls up and stops; other runners continue. Record distance, pace, active effects, and event times throughout the race.
4. **Race result:** Resolve the complete timeline before playback. Rank finishers by finish time and record pulled-up runners as “Pulled-up” without a numeric placing. Pass the results to the world simulation. Tie, disqualification, and other non-finish rules remain open.
5. **Playback:** Spectators read the same race timeline at a shared playback time; rendering cannot change the outcome. Identical entrant inputs and race seed produce identical timelines and results.

### World simulation

The world simulation maintains the region, racer roster, venues, leagues, and live races. A new career begins with no player-owned racers and no stable lease. It simulates at least its first 7 world days (one in-game week), then continues until every active AI racer has completed a race so late fixtures do not leave the world without history. The simulation starts with 490 active AI racers, each with $1,000 and a location, cash balance, and activity state. The player starts with $5,000, rents stable space in the home city for $1,000, and can generate and purchase breeder prospects for $1,000 each; a stable holds up to six racers. The world calendar advances at 24x, so one real hour equals one world day; league schedules and seasons use this accelerated clock. Travel, training, recovery, and background race duration use elapsed real time, so they are not accelerated with the calendar. Background races precompute their node timelines once, advance from elapsed real time, and are sampled by the viewer only when spectated; player-run races retain fixed playback ticks. Bankrupt racers retire, and each retirement immediately creates one replacement so the active AI population remains stable. Independent race updates and per-racer activities run in parallel; the main simulation thread applies completed race results, bankrupt replacements, and shared league/economy updates serially.

The generated country contains named regions with capital cities, cities with varying populations, and a capital region for each country. Population weights AI city assignment. Every racer has a permanent based-in city. Amateur league vacancies are limited to racers based in the league's city or region; joining a numbered city, regional, or national division marks the racer as professional. The world map opens at the home city (or current stable city) and zooms to a selectable region and country view; city and region overview panels show population and capital details. Each city and region has four 12-runner divisions, and each country has the same four national divisions; all divisions are seeded at career start. City Amateur leagues and the open home-city Amateur lobby each hold eight runners. City Division 1 feeds Regional Division 4, Regional Division 1 feeds National Division 4, and the national divisions form a ladder up to Division 1. Regional and national knockout cups run alongside the daily AI-only Amateur Cup. The League Browser shows competition scope, standings, fixtures, heats, and brackets, and lets the player race their next scheduled heat. Each 12-runner Division 1-4 league schedules three fixtures per season; each fixture contains two full-field six-runner heats, so every member races once at each fixture and makes three starts. Standings award points by placing in each heat. Each eight-runner Amateur league schedules one full-field heat per season, rotating the two resting entrants between seasons. Regular-season league, open, exhibition, and other non-knockout races use six entrants; playoff and cup knockout matches use only their two bracketed competitors. Tracks have no per-track concurrent-race cap. League races have no entry fee; first-place prizes scale by division. Mixed-division heats use the highest represented division's prize, while standalone open races retain their entry fee. Unassigned amateurs can also enter independent open races at city tracks; their field is reserved while entrants travel, and track assignments rotate. Background races precompute at their scheduled start and settle after elapsed playback time; spectators sample the node timeline on demand. A track building opens a scrollable list of all live races, letting the player inspect an existing race rather than starting a duplicate simulation.

Every race entry awards 10 XP; first, second, and third place earn 15, 10, and 5 additional XP respectively. A pulled-up runner receives only the entry XP. XP is a 64-bit career counter that accumulates beyond 100; it is not a 0-100 skill rating. Finishes award 3, 2, or 1 positional points for first, second, or third place. Every result updates races entered, wins, losses, points, XP, and best finish time. Each racer retains up to 256 recent race-history entries with the world day, event, track distance, placing, and finish time; the Racer Database exposes those entries in a history view. The RacerDB rank is ordered by positional points, then wins, XP, best time, name, and racer ID. The world also retains a bounded history of race events for live-race viewing.

Press F3 to open the temporary career debug menu from any screen. Its spawn action adds five named AI racers to the active world; those racers receive starting cash, a location, and normal career-world scheduling.

#### World map

Each world starts as a randomly generated 8x5 isometric city with municipal scenery, five vacant lots, a gym, bookmaker, license office, breeding clinic, available stable space, and 25 race tracks for the initial 490 AI racers. Track count grows proportionally at one per 20 AI racers; vacant lots and scenery are converted first, then additional rows can be explored by panning. The map uses selected CC0 sprites from Kenney's Isometric Tiles Buildings, City, and Vehicles packs. City scenery tiles repeat in a 4x3 pattern aligned to fixed 4x3 street blocks. Streets are rendered from a shared connected network, so intersections and right-angle traffic routes stay continuous; the Career Hub preview reuses the World Map artwork and road layout. Building-type badges above each venue identify tracks, gyms, stables, and other services. Civilian cars follow the connected road loops while occasionally sounding a soft synthesized horn. Use the mouse wheel over the map to zoom around the pointer and move the pointer to a viewport edge to pan. Hold right-click on a plot to inspect it; left-click enters an available building. Scenery is unavailable and vacant lots are not yet buildable. Each city track receives a generated length of 100, 200, 250, 500, or 1,000 metres and has no concurrent-race cap. Asset sources and license details are in `media/city/LICENSE.txt`.

#### Runner roster and ownership

Players can own and enter runners; the player starts with an empty stable while all 490 AI racers remain in the population database. After leasing home-city stable space, the player can generate and buy prospects at the breeder; a stable holds up to six racers. AI runners have independent cash and league membership, and can train, travel, rest, race, earn a racing license, retire, and be replaced. Genetic breeding adds offspring to both the population database and the owned stable. Acquiring or selling existing AI runners remains future work. Race participation awards every finisher growth across physical stats, scaled by finishing position and capped by Potential; race-based growth is faster than gym-based growth. Exact growth rates, placement scaling, and market rules remain open for prototyping.

#### Training and recuperating (gym)

The prototype gym has three point-and-click minigames for an owned runner: a five-rep treadmill timing game trains Endurance, a narrower timing window on the weights trains Acceleration, and a five-direction dance sequence trains Reflex, Composure, Acceleration, Endurance, and Speed when completed perfectly. Successful timing improves the focused stat by 0.2 per good rep and adds Fatigue; a perfect dance raises each of the five skills by 1. Skills remain capped at 100. Recovery, gym ownership, machine tiers, and membership fees remain future work.

#### Breeding

The breeder can generate and sell a new prospect for $1,000 after the player rents stable space. Genetic breeding still requires the breeding license, costs $1,000 per pairing, and accepts two distinct owned runners. Offspring inherit the mean of both parents' skill ratings, Potential, Trainability, and Luck, with normally distributed mutation (standard deviation 4) clamped to 0-100; height and weight are averaged and the offspring starts at age zero. Inheritance weights, aging, and further breeding rules remain open for prototyping.

#### Bookies and betting

The prototype bookmaker displays odds for the scheduled six-runner race and accepts a pre-race wager on one runner. It estimates each runner's strength directly from Speed, Acceleration, Reflex, Endurance, Composure, Luck, and race-day conditions; longer tracks shift weight from acceleration to endurance. Relative strength scores produce estimated win probabilities without simulating races. The bookmaker has 98% appraisal accuracy; a small deterministic estimation drift and a 10% house margin are applied when producing decimal odds. A wager is settled against the final race result. The screen also contains a $25 slot-machine minigame (a pair pays $50; three matching symbols pay $250) and $25 red/black European roulette (a winning color returns $50). Bookie operation and operating costs remain future work; live world races can be viewed at their track buildings. Placing a basic wager does not require the operator license.

#### Licenses

The player-facing license office sells four independent activity licenses: Racing ($500), Breeding ($1,000), Bookie Operator ($2,500), and Track Operator ($4,000). These are prototype prices and provide no player-stat bonuses. Basic betting and breeder-generated purchases are available without a license; genetic pairing is gated by the Breeding license. AI amateurs aim to save twice their $1,000 starting cash, then pay $1,000 for the racing license; licensed amateurs can qualify for Division 4 as league places open. A player-owned racer can join a local Amateur league; venue operation remains future work.

#### Tracks

The generated city-track buildings host scheduled league races, periodic open races, and player events on generated 100-1,000 m distances. A track has no concurrent-race cap. Each building lists every active world race, with a scrollable list, and opens the shared race at its current playback time. Player-prepared events cycle through owned stable runners, with matched AI opponents filling any remaining places. An open wager is settled after the finish. Track-operator licenses currently have no gameplay effect because player-owned venues are not implemented.

#### Stat appraisal

Appraisal provides deterministic, player-specific runner-stat estimates. Each player profile receives a hidden Accuracy rating from 1% to 99%, fixed for that profile. Estimates use the player's appraisal identity and accuracy, the runner's identity, name CRC32, skills and body details, and the identity and base value of the inspected stat. The estimate is drawn from a normal distribution centered on the true rating, with standard deviation `(100 - Accuracy) * 0.25` rating points; the result is rounded and bounded to 0–100. At 100% accuracy the appraisal function returns the exact base rating. For example, 90% accuracy gives a 2.5-point standard deviation, making extreme estimates exceptionally unlikely for an ordinary mid-range stat.

Appraisals are calculated when inspected and are never saved on the player or runner. Identical player, runner and stat inputs produce the same estimate; changing the runner's name or any current skill/body stat produces a new deterministic estimate. The current distribution mapping is an initial prototype value and remains subject to tuning.

## 7. Progression and rewards

### Player progression

Players progress by pursuing chosen careers, acquiring activity licenses, and developing their owned runners. AI league progression is described below; player participation in those AI seasons, license balance, and rewards remain subject to prototyping.

### Content progression

The prototype has four 12-runner city divisions per city, four 12-runner regional divisions per region, four 12-runner national divisions, eight-runner city Amateur leagues, and an open home-city Amateur lobby. Every numbered league runs three fixtures per season, each with two six-runner heats so all 12 entrants race once at every fixture. Amateur leagues run one six-runner heat and rotate two resting runners between seasons. Other non-knockout races use six-runner fields; playoffs and cup knockout matches remain 1v1. City and regional divisions feed into the next geographic tier, and the national divisions form a ladder; regional, national, and Amateur knockout cups run alongside the leagues. The League Browser exposes competition scope, standings, fixtures, heats, and brackets.

### Rewards and economy

| Reward / resource | Earned from | Spent on | Limits / rules |
|---|---|---|---|
| Money | Race prizes and winning wagers. | Stable lease, breeder prospects, genetic pairing, licenses, and other listed services. | Players start with $5,000; lease and prospect prices are prototype-tuned. |

### Save and persistence

The client saves the career world, profiles, racers and histories, geographic data, leagues, schedules, finances, stable lease, and simulation clocks to `saves\career.srsave` from the Career Hub; the main menu loads that save. The binary format is currently version 4. Version-3 saves are migrated, restoring the home-city stable lease for existing starter rosters and upgrading legacy league structures to the current schedule; unsupported older or newer formats are rejected. Bump the version whenever the serialized layout changes. Saves are available after the initial career simulation completes. Active race machines and transient race queues are not serialized; on load, interrupted races are discarded and unresolved league fixtures are scheduled again. Backups and offline progression are not implemented.

## 8. World, levels, and content

### Setting and world overview

The competition is foot racing presented through the spectacle and culture of British horse racing. World-map generation and simulation are specified in Section 6.

### Level / event structure

This section catalogs world content; gameplay rules for races and buildings are specified in Section 6.

| Content ID / name | Purpose | Key challenge | Unlock / prerequisites |
|---|---|---|---|
| Racetrack | Public amateur race venue with a generated track distance. | Track layout and ownership remain open. | None for prototype events |
| Bookie | Predicted odds, pre-race wagers, slots, and roulette. | Business operation remains future work. | None for prototype activities |
| Gym | Treadmill, weights, and dance training minigames. | Recovery and gym ownership remain future work. | None |
| Breeding clinic | Generate and buy a new prospect, or breed two owned runners. | Inheritance and offspring generation; see Section 6. | Stable lease for prospect purchases; breeding license for pairing |
| Player stable | Rent space for up to six racers or inspect the owned roster. | Empty until the player leases stable space and buys racers. | Career Hub or World map > stable |
| License building | Sells activity licenses. | See Section 6. | Payment |
| League / championship | Persistent AI league seasons across amateur and Divisions 1-4. | Three fixtures of paired full-field heats per numbered division season, standings, playoffs, promotion/relegation. | Twelve-member numbered divisions; eight-member Amateur leagues; AI racing-license progression |

### Content variety and replayability

[Procedural generation, variations, randomization, branching, or repeat-play support.]

## 9. User interface and user experience

### Screen list

| Screen / view | Purpose | Entry point | Main actions |
|---|---|---|---|
| Main menu | Start a standalone race or career, open display options, or exit. | Application start | Race, Career, Options, or Exit |
| CareerHubScreen | Show the current stable city, or the world map before the player rents a stable; provide top and bottom icon navigation. | Main menu > Career, after the initial world simulation | Open the League Browser, database, stable/roster, breeder, world map, or save career |
| WorldMapScreen | Explore the isometric career city, zoom to its region and country, inspect capitals/populations, and view live world activity. | CareerHubScreen > World map | Edge-pan the city; zoom or select overview markers; enter home-city buildings; hold right-click for plot details; open the population database or League Browser |
| BookiesScreen | View race odds and place wagers; open the slot machine or roulette. | World map > bookmaker | Select a runner and stake; open either dedicated game screen; return to the city |
| SlotMachineScreen | Play the bookmaker's slot-machine game. | BookiesScreen > slot machine | Set a stake and spin; return to the bookmaker |
| RouletteScreen | Play the bookmaker's roulette game. | BookiesScreen > roulette | Set a stake and place a bet; return to the bookmaker |
| GymScreen | Train an owned runner with the treadmill, weights, or dance minigame. | World map > gym | Select a runner and station; complete the activity; return to the city |
| LicenseOfficeScreen | Purchase activity licenses. | World map > license office | Review license costs and status; purchase an available license; return to the city |
| BreedingFarmScreen | Generate and buy a new prospect or breed two owned runners. | Career Hub or World map > breeder | Buy a generated prospect after renting stable space; select two licensed parents to breed; return to the city |
| RaceTrackScreen | Enter a player race or inspect live races on the selected city track. | World map > race track | Review the field and distance; start a player race or open a shared live race; return to the city |
| LeagueBrowserScreen | Browse generated leagues, join an open amateur lobby, race the next player heat, and inspect standings, race schedules, and playoff brackets. | CareerHubScreen or WorldMapScreen > League Browser | Select an open amateur league and join; race a scheduled player heat; inspect the season schedule or bracket |
| StableScreen | Rent home-city stable space or inspect the player's owned runners. | Career Hub or World map > stable | Rent a six-runner space; select a runner and review its profile; return to the city |
| RacerDBScreen | Compare every generated runner in a scrollable, sortable leaderboard of records and appraised ratings, then inspect the selected runner's full report. | Career Hub or World map > Racer database | Sort by any displayed column; select a runner; drag the scrollbar or use the mouse wheel; temporarily hover over a stat to reveal its true base value for debugging; return to the previous career screen |
| OptionsScreen | Change the windowed resolution and switch between windowed and desktop-fullscreen modes. | Main menu > Options | Choose a windowed-resolution preset; toggle fullscreen; resize the window; return to the main menu |

### Navigation flow

Main menu > Career > (new-career week simulation with progress) > Career Hub. The Hub shows a world-map view until stable space is rented, then centers the leased city map; top icons open the League Browser and database, while the stable icon opens the roster and lease screen. Bottom icons open the wider World Map, breeder, and save action. Rent space at the home-city stable, then generate and buy prospects at the breeder before entering races or joining an Amateur league. The wider World Map remains available for browsing city, region, and country views. The stable shows only player-owned runners; the database includes the full population. Owned racer names use a consistent gold highlight wherever they appear in lists, race fields, profiles, and results. Use Career hub or Escape to return from the city, and Back or Escape to return from a building. Main menu > Options opens window-size and fullscreen controls. The window opens at a size derived from the desktop mode and can be resized; screen layouts reposition content for the current aspect ratio while keeping controls proportional rather than stretching them. Display choices are runtime-only.

### HUD and information hierarchy

[What is always visible, what is contextual, and what information has priority.]

### Notifications and feedback

[Errors, confirmations, tutorials, objective updates, results, and other messages.]

## 10. Art direction and animation

### Visual direction

[Style, mood, color palette, composition, readability, and visual references.]

### Characters, environments, and effects

[Visual requirements and distinguishing details.]

### Animation principles

[Timing, transitions, expressive goals, and gameplay readability.]

### Asset requirements

| Asset | Use | Priority | Status / owner |
|---|---|---|---|
| Kenney Isometric Tiles Buildings | Isometric venue and building sprites. | Should | Selected CC0 PNGs in `media/city/isometric/buildings`; source and license in `media/city/LICENSE.txt` |
| Kenney Isometric Tiles Landscape | Isometric ground and terrain tiles. | Should | Selected CC0 PNGs in `media/city/isometric/landscape`; source and license in `media/city/LICENSE.txt` |
| Kenney Isometric Tiles City | Isometric city scenery and plot sprites. | Should | Selected CC0 PNGs in `media/city/isometric/city`; source and license in `media/city/LICENSE.txt` |
| Kenney Isometric Tiles Vehicles | Ambient civilian traffic sprites. | Should | Selected CC0 PNGs in `media/city/isometric/vehicles`; source and license in `media/city/LICENSE.txt` |
| SportsRace Borough Map | Legacy illustrated world-map background. | Should | Original CC0 SVG and raster copy retained in `media/city`; license in `media/city/sportsrace-borough-map-LICENSE.txt` |

## 11. Audio and music

### Audio direction

World-map traffic occasionally plays a quiet, synthesized two-tone horn. It is intentionally infrequent and low-volume so it adds city ambience without competing with race audio.

### Music

[Themes, adaptive music, transitions, and silence requirements.]

### Sound effects and voice

Race commentary and brief interview segments carry the sporting satire. Interview patter should parody stock clichés, empty motivational answers, and earnest non-answers; voice and subtitle requirements remain TBD.

## 12. AI and difficulty

AI agents populate the single-player sandbox and may combine player-like careers rather than specialize. The current prototype simulates their cash, location, travel, training, recovery, license progression, league and open-race entry, retirement, and replacement. Decision priorities and economy balance remain open for tuning.

- **Difficulty model:** TBD.
- **Fairness/readability rules:** TBD.

## 13. Multiplayer and online features

- **Player count and roles:** Single-player uses AI agents; multiplayer later adds networked players to the racing world.
- **Session flow:** Joining, leaving, and connecting to a hosted world are TBD.
- **Networking model:** Reuse the single-player simulation engine with network clients added. Authority, replication, and hosting details are TBD.
- **Competitive rules:** Players and AI agents participate in the same race and economy systems; detailed rules are TBD.
- **Disconnect and abuse handling:** TBD.

## 14. Monetization and live operations

[Optional. Include only if applicable.]

- **Monetization model:** [What is sold, if anything]
- **Earnable versus paid content:** [How access is divided]
- **Time-limited content:** [Events, seasons, rotations]
- **Player protections and transparency:** [Costs, odds, limits, and disclosures]

## 15. Accessibility and localization

### Accessibility

[Text scaling, contrast, colorblind support, remapping, subtitles, difficulty options, and other needs.]

### Localization

[Supported languages, text expansion, fonts, voiceover, cultural review, and regional requirements.]

## 16. Technical and production constraints

### Technical assumptions

| Area | Requirement / decision | Owner / status |
|---|---|---|
| Engine / framework | C++ with SDL3; Visual Studio solution builds the client and related projects. | Implemented prototype |
| Target hardware | [Minimum and recommended specs] | [Owner / status] |
| Performance targets | [Frame rate, load times, memory, etc.] | [Owner / status] |
| Shared simulation | `SportRaceCore` is the static library shared by the client, tests, and headless host; it owns the race and career-world simulation. | Implemented prototype |
| Headless host | `SportsRaceServer` runs the career simulation without the graphical client; it currently has no network transport. | Implemented prototype |
| Data / save format | Career state is held in memory; persistent save and recovery are not implemented. | Open |
| Race simulation | Implement the race-flow rules in Section 6. | Implemented; event schema and storage structure remain implementation details |
| Race rendering | Implement the playback rules in Section 6. | Proposed; exact playback speed TBD |
| AI and multiplayer | AI racers operate in the client career and headless host; network authority and replication remain future work. | AI prototype implemented; networking TBD |
| Third-party services | [Dependencies and constraints] | [Owner / status] |

### Content pipeline and tools

[How content is authored, imported, reviewed, and delivered.]

### Risks and mitigations

| Risk | Likelihood / impact | Mitigation | Owner |
|---|---|---|---|
| [Risk] | [Low/Medium/High] | [Response] | [Owner] |

## 17. Testing, analytics, and success criteria

### Quality and playtesting goals

- Treat numerical values—including stat ranges and weightings, random distributions and probabilities, license and building costs, income, odds, payouts, and the exact real-to-world-time conversion—as prototype-tuned rather than fixed in this document.
- Build a repeatable gameplay test suite covering runner generation, race simulation and events, world systems, progression, economy, and AI activity; use results to tune prototypes and catch regressions.

### Success criteria

| Goal | Measurement | Target / decision threshold |
|---|---|---|
| Deterministic race generation | Repeat full-race simulation with identical entrant inputs and seed; verify racer stats, conditions, and curves remain unchanged during the race. | Identical event timelines and results; after-race condition changes are applied only on completion |
| Curve generation | Generate Acceleration and Endurance curves across runner seeds; sample each from normalized time 0 to 1; vary Acceleration, Speed, Endurance, and track length independently. | P0/P3 are fixed by curve type; P1/P2 reproduce for the same runner seed and vary across seeds; Acceleration P1.y is at least 0.4; Acceleration rises and Endurance falls monotonically; Acceleration reaches and holds its baseline Speed; Endurance drains reserve and scales pace from 100% at full to 20% at empty without changing Acceleration; Gassing starts at 10% reserve and holds reserve there until it ends; Endurance duration is unaffected by track length |
| Race resolution order | Simulate known runner inputs and inspect the generated timeline, event boundaries, distance, and finish time before playback. | Entrant stats and conditions determine effective stats, curve timing, pace endpoints, and event odds; Reflex delay precedes Acceleration; Endurance depletes concurrently; event and Endurance transitions are applied before effective pace advances distance; finish time is the first track-distance crossing |
| Career background races | Observe the live autonomous race pool on each track. | Numbered league fixtures pair two six-runner heats so all 12 entrants race once per fixture; open races and other non-knockout races use six entrants; playoff and cup knockout matches use the bracketed pair; an AI runner is never in two live races; all six heat entrants receive finish-position standings points, while only bracketed competitors advance in knockouts; background timelines precompute once and elapsed real time advances them, with viewers sampling nodes on demand |
| Career ranking and XP | Record known first-, second-, third-, fourth-place, and pulled-up results; inspect the RacerDB and sort each column both ways. | Results award 3/2/1 positional points and 10 base XP plus 15/10/5 podium bonus XP; XP accumulates beyond 100; pulls receive base XP only; rank uses points, wins, XP, best time, name, and ID; all displayed columns sort correctly |
| Career world generation | Start careers with different seeds, grow the AI population, inspect geography, competitions, and city tracks. | New players own no racers and have no stable lease; the initial AI population is 490; renting stable space and buying breeder-generated prospects persist; cities have varying populations and capital markers; map selection navigates city/region/country levels; each city starts with 40 plots in an 8x5 grid and 25 race tracks |
| Building screen routing | Enter each available building from the map and exercise its activity. | The gym, bookmaker, license office, breeding clinic, each track, and stable open their matching dedicated screen classes |
| Scheduled race precomputation | Admit entrants through the scheduled start and observe precomputation time. | Entries close and the timeline is generated at the scheduled start; the entrant list and race configuration are fixed |
| Open race precomputation | Observe an open world race as its field fills and starts. | It starts only when exactly six eligible racers are ready; entrants and race configuration are fixed at start |
| Race timeline rendering | Sample each runner's events at arbitrary race times across multiple spectators. | Reproducible position and animation at the same shared playback time, without renderer-driven simulation |
| Live race information | Inspect data and playback position for spectators who join an in-progress precomputed race. | Join at the current live race time, synchronized with other spectators; no future events or final result before the finish |
| Betting cutoff | Attempt to place wagers before and after the race starts. | Pre-race bets are accepted; no wagers are accepted once the race has started |
| Bookmaker strength estimate | Compare racer strength on short and long tracks and change race-day conditions. | Strength reflects race-relevant skills and conditions; longer tracks favor endurance over acceleration; odds are estimated without simulating a race |
| Runner-stat appraisal | Recompute estimates for identical and changed player/runner/stat inputs; sample the accuracy distribution at representative ratings, including 90% accuracy. | Identical inputs are deterministic and never persisted; changing runner stats changes the appraisal seed; player accuracy stays fixed from 1% to 99%; at 90% accuracy, extreme estimates for a base rating of 50 are exceptionally unlikely. The initial error standard deviation is `(100 - Accuracy) * 0.25` and remains open to prototype tuning |
| Gameplay balance | Run end-to-end simulations across runner generation, races, development, world activities, economy, and AI behavior. | Identify cross-system balance issues; numerical targets and tolerances are set through prototyping |
| Runner body generation | Generate representative batches of runners at spawn. | Height and weight match the research-informed bell-curve distributions and preserve their empirical relationship; fitted parameters and tolerances set through prototyping |
| Runner recovery | Compare rest and gym recovery for race fatigue and injuries. | Both recover through either option; gym recovery is faster and paid, with rates and costs tuned through prototyping |
| Persistent runner conditions | Simulate repeated races, rest/gym recovery, wins and losses, and steroid use. | Fatigue, Injury, and Confidence persist between races; racing increases Fatigue; Injury can occur in any race with higher risk while fatigued; Confidence starts at 0.5 on the proposed 0-1 scale, rises with wins, falls with losses, and can be raised by certain steroids. Exact probabilities and changes are prototype-tuned |
| Runner growth through racing | Compare race-driven changes to all physical stats across every race type under matched starting stats, sufficient Potential headroom across all physical stats, and gym-training exposure. Vary finishing position at fixed Trainability, then vary Trainability at fixed finishing position; verify every finisher, including last place, receives growth, higher placements yield more, every physical stat receives equal gains up to its Potential ceiling, and mental and hidden stats gain nothing from racing. Include a non-racing control. Compare race and gym growth under matched conditions and verify race-based growth is faster. | Every race type gives every finisher finishing-position-scaled growth equally across all physical stats, subject to Potential ceilings; race-based growth is faster than gym-based growth, with exact growth rates, per-placement values, and Trainability scaling tuned through prototyping |
| Race-only effects and events | Simulate Gassing, Pulling up, Stumble, Second Wind, Lost Belief, and Believes in Self while varying Composure, Fatigue, Confidence, and Luck; measure effective stats, pace, trigger odds, durations, and Endurance restoration. | Verify the pull-up limit increases monotonically with Composure and Confidence and never exceeds 5 seconds, a pulled-up runner stops without a numeric placing, and other entrants continue; verify Stumble's increased trigger rate and odds increasing with Fatigue and decreasing with Confidence, no snap back to pre-stumble pace, and continued acceleration along the curve; Lost Belief only below 0.2 Confidence; Believes in Self only above 0.8; Second Wind's per-second chance does not exceed the documented 0.385% maximum; the “Second wind” label remains visible for one second; restoration reaches but never exceeds 50% per event; the one-step pace override; and no change to base stats from temporary bonuses |
| City interactions | Inspect map plots and activate buildings with controlled mouse input. | Type badges distinguish buildings; right-click inspection appears only after a hold; left-click enters an available building once per press; scenery and vacant lots cannot be entered |
| License-gated activities | Attempt breeding and public racing with and without the relevant licenses. | Genetic pairing is blocked without its license; breeder-generated purchases, public amateur races, and basic wagers remain available without a license; operator licenses do not grant unimplemented venue functions |
| Career building activities | Exercise the gym minigames, bookie games, breeding clinic, and stable with representative player data. | Valid actions update only the intended owned-racer stats, finances, offspring, or race bet; invalid actions report why they cannot proceed |
| Server mass simulation | Measure completed races per second under representative load. | TBD |

### Analytics and privacy

[Events to measure, purpose, consent, retention, and data-minimization requirements.]

## 18. Scope and roadmap

### Scope definition

- **Must have:** [Required for the first release / milestone]
- **Should have:** [Important but not release-blocking]
- **Could have:** [Optional if time allows]
- **Out of scope:** [Explicitly excluded]

### Milestones

| Milestone | Deliverables | Dependencies | Target date / status |
|---|---|---|---|
| [Milestone] | [Deliverables] | [Dependencies] | [Date / status] |

## 19. Open questions and decisions

| Question / decision | Options | Owner | Due date | Status |
|---|---|---|---|---|
| Activity license balance | Tune player license prices and the AI racing-license threshold and fee. | TBD | TBD | Prototype prices; balance open |
| World calendar and time | Tune the current 24x world-clock rate and define pause and offline progression; keep timed activities on real time. | TBD | TBD | 24x and real-time activity clocks implemented; balance and offline behavior open |
| Bankruptcy and recovery | Tune bankruptcy thresholds, replacement finances, and asset-liquidation mechanics. | TBD | TBD | One-for-one AI replacement implemented; balance open |
| Runner-stat appraisal accuracy | Fit the candidate bounded bell-curve distribution and map accuracy to estimation error. | TBD | TBD | Deferred to prototyping |
| Runner growth | Tune race and gym growth rates, placement scaling, Trainability, and any age or body-detail effects. | TBD | TBD | Open |
| Training and recuperating | Tune treadmill, weights, and dance-game skill effects; determine recovery rates and future gym purchase and membership economics. | TBD | TBD | Open |
| Breeding | Tune stat inheritance and the prototype's normal-distribution mutation (standard deviation 4). | TBD | TBD | Open |
| Persistent runner conditions | Set Fatigue and Injury scales and probabilities, Confidence change amounts, and Composure's influence on Confidence changes and performance under pressure. | TBD | TBD | Open |
| Runner height/weight distributions | Fit the research-informed height and weight distributions and their empirical relationship. | TBD | TBD | Open |
| Pace and Endurance curves | Tune Acceleration's duration and Endurance depletion timing; retain the 10% Gassing threshold and reserve hold, and tune Gassing's decline profile. | TBD | TBD | Open |
| Betting and bookmaking economy | Tune the 32-simulation prediction, 10% house margin, slot and roulette payouts, and future operating costs/income. | TBD | TBD | Open |
| Steroid rules | Determine the item catalog and effects, licensed-race check rates and timing, and ban durations. | TBD | TBD | Open |
| Race-fixing enforcement | Design detection and penalties in licensed competition. | TBD | TBD | Deferred to later design |
| Buildings and venue ownership | Define buildable vacant lots, city expansion, venue operation/ownership, gym tiers, and associated costs and income. | TBD | TBD | Open |
| League and championship progression | Tune progression balance, payouts, and player access to AI seasons. | TBD | TBD | Full-field league heats, automatic promotion/relegation, and a daily playoff-only Amateur Cup implemented; balance remains open |
| Event-driven race model | Define event-trigger timing, simultaneous-effect priority, and event schema; tune odds and effect magnitudes, decide any additional event types, and resolve tie, disqualification, and non-finish handling. | TBD | TBD | Open |
| Race input lock and schedule | Decide whether non-career race modes share the six-runner field. | TBD | TBD | All non-playoff race types use six entrants; playoff matches use two |
| Race playback timing | Set the playback-speed mapping from race-relative time. | TBD | TBD | Open |
| AI and multiplayer integration | Tune AI activity priorities; determine network authority, replication, and multiplayer session flow. | TBD | TBD | AI career simulation implemented; networking open |

### Decision history

Current rules are specified in their owning sections; this log preserves the rationale for the main design directions.

| Date | Decision | Rationale |
|---|---|---|
| 2026-11-13 | Foot racing is presented through British horse-racing culture and satire. | Borrow familiar spectacle while keeping the sport itself foot racing. |
| 2026-11-13 | Races anchor a sandbox of different money-making careers. | Let players participate in the racing world without requiring runner ownership or race entry. |
| 2026-11-13 | The game is free-to-play with community self-hosted dedicated-server software; multiplayer is a longer-term goal. | Make the world accessible and keep hosting open to communities. |
| 2026-11-14 | Licenses gate selected activities without improving player stats or requiring a racing qualifier. | Provide activity progression without performance advantages or extra entry barriers. |
| 2026-11-14 | Runner development, appraisal, and race effects are separated by system in Section 6. | Keep persistent runner data distinct from temporary race behavior and world services. |
| 2026-11-14 | Runner body distributions are informed by empirical sprinter data, without treating reference values as hard bounds. | Generate plausible variety rather than fixed body templates. |
| 2026-11-14 | Gym ownership, breeding, betting, bookmaking, and venue operation support player-chosen careers. | Allow specialized and mixed paths through the world economy. |
| 2026-11-14 | Race outcomes are precomputed from locked inputs and then played back independently of rendering. | Keep results consistent for entrants and spectators. |
| 2026-11-14 | Betting is pre-race; enforcement for steroids and race fixing varies by competition type. | Keep wagers based on pre-race choices and reserve licensed-competition enforcement for further design. |
| 2026-11-14 | Financial failure triggers a bottom-up restart while retaining the character. | Make failure consequential without ending the sandbox. |
| 2026-11-14 | Single-player AI may combine several player-like careers. | Keep the world active across its different activities. |
| 2026-11-25 | Career begins with no owned runners or stable lease; the player rents home-city space and buys prospects generated by the breeder. | Make player ownership an earned progression step while keeping the world population active. |
| 2026-12-01 | Public amateur city races and basic wagers are open; breeding requires its activity license. | Keep the first race accessible while making breeding a licensed progression activity. |
| 2026-12-01 | City-track distances, race prediction, training activities, and bookmaker games are seeded or deterministic where appropriate. | Make repeated play reproducible while avoiding saved appraisals or predictions. |

## 20. Glossary and references

### Glossary

| Term | Definition |
|---|---|
| [Term] | [Meaning in this project] |

### References

- Uth, N. (2005). “Anthropometric comparison of world-class sprinters and normal populations.” *Journal of Sports Science & Medicine, 4*(4), 608–616. [Article](https://pmc.ncbi.nlm.nih.gov/articles/PMC3899678/) · [PDF](https://www.jssm.org/vol4/n4/18/v4n4-18pdf.pdf). Reports height and body-mass distributions for 42 male and 44 female world-class 100 m sprinters; use the data to inform spawn-time bell-curve shapes, not as universal bounds.
- *Sigma Derby* — mechanical horse-racing arcade game; inspiration for compact race spectacle and group viewing.
- *Derby Day* — seaside mechanical horse-racing game; [YouTube video](https://www.youtube.com/watch?v=PgxNU8vuPuY).
- Coral bookmakers (UK), virtual horse racing — inspiration for virtual-race viewing in a bookmaker venue; exact product/title TBD.
- *Final Fantasy XIV: Chocobo Racing* — inspiration; specific aspects to adapt are TBD.
- [Easing functions demos](https://joshcollinsworth.com/demos/easing) — reference for curve shapes.

## Appendices

[Optional: detailed balance tables, formulas, sample dialogue, flow diagrams, wireframes, asset lists, or feature-specific specifications.]
