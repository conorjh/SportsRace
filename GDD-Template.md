# [Game Title] — Game Design Document

> **Document status:** [Draft / In Review / Approved]
> **Owner:** [Name or team]
> **Version:** [0.1]
> **Last updated:** [YYYY-MM-DD]
> **Repository / project:** [Link or path]

> **How to use this template:** Replace bracketed text with project-specific information. Remove sections that do not apply, or mark them **Not applicable**. Clearly label ideas that are proposed, unconfirmed, or implemented so the document does not mix current behavior with future plans.

## 1. Game summary

### High concept

[Describe the game in one or two sentences.]

### Elevator pitch

[Explain what the player does, what makes it appealing, and what makes it distinct.]

| Item | Description |
|---|---|
| Genre | [Primary genre and subgenre] |
| Platform(s) | [PC, console, mobile, web, etc.] |
| Target audience | [Intended players and age range, if relevant] |
| Player count | [Single-player, local multiplayer, online multiplayer] |
| Business / release model | [Premium, free-to-play, episodic, prototype, etc.] |
| Target session length | [Typical play session] |
| Target total playtime | [If applicable] |

## 2. Vision and goals

### Vision statement

[What should the game be known for?]

### Design pillars

1. **[Pillar name]** — [Principle and how it guides design decisions.]
2. **[Pillar name]** — [Principle and how it guides design decisions.]
3. **[Pillar name]** — [Principle and how it guides design decisions.]

### Goals

- [Player experience or product goal.]
- [Player experience or product goal.]

### Non-goals

- [What this game intentionally will not try to do.]
- [Features or experiences outside the intended scope.]

## 3. Player experience

### Player fantasy

[Who does the player get to be, and what do they get to do?]

### Intended emotions

[What should players feel during key moments?]

### Audience assumptions

[Prior genre knowledge, expected play style, accessibility needs, and other assumptions.]

## 4. Game structure and loops

### Game modes

| Mode | Description | Availability / unlock condition |
|---|---|---|
| [Mode] | [What the player does] | [When it is available] |

### Core gameplay loop

1. [Player goal or choice]
2. [Main action]
3. [Challenge or consequence]
4. [Reward, feedback, or result]
5. [What motivates the next loop]

### Meta-game loop

[Describe long-term play across sessions: progression, collection, competition, narrative, or other goals.]

### Session flow

[Describe launch, menus, play, results, and return-to-game flow.]

### Win, loss, and failure conditions

- **Success:** [What counts as success?]
- **Failure:** [What happens when the player fails?]
- **Retry / continue:** [Available recovery paths, if any.]

## 5. Controls, camera, and interaction

### Control scheme

| Action | Input | Context / notes |
|---|---|---|
| [Action] | [Button, key, gesture] | [When and how it works] |

### Camera and view

[Camera perspective, movement, framing, and player control.]

### Interaction rules

[Targeting, selection, confirmation, cancellation, pause, and other shared interaction rules.]

## 6. Gameplay systems and mechanics

### System overview

| System | Player purpose | Status | Dependencies |
|---|---|---|---|
| [System] | [Why it exists] | [Implemented / Planned / Proposed] | [Other systems] |

### Feature specification template

Copy this subsection for each major mechanic or feature.

#### [Feature name]

- **Purpose:** [Player need or design goal]
- **Status:** [Implemented / Planned / Proposed]
- **Player flow:** [Steps from entry to completion]
- **Inputs:** [Player actions and system triggers]
- **Rules:** [Conditions, calculations, limits, and exceptions]
- **Outputs and rewards:** [State changes, rewards, unlocks, or results]
- **Feedback:** [Visual, audio, haptic, or text response]
- **Failure and edge cases:** [Invalid actions, interruption, failure, recovery]
- **Dependencies:** [Required systems, assets, services, or content]
- **Acceptance criteria:** [Observable conditions that define completion]

## 7. Progression and rewards

### Player progression

[Levels, skills, attributes, upgrades, unlocks, or other growth systems.]

### Content progression

[How stages, regions, missions, events, or modes become available.]

### Rewards and economy

| Reward / resource | Earned from | Spent on | Limits / rules |
|---|---|---|---|
| [Resource] | [Source] | [Use] | [Cap, expiry, conversion, etc.] |

### Save and persistence

[What is saved, when it is saved, how it is loaded, and what happens after a reset or failure.]

## 8. World, levels, and content

### Setting and world overview

[Time, place, tone, rules of the world, and important locations.]

### Level / event structure

[How play spaces, missions, events, or encounters are organized.]

