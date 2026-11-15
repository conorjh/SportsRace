# SportsRace — Game Design Document

> **Document status:** Draft
> **Owner:** TBD
> **Version:** 0.1
> **Last updated:** 2026-11-14
> **Repository / project:** SportsRace

> **How to use this template:** Replace bracketed text with project-specific information. Remove sections that do not apply, or mark them **Not applicable**. Clearly label ideas as **confirmed**, **proposed**, or **open** so the document distinguishes decisions from unresolved ideas.

## 1. Game summary

### High concept

SportsRace is a satirical sports-business sandbox built around stats-driven foot races presented through the spectacle and culture of British horse racing: odds, betting, bookies, and commentary. The competitors are foot racers, not horses.

### Elevator pitch

Players inhabit a continuously simulated racing world and choose how to make money: own, breed, develop, buy, sell, and race runners; bet on or fix races; or own businesses such as bookies, tracks, and other venues. Races are shared events at the center of the world: players can enter them, bet on any race, or watch any active race from a bookie. Single-player uses AI agents; a longer-term multiplayer version adds network clients to the same simulation engine.

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
2. Use the world and its buildings to act: enter a scheduled or open race, bet on a race, or watch an active race from a bookie.
3. Resolve every race type as a stats-driven simulation; race results determine placings, prizes, and betting outcomes.
4. Earn and spend money on runner purchases, development and care, race entries, performance-enhancing drugs, and businesses or venues. Larger leagues and championships pay more.
5. Purchase activity licenses at dedicated buildings to unlock restricted activities.

### Win, loss, and failure conditions

- **Success:** There is no formal win condition; players pursue self-directed goals through their chosen activities.
- **Failure:** Running out of money is the only failure condition.
- **Retry / continue:** The world continues; after running out of money, all of the player's owned assets are liquidated before they restart from the bottom. Licenses and progression reset; their character is the only thing retained and does not need to be remade. Exact liquidation mechanics and restart finances remain TBD.

## 5. Controls, camera, and interaction

### Control scheme

Players navigate and interact primarily by clicking. Both left-click and right-click are used; their exact actions by context remain to be specified.

### Camera and view

The world map uses a top-down 2D view. Race simulations are shown in a side-on 2D view.

### Interaction rules

Deferred. Contextual interactions, selection, confirmation, cancellation, and other shared rules will be specified later.

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
| Endurance | Begins at race time zero and runs concurrently with Acceleration. Generated Endurance ratings range from 20 to each runner's Potential (70–100). Before condition modifiers, ratings below 80 receive a duration-only bonus of `rating * (80 - rating) / 80` (a rating of 40 maps to a duration rating of 60 and lasts 40.5 seconds); ratings of 80 and above are unchanged (80 lasts 46.5 seconds), and a rating of zero keeps the 22.5-second baseline. At neutral conditions, reserve duration is 22.5 seconds plus 300 ms per duration rating point. Generated values therefore span approximately 33–52.5 seconds. Endurance determines reserve depletion duration, independent of track length; the curve shapes how reserve drains. The remaining reserve linearly scales acceleration-curve pace from 100% at full reserve to 20% at empty reserve. Gassing begins when reserve reaches 10%, starts from 28% of pre-endurance pace, and holds reserve at 10% for its duration. |

Acceleration uses a cubic Bézier curve with four control points, P0–P3. Endurance uses the same curve construction. P0 and P3 are fixed for each curve type; P1 and P2 are the randomized handles. Coordinates use normalized time (`x`) and normalized pace progress or reserve (`y`). Acceleration ascends from P0 `(0, 0)` to P3 `(1, 1)`; Endurance descends from P0 `(0, 1)` to P3 `(1, 0)`. Endurance reserve scales the acceleration-curve pace by `0.2 + 0.8 * reserve`: full reserve applies no reduction, and zero reserve leaves 20% of the pre-endurance pace. Gassing begins at 10% reserve, where pace is 28% of the pre-endurance pace, and holds reserve at 10% while active.

For rising curves, constrain the handles to `0 <= x1 <= x2 <= 1` and `0 <= y1 <= y2 <= 1`; for Endurance, use `0 <= x1 <= x2 <= 1` and `1 >= y1 >= y2 >= 0`. Draw x handles uniformly from `[0, 1]` and sort them ascending. Draw y handles uniformly and sort them for each curve's monotonic direction; for Acceleration, raise P1.y into `[0.4, 1]` with `y1 = 0.4 + 0.6 * y1`, then raise P2.y if needed to preserve `y1 <= y2`. Endurance y handles remain sorted descending. Generate P1/P2 once per runner and curve type from a deterministic runner-specific seed and reuse them across races. P1 primarily shapes the curve's early progress; P2 shapes its later progress. At normalized elapsed time `u`, find `t` such that `B_x(t) = u`, where `B(t) = (1 - t)^3 P0 + 3(1 - t)^2 t P1 + 3(1 - t) t^2 P2 + t^3 P3`; use `B_y(t)` to interpolate between the curve's pace or reserve endpoints. Randomized handles affect shape only, not endpoints or duration.

#### Race-only effects

These are temporary race effects; persistent runner conditions are listed under Runner attributes and conditions.

| State / event | Intended role |
|---|---|
| Confidence modifier | During a race, Confidence provides a small temporary bonus to all effective runner stats without changing base stats; exact bonus scaling remains TBD. |
| Stumble | Race-only event that temporarily reduces pace for a short, randomly determined duration. Its trigger rate is increased; higher Fatigue and lower Confidence increase its odds. Exact odds, pace loss, and duration remain TBD. |
| Second Wind | While Gassing, the runner rolls once per second for Second Wind. The base chance is 0.01%; Confidence adds up to 0.125 percentage points and Luck up to 0.25 percentage points, for a maximum of 0.385% per roll. On success, Gassing ends, pace is overridden to its pre-Gassing level for one 50 ms simulation step, and Endurance reserve is restored to 35–50% of its full amount. Luck adds up to 5 percentage points and Confidence up to 10 percentage points to the 35% base restoration. The “Second wind” label remains visible for one second. A later depletion can start another Gassing period and allow another Second Wind; there is no race-wide restoration cap. |
| Pulling up | A runner pulls up if a continuous Gassing period reaches its limit: 1 second when both Composure and Confidence are zero, up to 5 seconds when both are at their maximum. The limit interpolates linearly from the mean of normalized Composure (0–100) and Confidence (0–1). A Second Wind ends that Gassing period and its timer; a later Gassing period starts a new timer. A pull-up stops the runner, records a “Pulled-up” result without a numeric placing or placing prize, and does not stop the remaining runners. |
| Lost Belief (working name) | Race-only condition that lowers pace; it can occur only below 0.2 Confidence, with lower Confidence increasing its odds. |
| Believes in Self (working name) | Positive race-only trait that raises pace through a small stat boost; it can occur only above 0.8 Confidence, with higher Confidence increasing its odds. |

#### Items

Players can buy and give steroids to runners; certain steroids can raise persistent Confidence. Steroids are the current item example, not a complete item catalog, and more items can be added as they are designed. Steroid use is not detected in amateur or unlicensed races. Licensed leagues may randomly check before or after selected races and issue temporary bans; the catalog, other effects, check rules, and penalties remain open.

#### Match fixing

Players can fix races. Amateur and unlicensed races have no chance of detecting race fixing. Enforcement in licensed competition is deferred; methods, detection, and penalties remain to be designed.

#### Race flow and resolution

1. **Entry and lock:** Scheduled races close entry at their start. Open races start when the minimum entrant count is reached; that count remains TBD. Entrants, track, finish distance, and race seed lock at that point. Racer stats, conditions, and curve handles remain unchanged during the race and are read directly from each entrant.
2. **Runner timeline:** Initialize each runner at distance zero and evaluate the Acceleration and Endurance curves using the entrant's existing stats, conditions, and curves. Derive effective stats for the race without changing base stats. Use the race seed for event rolls; trigger timing and simultaneous-effect priority remain open.
3. **Movement and finish or pull-up:** At each event boundary, update active effects and calculate effective pace from the curve state and those effects. Integrate effective pace between boundaries to update distance; a runner finishes when its distance reaches the track's finish distance. A runner that remains Gassing beyond its Composure/Confidence-based limit pulls up and stops; other runners continue. Record distance, pace, active effects, and event times throughout the race.
4. **Race result:** Resolve the complete timeline before playback. Rank finishers by finish time and record pulled-up runners as “Pulled-up” without a numeric placing. Pass the results to the world simulation. Tie, disqualification, and other non-finish rules remain open.
5. **Playback:** Spectators read the same race timeline at a shared playback time; rendering cannot change the outcome. Identical entrant inputs and race seed produce identical timelines and results.