| Content ID / name | Purpose | Key challenge | Unlock / prerequisites |
|---|---|---|---|
| [Content] | [Role] | [Challenge] | [Requirement] |

### Content variety and replayability

[Procedural generation, variations, randomization, branching, or repeat-play support.]

## 9. Story, characters, and dialogue

[Optional. Remove this section for games without narrative content.]

### Narrative summary

[Premise, central conflict, and intended story arc.]

### Characters

| Character | Role | Motivation | Relationship to player |
|---|---|---|---|
| [Character] | [Role] | [Motivation] | [Relationship] |

### Narrative delivery

[Cutscenes, dialogue, environmental storytelling, text, or other delivery methods.]

## 10. User interface and user experience

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

## 11. Art direction and animation

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

## 12. Audio and music

### Audio direction

[Overall sound identity and player-facing goals.]

### Music

[Themes, adaptive music, transitions, and silence requirements.]

### Sound effects and voice

[Important sound cues, feedback sounds, voice, and subtitle requirements.]

## 13. AI and difficulty

[Optional. Describe opponents, NPCs, procedural decision-making, and difficulty systems.]

- **AI goals:** [What should the AI make the player experience?]
- **Difficulty model:** [Fixed, selectable, adaptive, or other]
- **Fairness/readability rules:** [How the player understands AI behavior]
- **Tuning parameters:** [Key values and who owns them]

## 14. Multiplayer and online features

[Optional. Remove if the game is offline single-player.]

- **Player count and roles:** [Who participates and what they can do]
- **Session flow:** [Invites, matchmaking, joining, leaving]
- **Networking model:** [High-level technical/design assumptions]
- **Competitive rules:** [Teams, scoring, ranking, rematch]
- **Disconnect and abuse handling:** [Expected behavior]

## 15. Monetization and live operations

[Optional. Include only if applicable.]

- **Monetization model:** [What is sold, if anything]
- **Earnable versus paid content:** [How access is divided]
- **Time-limited content:** [Events, seasons, rotations]
- **Player protections and transparency:** [Costs, odds, limits, and disclosures]

## 16. Accessibility and localization

### Accessibility

[Text scaling, contrast, colorblind support, remapping, subtitles, difficulty options, and other needs.]

### Localization

[Supported languages, text expansion, fonts, voiceover, cultural review, and regional requirements.]

## 17. Technical and production constraints

### Technical assumptions

| Area | Requirement / decision | Owner / status |
|---|---|---|
| Engine / framework | [Technology] | [Owner / status] |
| Target hardware | [Minimum and recommended specs] | [Owner / status] |
| Performance targets | [Frame rate, load times, memory, etc.] | [Owner / status] |
| Data / save format | [High-level approach] | [Owner / status] |
| Third-party services | [Dependencies and constraints] | [Owner / status] |

### Content pipeline and tools

[How content is authored, imported, reviewed, and delivered.]

### Risks and mitigations

| Risk | Likelihood / impact | Mitigation | Owner |
|---|---|---|---|
| [Risk] | [Low/Medium/High] | [Response] | [Owner] |

## 18. Testing, analytics, and success criteria

### Quality and playtesting goals

[What must be tested: usability, balance, performance, accessibility, compatibility, and so on.]

### Success criteria

| Goal | Measurement | Target / decision threshold |
|---|---|---|
| [Goal] | [How it is measured] | [Target] |

### Analytics and privacy

[Events to measure, purpose, consent, retention, and data-minimization requirements.]

## 19. Scope and roadmap

### Scope definition

- **Must have:** [Required for the first release / milestone]
- **Should have:** [Important but not release-blocking]
- **Could have:** [Optional if time allows]
- **Out of scope:** [Explicitly excluded]

### Milestones

| Milestone | Deliverables | Dependencies | Target date / status |
|---|---|---|---|
| [Milestone] | [Deliverables] | [Dependencies] | [Date / status] |

## 20. Open questions and decisions

| Question / decision | Options | Owner | Due date | Status |
|---|---|---|---|---|
| [Question] | [Options] | [Owner] | [Date] | [Open / Decided] |

### Decision log

| Date | Decision | Rationale | Owner |
|---|---|---|---|
| [YYYY-MM-DD] | [Decision] | [Why] | [Owner] |

## 21. Glossary and references

### Glossary

| Term | Definition |
|---|---|
| [Term] | [Meaning in this project] |

### References

- [Link, document, game, visual reference, or research source]

## Appendices

[Optional: detailed balance tables, formulas, sample dialogue, flow diagrams, wireframes, asset lists, or feature-specific specifications.]