### World simulation

The world simulation maintains the region, runner roster, and buildings around the race simulation. It supplies race entrants and facilities, and applies race results to related systems such as development, rewards, betting, and progression.

#### World map

Each world is a randomly generated region containing tracks and buildings. The accelerated calendar runs while the server is online, including when no players are connected, and pauses during downtime. One in-game day per real hour is a candidate rate; the final conversion is deferred to prototyping. Each region includes at least one premium gym and multiple lower-quality gyms. Other map-generation rules and the full building list remain open.

#### Runner roster and ownership

Players can own, buy, sell, and enter runners. Race participation awards every finisher growth across physical stats, scaled by finishing position and capped by Potential; race-based growth is faster than gym-based growth. Exact growth rates, placement scaling, runner-market rules, and ownership details remain open for prototyping.

#### Training and recuperating (gym)

Gyms provide runner training and recovery. Players choose training focus through machines; standard machines target one stat, while higher-quality machines may affect multiple but never all stats. Rest and gym use recover Fatigue and Injury; gym recovery is faster and paid. Gym owners can buy machines and earn membership fees. Machine types, effects, purchase prices, membership fees, and recovery rates remain open.

#### Breeding

Breeding requires the breeding license. Offspring blend both parents' developable stats and hidden traits with random variation. Inheritance weights and further breeding rules remain open for prototyping.

#### Bookies and betting

Players can bet on any race and watch any active race from a bookie. Bets close before a race starts; in-running bets are not allowed. Operating a bookie requires the bookie-operator license. Bookie odds, the pre-race cutoff, payouts, operating costs, and other bookie features remain open.

#### Licenses

License buildings sell access licenses for licensed-league racing, breeding, bookie operation, and track operation. Licenses provide no player-stat bonuses. The racing license grants licensed-league access without a qualifier; basic betting, runner trading, amateur or unlicensed racing, and spectating remain available without licenses. Prices and other license rules remain open for prototyping.

#### Tracks

Track buildings host races and may be player-owned; operating a track requires the track-operator license. Race formats and entry timing are described under Race flow and resolution. Track layouts, schedules, ownership rules, and other track features remain open.

#### Stat appraisal

Appraisal provides deterministic, player-specific runner-stat estimates. Accuracy is fixed per player and strictly between 0% and 100%; a bounded bell-curve distribution is a candidate. Estimates use player identity and runner details, with accuracy controlling estimation error; perceived values are derived when inspected rather than saved. Distribution parameters and the error mapping are deferred to prototyping.

## 7. Progression and rewards

### Player progression

Players progress by pursuing chosen careers, acquiring activity licenses, and moving into higher-level competition. Activity-license rules and licensed-league access are specified in Section 6. License prices and league progression remain subject to prototyping.

### Content progression

League and championship structures and schedules remain TBD. See Section 6 for race formats and activity access.

### Rewards and economy

| Reward / resource | Earned from | Spent on | Limits / rules |
|---|---|---|---|
| Money | Race winnings; betting; breeding and runner sales; building income; illicit schemes. | Runner purchases, race entries, development and care, steroids, buildings, and operating costs. | Larger competitions pay more; exact values are prototype-tuned. |

### Save and persistence

The server world must resume from its previous state after downtime. Save format, backup, and recovery rules are TBD.

## 8. World, levels, and content

### Setting and world overview

The competition is foot racing presented through the spectacle and culture of British horse racing. World-map generation and simulation are specified in Section 6.

### Level / event structure

This section catalogs world content; gameplay rules for races and buildings are specified in Section 6.

| Content ID / name | Purpose | Key challenge | Unlock / prerequisites |
|---|---|---|---|
| Racetrack | Race venue; may be player-owned. | Track details remain open. | Track-operator license |
| Bookie | Bookmaking and race-viewing venue; may be player-owned. | Business details remain open. | Bookie-operator license |
| Gym | Runner training and recovery venue; may be player- or AI-owned. | Gym details remain open. | TBD |
| License building | Sells activity licenses. | See Section 6. | Payment |
| League / championship | Higher-level competition. | Series structure and schedules TBD. | Racing license; see Section 6 |

### Content variety and replayability

[Procedural generation, variations, randomization, branching, or repeat-play support.]

## 9. User interface and user experience

### Screen list

| Screen / view | Purpose | Entry point | Main actions |
|---|---|---|---|
| [Screen] | [Purpose] | [How the player gets here] | [Available actions] |

### Navigation flow

[Insert a screen-flow diagram or describe the main navigation paths.]

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
| [Asset] | [Where it appears] | [Must / Should / Could] | [Status] |

## 11. Audio and music

### Audio direction

[Overall sound identity and player-facing goals.]

### Music

[Themes, adaptive music, transitions, and silence requirements.]

### Sound effects and voice

Race commentary and brief interview segments carry the sporting satire. Interview patter should parody stock clichés, empty motivational answers, and earnest non-answers; voice and subtitle requirements remain TBD.

## 12. AI and difficulty

AI agents populate the single-player sandbox and may combine player-like careers rather than specialize. Their activity mix, priorities, and decision-making remain open.

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
| Engine / framework | [Technology] | [Owner / status] |
| Target hardware | [Minimum and recommended specs] | [Owner / status] |
| Performance targets | [Frame rate, load times, memory, etc.] | [Owner / status] |
| Data / save format | [High-level approach] | [Owner / status] |
| Race simulation | Implement the race-flow rules in Section 6. | Proposed; event schema and storage structure remain implementation details |
| Race rendering | Implement the playback rules in Section 6. | Proposed; exact playback speed TBD |
| AI and multiplayer | See Sections 12 and 13. | Proposed; network authority and replication TBD |
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
| Scheduled race precomputation | Admit entrants through the scheduled start and observe precomputation time. | Entries close and the timeline is generated at the scheduled start; the entrant list and race configuration are fixed |
| Open race precomputation | Add entrants until the minimum count is reached and observe when the race starts. | The race starts and its timeline is generated as soon as the minimum entrant count is reached; the entrant list and race configuration are fixed, with the minimum tuned through prototyping |
| Race timeline rendering | Sample each runner's events at arbitrary race times across multiple spectators. | Reproducible position and animation at the same shared playback time, without renderer-driven simulation |
| Live race information | Inspect data and playback position for spectators who join an in-progress precomputed race. | Join at the current live race time, synchronized with other spectators; no future events or final result before the finish |
| Betting cutoff | Attempt to place wagers before and after the race starts. | Pre-race bets are accepted; no wagers are accepted once the race has started |
| Runner-stat appraisal | Recompute estimates for identical and differing player/runner inputs, sample the accuracy distribution, and compare the same player's accuracy before and after progression. | Identical inputs are deterministic; each player's accuracy remains fixed through progression and strictly between 0% and 100%; distribution parameters are set through prototyping |
| Gameplay balance | Run end-to-end simulations across runner generation, races, development, world activities, economy, and AI behavior. | Identify cross-system balance issues; numerical targets and tolerances are set through prototyping |
| Runner body generation | Generate representative batches of runners at spawn. | Height and weight match the research-informed bell-curve distributions and preserve their empirical relationship; fitted parameters and tolerances set through prototyping |
| Runner recovery | Compare rest and gym recovery for race fatigue and injuries. | Both recover through either option; gym recovery is faster and paid, with rates and costs tuned through prototyping |
| Persistent runner conditions | Simulate repeated races, rest/gym recovery, wins and losses, and steroid use. | Fatigue, Injury, and Confidence persist between races; racing increases Fatigue; Injury can occur in any race with higher risk while fatigued; Confidence starts at 0.5 on the proposed 0-1 scale, rises with wins, falls with losses, and can be raised by certain steroids. Exact probabilities and changes are prototype-tuned |
| Runner growth through racing | Compare race-driven changes to all physical stats across every race type under matched starting stats, sufficient Potential headroom across all physical stats, and gym-training exposure. Vary finishing position at fixed Trainability, then vary Trainability at fixed finishing position; verify every finisher, including last place, receives growth, higher placements yield more, every physical stat receives equal gains up to its Potential ceiling, and mental and hidden stats gain nothing from racing. Include a non-racing control. Compare race and gym growth under matched conditions and verify race-based growth is faster. | Every race type gives every finisher finishing-position-scaled growth equally across all physical stats, subject to Potential ceilings; race-based growth is faster than gym-based growth, with exact growth rates, per-placement values, and Trainability scaling tuned through prototyping |
| Race-only effects and events | Simulate Gassing, Pulling up, Stumble, Second Wind, Lost Belief, and Believes in Self while varying Composure, Fatigue, Confidence, and Luck; measure effective stats, pace, trigger odds, durations, and Endurance restoration. | Verify the pull-up limit increases monotonically with Composure and Confidence and never exceeds 5 seconds, a pulled-up runner stops without a numeric placing, and other entrants continue; verify Stumble's increased trigger rate and odds increasing with Fatigue and decreasing with Confidence; Lost Belief only below 0.2 Confidence; Believes in Self only above 0.8; Second Wind's per-second chance does not exceed the documented 0.385% maximum; the “Second wind” label remains visible for one second; restoration reaches but never exceeds 50% per event; the one-step pace override; and no change to base stats from temporary bonuses |
| Gym world generation | Generate representative world maps. | Every world has at least one premium gym and multiple lower-quality gyms |
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
| Activity license pricing | Set license prices through prototyping. | TBD | TBD | Deferred to prototyping |
| World calendar and time | Set the real-to-world-time conversion; one in-game day per real hour remains a candidate. | TBD | TBD | Deferred to prototyping |
| Bankruptcy and recovery | Set restart finances and exact asset-liquidation mechanics. | TBD | TBD | Open |
| Runner-stat appraisal accuracy | Fit the candidate bounded bell-curve distribution and map accuracy to estimation error. | TBD | TBD | Deferred to prototyping |
| Runner growth | Tune race and gym growth rates, placement scaling, Trainability, and any age or body-detail effects. | TBD | TBD | Open |
| Training and recuperating | Determine machine types and effects, recovery rates, and gym purchase and membership economics. | TBD | TBD | Open |
| Breeding | Determine inheritance weights and the amount of random variation. | TBD | TBD | Open |
| Persistent runner conditions | Set Fatigue and Injury scales and probabilities, Confidence change amounts, and Composure's influence on Confidence changes and performance under pressure. | TBD | TBD | Open |
| Runner height/weight distributions | Fit the research-informed height and weight distributions and their empirical relationship. | TBD | TBD | Open |
| Pace and Endurance curves | Tune Acceleration's duration and Endurance depletion timing; retain the 10% Gassing threshold and reserve hold, and tune Gassing's decline profile. | TBD | TBD | Open |
| Betting and bookmaking economy | Set the pre-start betting cutoff, odds, payouts, operating costs, and income rules. | TBD | TBD | Open |
| Steroid rules | Determine the item catalog and effects, licensed-race check rates and timing, and ban durations. | TBD | TBD | Open |
| Race-fixing enforcement | Design detection and penalties in licensed competition. | TBD | TBD | Deferred to later design |
| Buildings and venue ownership | Define other building interactions, gym tiers and machines, and ownership costs and income. | TBD | TBD | Open |
| League and championship progression | Determine league and championship structures, schedules, and payout scaling. | TBD | TBD | Open |
| Event-driven race model | Define event-trigger timing, simultaneous-effect priority, and event schema; tune odds and effect magnitudes, decide any additional event types, and resolve tie, disqualification, and non-finish handling. | TBD | TBD | Open |
| Race input lock and schedule | Choose the minimum entrant count for open races. | TBD | TBD | Open |
| Race playback timing | Set the playback-speed mapping from race-relative time. | TBD | TBD | Open |
| AI and multiplayer integration | Set AI activity priorities and decision policies; determine network authority, replication, hosting, and session flow. | TBD | TBD | Open |

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
