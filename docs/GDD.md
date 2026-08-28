# RIFTWORKS — Game Design Document

> **Status:** living design document and project source of truth.
> 
> Before implementing any major feature, check this file. After implementation, update the checklist at the end and keep the document aligned with the actual game.

## 1. High concept

RIFTWORKS is a low-poly 3D survival engineering sandbox set in a world trapped in near-permanent darkness after a deep-earth event known provisionally as **The Breach**.

The player explores empty cities and extensive underground layers, scavenges real functional components, restores and builds power infrastructure, creates bases and machines, and eventually engineers bespoke solutions to hunt colossal creatures.

The game is designed around a solo-dev production philosophy: procedural generation, reusable modular kits, one dominant humanoid rig family, simple non-humanoid rigs, systemic gameplay instead of huge amounts of authored content, and assets that participate in multiple systems at once.

The core promise is:

> **If you can imagine a machine, the game should try very hard to let you build it.**

The core crafting philosophy is:

> **The game should not mainly ask “what recipe do you want to craft?” It should ask “what can you invent with what you found?”**

---

## 2. Player fantasy

The player begins weak, poorly equipped and dependent on a small flashlight. Progress is not mainly represented by RPG numbers. It is represented by **capability and infrastructure**.

The intended arc is:

- “I need batteries for my flashlight.”
- “I need enough power to light this room.”
- “I need a generator for this workshop.”
- “I need a vehicle to recover that industrial motor.”
- “I need a proper grid for my underground base.”
- “I need an elevator to move material from 250 metres down.”
- “I need a power plant to run the machine I built to hunt that Colossus.”

The player should feel that they are gradually forcing order, light and machinery into a hostile dark world.

---

## 3. Design pillars

### 3.1 Darkness
Darkness is a gameplay system, not a visual filter. Light determines visibility, safety, workability, stealth, threat attraction and navigation.

### 3.2 Engineering
The player builds systems from components with simple physical or logical behaviours.

### 3.3 Scavenging
The abandoned world is a giant catalogue of functional parts. A motor is not merely “+5 scrap”; ideally it is a motor the player can recover and reuse.

### 3.4 Depth
Roughly half of the game should happen underground. Vertical world progression reduces the need for an impossibly huge handcrafted surface.

### 3.5 Emergence
The most valuable features are those that create new possibilities rather than only adding more authored content.

### 3.6 Production efficiency
Every major design decision must be evaluated through a solo-dev lens. Reuse is intentional design, not an apology.

---

## 4. Perspective and controls

RIFTWORKS is a **first-person game**. This is a locked project decision, not a temporary prototype preference.

First-person is central to the intended experience because it strengthens:

- the physicality of scavenging and manipulating machinery;
- the scale and terror of Colossi;
- immersion in darkness, flashlight use and underground exploration;
- precision when wiring, assembling and operating player-built systems;
- the feeling that the player personally inhabits the base and infrastructure they create.

The player body may still exist invisibly for collision, shadows and systemic interactions, but normal gameplay presentation remains first-person. Core systems should be designed around this perspective rather than requiring a third-person camera.

The movement target is grounded, responsive and readable rather than simulation-heavy. Camera motion should remain restrained: subtle head movement is welcome, but excessive bob, sway or forced cinematic motion must not interfere with long play sessions.

Desktop gameplay should launch in **fullscreen** by default. UI and rendering must scale cleanly across common desktop aspect ratios and resolutions.
---

## 5. World premise

Humanity opened or discovered something deep below the surface. Whether it is a portal, alien structure, impossible geological layer, ancient magical infrastructure or some overlap of those ideas remains deliberately ambiguous.

After The Breach:

- large urban areas were abandoned;
- conventional infrastructure collapsed;
- the atmosphere/world entered near-permanent darkness;
- subterranean human infrastructure connected to impossible spaces;
- new materials and physical phenomena appeared;
- humanoid survivor groups began occupying the remains;
- automatons, golems and strange creatures emerged;
- colossal entities began moving through some regions.

The player is not the chosen one. They are an explorer, scavenger, builder, engineer and eventually a Colossus hunter.

---

## 6. Surface world

The surface is procedural, persistent and grounded enough to feel coherent. Regions should transition naturally rather than abruptly.

Primary surface content:

- empty small cities;
- suburbs;
- industrial districts;
- highways;
- gas stations;
- farms;
- warehouses;
- factories;
- workshops;
- hospitals and civic buildings;
- substations;
- rail infrastructure;
- quarries;
- mines;
- forests;
- military or research sites.

The target is not a handcrafted AAA metropolis. We use modular kits, procedural road graphs, district rules, props, lighting, weather and damage variation to multiply a compact asset library.

---

## 7. Underground world

The underground is effectively a second world and may account for about half the playtime.

### Depth 1 — Human infrastructure
- metro tunnels;
- sewers;
- maintenance corridors;
- basements;
- parking structures;
- bunkers;
- electrical infrastructure.

### Depth 2 — Industrial underworld
- mines;
- shafts;
- industrial tunnels;
- buried laboratories;
- storage complexes;
- rail systems.

### Depth 3 — Natural depths
- giant caverns;
- subterranean rivers;
- lakes;
- cliffs;
- chambers large enough to contain structures and giant creatures.

### Depth 4 — Breach zones
- impossible ruins;
- alien/magical architecture;
- ancient machinery;
- hybrid organic structures;
- exotic materials and physical phenomena.

### Depth 5+ — The Deep
High-end procedural exploration with increasingly hostile conditions and strange rules rather than only inflated enemy health.

---

## 8. Procedural generation philosophy

Procedural generation exists to reduce production cost while creating replayability.

Surface generation should reason about:

- terrain;
- roads;
- rail;
- settlement density;
- districts;
- vegetation;
- utility infrastructure;
- POIs.

Underground generation should use a graph of authored modules and connectors:

`shaft -> utility tunnel -> metro -> mine -> cave -> Breach chamber`

Transitions should be physically plausible. Repetition is hidden through layout, lighting, props, fog, damage states and contextual dressing.

---

## 9. Light and darkness system

This is one of the most important systems in the entire project and must receive disproportionate polish.

Lighting goals:

- flashlight quality must feel premium;
- shadows must be strong, readable and stable;
- light falloff should feel physically believable;
- surfaces need convincing specular response;
- volumetric scattering should make beams readable in fog/dust without turning the whole image into soup;
- exposure/tonemapping must preserve darkness without crushing all detail;
- lighting must create atmosphere while remaining gameplay-readable.

Progression examples:

- weak handheld flashlight;
- headlamp;
- rechargeable flashlight;
- flares;
- portable work lights;
- vehicle headlights;
- industrial floodlights;
- powered base lighting;
- Breach-based illumination.

Light is also risk. Bright infrastructure can reveal the player or increase a base’s detectable signature.

---

## 10. Power system

Electricity is the technical blood of the game.

The system tracks at minimum:

- generation;
- consumption;
- storage;
- connection;
- priority/status.

Progression:

### Early
- disposable batteries;
- rechargeable batteries;
- portable fuel generators.

### Mid
- industrial generators;
- larger battery banks;
- hydro/utility recovery where appropriate;
- hybrid grids.

### Late
- geothermal;
- restored infrastructure;
- Breach technology;
- Colossus-derived power components.

### High end
- large reactors;
- distributed grids;
- megawatt-scale installations;
- specialized power systems for giant machines.

Power should be visible and understandable. Machines show whether they are powered, starved or offline.

---

## 11. Power signature and blackout

Large installations create a stronger signature through light, noise, heat or Breach emissions.

This creates a natural progression pressure: a bigger base is more capable but harder to hide.

Players can engineer blackout behaviour using relays, sensors and priorities:

- shut off decorative lighting;
- preserve essential batteries;
- close doors;
- disable noisy machinery;
- keep defenses powered;
- switch to low-signature systems.

Ideally this is created from the same automation components used everywhere else rather than a separate magical “defend base” button.

---

## 12. Scavenging

Scavenging is driven by player intent.

The player should go into the world because they need specific things.

Examples:

- battery;
- alternator;
- generator;
- pump;
- motor;
- cable;
- compressor;
- industrial light;
- electronics;
- structural steel;
- fuel;
- tools.

Whenever practical, valuable components exist physically and can be carried, transported or installed.

If the player does not want a component, it may be dismantled into generic materials.

---

## 13. Salvage progression

### Tier 1
Small hand-carried components.

### Tier 2
Heavy components requiring better tools or inventory capacity.

### Tier 3
Equipment requiring carts, vehicles or winches.

### Tier 4
Industrial machinery requiring trucks, cranes or player-built recovery systems.

This converts increased capability into new loot opportunities.

---

## 14. Fabrication

There are two complementary forms of crafting.

### Fixed fabrication
Good for standardized consumables and components:
- ammunition;
- cables;
- bolts;
- structural pieces;
- medical items;
- replacement parts.

### System building
Used for machines. The player is not given a single recipe called “Automatic Mining Machine”. They assemble one from functional parts.

---

## 15. Functional Assembly System (FAS)

This is the core systemic feature.

### Structure
- beams;
- plates;
- chassis;
- foundations;
- platforms.

### Movement
- wheels;
- motors;
- rotors;
- pistons;
- hinges;
- sliders;
- axles.

### Energy
- batteries;
- generators;
- reactors;
- cables;
- switches.

### Logic/control
- buttons;
- levers;
- relays;
- timers;
- sequencers;
- basic logic gates.

### Sensors
- motion;
- proximity;
- weight;
- light;
- heat;
- pressure;
- hostile detection where justified.

### Work
- drills;
- saws;
- pumps;
- winches;
- conveyors;
- cranes;
- crushers.

### Combat
- turrets;
- harpoons;
- traps;
- flamethrowers;
- heavy weapons;
- specialized emitters.

### Breach components
Late-game parts introducing new physical properties.

---

## 16. FAS golden rule

A component should understand its own behaviour, not the final machine.

Examples:

- a motor consumes power and produces rotation;
- a piston produces linear force;
- a button emits a signal;
- a sensor emits a signal when its condition is met;
- a lamp consumes power and emits light;
- a battery stores energy.

This makes twenty reliable parts more valuable than hundreds of hardcoded machines.

---

## 17. Connections

Keep connection types limited and reusable:

- **Weld:** rigid attachment;
- **Hinge:** rotation;
- **Axle:** rotational transmission;
- **Slider:** constrained linear motion;
- **Power:** energy connection;
- **Signal:** logical connection;
- **Rope/Cable:** flexible physical connection where needed.

The system should be powerful but not require engineering-school knowledge.

---

## 18. Physics philosophy

The game needs enough physics for creativity, not a simulation of every screw.

Priorities:

- stable rigid bodies;
- predictable joints;
- clear mass differences;
- useful collision behaviour;
- recoverable failures;
- deterministic enough behaviour for blueprints.

Do not chase fully destructible voxel worlds or expensive structural destruction.

---

## 19. Blueprints

Player constructions can eventually be saved as reusable blueprints.

Potential functionality:

- save a construction;
- name it;
- rebuild when materials are available;
- improve versions;
- share externally or through Steam Workshop later.

Blueprint sharing is a major long-term virality opportunity.

---

## 20. Player-created chaos

The game should support creations that are:

- useful;
- efficient;
- elegant;
- silly;
- catastrophically bad;
- unexpectedly brilliant.

Ideal social content:

- automated defenses;
- underground elevators;
- bizarre vehicles;
- monster traps;
- mining systems;
- ridiculous launchers;
- Rube Goldberg contraptions;
- unconventional Colossus kills.

The developer supplies consistent rules. Players supply the stories.

---

## 21. Base building

The player can build from scratch but is encouraged to appropriate existing structures.

Potential bases:

- house;
- supermarket;
- warehouse;
- factory;
- bunker;
- metro station;
- mine;
- cavern;
- utility facility.

Existing architecture reduces production and building-system complexity while giving players meaningful choices.

---

## 22. Outposts and network growth

A single base eventually becomes insufficient.

Players create:

- caches;
- power substations;
- mines;
- underground stations;
- safe rooms;
- light towers;
- transport hubs;
- hunting posts.

Over time the world becomes visibly marked by the player’s infrastructure.

---

## 23. Logistics

Logistics is a major progression axis.

Early:
- player carries everything.

Mid:
- carts and vehicles move heavy loads.

Late:
- lifts, conveyors, rail, automated transport and specialized vehicles move resources.

High-end:
- distributed infrastructure moves industrial quantities across depth layers.

A rare resource 400 metres underground is only valuable if the player can solve the logistics problem.

---

## 24. Automation

Automation should emerge from general-purpose components.

Example:

`motion sensor -> relay -> floodlights`

Or:

`vehicle detector -> door motor -> timer -> lights`

Or:

`hostile sensor -> siren -> turret -> emergency cutoff`

The same parts support production, comfort, defense and jokes.

---

## 25. Vehicles

Vehicles support exploration, salvage and construction.

Progression may include:

- bicycle/cart;
- car;
- pickup;
- van;
- truck;
- custom engineering platforms.

Vehicles may host:

- storage;
- batteries;
- generators;
- work lights;
- winches;
- armor;
- cranes;
- weapons.

Long term, custom vehicles should use the same FAS principles instead of a wholly separate system.

---

## 26. Combat

Combat is important but not the sole identity.

Baseline arsenal may include:

- pistols;
- rifles;
- shotguns;
- melee;
- explosives;
- improvised weapons.

Combat exists to make exploration dangerous and engineering meaningful. It should not become a stat-heavy looter shooter.

---

## 27. Humanoid enemies

Most common intelligent enemies should be humanoid for production efficiency.

Possible groups:

- scavengers;
- raiders;
- mercenaries;
- cultists;
- hostile survivors;
- soldiers;
- Breach-touched humanoids.

Reuse one main humanoid skeleton and retargetable animation set. Differentiate through equipment, clothing, weapons and behaviour.

---

## 28. Non-humanoid enemies

Allowed when production-efficient.

Good candidates:

- drones;
- rigid-body automatons;
- golems;
- small crawling creatures;
- simple parasites;
- IK-driven creatures;
- slow large entities.

Avoid creatures that need enormous bespoke animation sets unless their gameplay value is exceptional.

---

## 29. Colossi

Colossi are rare giant entities and one of the game’s visual signatures.

They should be visible or audible from far away. Early-game players may see one walking through an empty city and correctly decide to avoid it.

Colossi are engineering challenges, not giant health bars.

Players may use:

- traps;
- harpoons;
- vehicles;
- terrain;
- explosives;
- heavy weapons;
- automated defenses;
- completely unexpected contraptions.

---

## 30. Colossus harvesting

Colossi unlock new engineering capabilities rather than simply granting XP.

Example harvestables:

- bioelectric core;
- luminous eye;
- gravity organ;
- tendon material;
- carapace;
- thermal gland;
- resonance organ.

Boss progression becomes:

`discover -> study -> engineer solution -> hunt -> harvest -> gain new physical capability -> invent`

---

## 31. Magic as alternate physics

Magic should mostly enter the engineering sandbox as materials/components with understandable properties.

### Repulsion
Generates directional force.

### Attraction
Pulls selected matter.

### Gravity
Changes effective weight or force direction.

### Luminance
Produces powerful or special-spectrum light.

### Thermal
Produces heat.

### Cryo
Removes heat.

### Phase
Rare high-end material affecting solidity/density/interaction.

These parts are the fantasy equivalent of advanced engineering components and should create combinatorial possibilities.

---

## 32. Progression philosophy

Primary progression is capability, not percentages.

Prefer:

- “I can now lift this machine.”
- “I can now power this site.”
- “I can now descend safely.”
- “I can now automate this route.”
- “I can now hunt that creature.”

Over:

- +3% damage;
- +5% stamina;
- arbitrary color-tier inflation.

Small RPG progression may exist, but infrastructure and equipment remain dominant.

---

## 33. Survival systems

Keep traditional survival friction restrained.

Potential systems:

- health;
- stamina;
- limited temperature hazards;
- simplified food/medical needs.

Do not make the game primarily about repeatedly filling hunger and thirst bars. Light, energy, preparation and logistics are more important.

---

## 34. Death

Death should matter without destroying engineering work.

Base concept:

- respawn at a claimed base/outpost;
- carried gear remains near death location;
- constructed infrastructure persists;
- retrieval creates a natural recovery expedition.

Hardcore options can come later.

---

## 35. Sound

Sound is unusually important because darkness reduces visual certainty.

The player should hear:

- distant footsteps;
- generators;
- electrical hum;
- rain;
- metal movement;
- gunfire;
- underground echoes;
- a Colossus long before clearly seeing it.

Sound can make a low-poly world feel far larger and more alive than the asset count suggests.

---

## 36. Weather and atmosphere

Weather provides visual variation cheaply:

- rain;
- fog;
- dust;
- wind;
- storms.

It also alters visibility, sound and light readability.

---

## 37. Factions

Factions are desirable but not a first milestone.

Architecture should eventually support:

- traders;
- neutral survivors;
- hostile groups;
- reputation;
- territories;
- settlements;
- contracts.

Do not build a giant political simulation before the core sandbox works.

---

## 38. Economy

Economy may eventually support:

- buying;
- selling;
- barter;
- component orders;
- hiring;
- repair services.

Exploration must remain relevant. Money should not trivially replace scavenging.

---

## 39. Core gameplay loop

1. **Need** — the player wants a capability or component.
2. **Explore** — locate an appropriate POI or depth layer.
3. **Secure** — establish visibility and survive threats.
4. **Scavenge** — recover specific useful parts/materials.
5. **Transport** — solve weight, vehicle or logistics problems.
6. **Build** — construct or upgrade a system.
7. **Capability** — the new machine/infrastructure enables something previously impossible.
8. **Discover** — new regions, depths or threats become viable.
9. **Challenge** — solve a new engineering/combat/logistics problem.
10. **Invent** — create a solution.
11. Repeat.

---

## 40. Minute-to-minute loop

- navigate darkness;
- read the environment;
- manage flashlight/light position;
- listen;
- move;
- inspect objects;
- loot/scavenge;
- fight or avoid;
- manipulate machines;
- improvise.

---

## 41. 10–30 minute loop

Example:

The player needs an industrial motor, locates a factory, travels there, establishes temporary lighting, deals with enemies, recovers the motor and transports it home.

---

## 42. 1–3 hour loop

The recovered component unlocks a meaningful machine or infrastructure improvement, which opens a new depth/POI/logistics challenge and creates the next self-directed objective.

---

## 43. First two hours

Target experience:

- begin with poor lighting;
- discover a viable shelter/base;
- encounter a broken power situation;
- scavenge the part/fuel needed;
- restore first generator;
- physically light the base;
- discover an underground entrance;
- hear/see first evidence of something enormous.

The first major emotional reward is **turning darkness into owned, powered space**.

---

## 44. Hours 2–10

- better flashlight/tools;
- first vehicle or heavy-transport solution;
- real component scavenging;
- first automation;
- early underground infrastructure;
- humanoid threats;
- first full Colossus sighting;
- player still avoids serious Colossus combat.

---

## 45. Hours 10–40

- expanding main base;
- stable grid;
- multiple machine types;
- deeper exploration;
- first Breach components;
- outposts;
- more advanced logistics;
- first engineered encounters with large creatures.

---

## 46. Hours 40–100

- multi-site infrastructure;
- specialized vehicles;
- automated production/logistics;
- deliberate Colossus hunting;
- deep exploration;
- complex saved blueprints;
- major energy projects.

---

## 47. 100+ hour sandbox

The game stops prescribing goals and supports player-generated ambitions:

- electrify a district;
- create a city-scale base;
- build an underground rail network;
- automate a mine;
- create ridiculous vehicles;
- optimize power consumption;
- hunt Colossi in unusual ways;
- build factories;
- create elaborate traps;
- explore deeper procedural layers.

---

## 48. High-end pillar: deeper expeditions

Deep regions add difficulty through new conditions:

- severe darkness;
- electrical interference;
- vertical traversal;
- energy constraints;
- unusual gravity;
- hostile architecture;
- logistics distance;
- new enemy behaviours.

Avoid relying only on inflated health/damage values.

---

## 49. High-end pillar: Colossus hunting

Different Colossi demand different preparation and reward different physical capabilities.

The player can develop dedicated hunting rigs and infrastructure.

---

## 50. High-end pillar: megaprojects

Megaprojects are open engineering problems, not rigid recipes.

Examples:

- establish permanent power at extreme depth;
- move 20 tonnes of salvage to the surface;
- create a bridge over a giant cavern;
- construct a Colossus trap;
- build a deep elevator network;
- restore a major substation;
- power an enormous experimental device.

---

## 51. High-end pillar: optimization

Once survival is solved, players create their own game through optimization:

- output/minute;
- lower power use;
- smaller machine footprint;
- faster transport;
- safer automation;
- cleaner layouts;
- better Colossus kill methods.

This is one of the mechanisms that gives sandbox games very long lifetimes.

---

## 52. Infinite play engine

The game is not literally infinite. It should create enough recombination that the player repeatedly asks:

> “What if I try this?”

Long-term engines:

- procedural world;
- procedural underground;
- engineering combinations;
- new physical properties;
- blueprints;
- Colossi;
- infrastructure networks;
- optimization;
- self-imposed projects;
- factions/economy later.

---

## 53. Post-narrative world

A narrative endpoint may exist, but it must not end the sandbox.

After narrative completion:

- deeper zones remain explorable;
- Colossi persist/migrate;
- new Breach layouts can appear;
- player megaprojects remain meaningful;
- economy/faction contracts can continue later.

---

## 54. Low-poly art direction

Low-poly is chosen as a production strategy and an aesthetic.

Targets:

- strong silhouettes;
- clean geometric forms;
- controlled materials;
- restrained texture complexity;
- believable PBR response where useful;
- dramatic high-quality lighting;
- fog and emissive accents;
- readable equipment;
- visually coherent modular environment kits.

The game should never look like “random asset-store low poly”. A consistent material, scale, silhouette and lighting language must unify everything.

---

## 55. Lighting quality bar

The flashlight and darkness experience are signature features.

Target techniques for high PC quality:

- Forward+ renderer;
- high-resolution positional shadow atlas;
- soft shadow filtering;
- physically believable inverse-square flashlight attenuation;
- non-zero source size for penumbra;
- projector/cookie texture for organic beam shape;
- volumetric fog interaction;
- filmic/AgX or ACES-like tonemapping;
- SSAO;
- SSIL and/or suitable GI where performance allows;
- material roughness/specular tuning;
- subtle beam sway and response rather than static game-light feeling.

Performance tiers can later disable expensive options, but the visual north star is a genuinely premium flashlight.

---

## 56. Production multipliers

Every asset should ideally do more than one job.

Example: industrial generator

- environment prop;
- loot target;
- functional power device;
- transport challenge;
- base equipment;
- economic item;
- quest/contract target later.

Prioritize systems that multiply assets rather than demand more assets.

---

## 57. Humanoid production pipeline

- one main reusable humanoid skeleton;
- retargetable animation library;
- modular clothing and equipment;
- weapons/accessories as attachments;
- AI behaviour differentiation over bespoke animation differentiation.

---

## 58. Creature production pipeline

Prefer:

- few bones;
- rigid articulation;
- procedural IK;
- slow readable movement;
- reusable motion logic;
- oversized creatures where spectacle justifies bespoke work.

Do not create dozens of animation-heavy bespoke monsters.

---

## 59. Colossus production strategy

A smaller roster of memorable giants is preferable to a huge bestiary.

A Colossus should earn its production cost by contributing:

- world spectacle;
- navigation tension;
- high-end combat;
- unique salvage;
- engineering progression;
- marketing/GIF value.

---

## 60. Persistence

World uses deterministic seeds where possible.

Persist only meaningful deltas:

- removed salvage;
- constructed machines;
- base claims;
- power state when necessary;
- placed objects;
- important enemy/Colossus state;
- loot containers;
- unlocked blueprints.

Avoid serializing the untouched procedural world.

---

## 61. Multiplayer

Single-player first.

Potential 1–4 player co-op later if architecture and market justify it.

Do not let networking multiply prototype complexity before the core engineering sandbox is fun.

---

## 62. MVP proof points

The first serious vertical slice must prove:

1. darkness feels excellent;
2. the flashlight feels premium;
3. scavenging a real component feels valuable;
4. power connections are understandable and satisfying;
5. the player can combine simple engineering parts into something functional;
6. surface-to-underground exploration is compelling;
7. seeing a Colossus creates immediate awe/tension.

If these are not fun, adding content is not the solution.

---

## 63. Vertical slice content target

- one coherent procedural surface district;
- small city;
- industrial facility;
- forest/rural transition;
- metro/subterranean network;
- one deep chamber;
- one viable base location;
- humanoid enemy family;
- one simple non-humanoid enemy;
- one automaton/golem;
- one functional Colossus;
- generator;
- battery;
- cable/power link;
- multiple lights;
- motor;
- wheel;
- hinge;
- button;
- sensor;
- small functional build system;
- persistence.

---

## 64. Systems explicitly out of scope for now

- voxel world;
- full structural destruction;
- realistic simulation of every mechanical part;
- huge authored quest campaign;
- hundreds of enemies;
- AAA-scale metropolis;
- giant branching dialogue RPG;
- MMORPG simulation;
- complex faction politics before sandbox core;
- mandatory multiplayer.

---

## 65. Solo-dev decision rule

For every proposed feature ask:

1. Does it create new gameplay possibilities or only new content?
2. Can existing assets participate in it?
3. Does it require bespoke animation/art at scale?
4. Can it be procedural/systemic?
5. Does it make player stories more likely?
6. Is there a cheaper design that preserves the fantasy?

Prefer a new universal joint over twenty bespoke machines. Prefer a new systemic property over ten similar rifles.

---

## 66. Virality by design

The game should naturally create shareable moments rather than trying to force memes.

Desired player posts:

- “Look what I built.”
- “This giant walked past my base.”
- “My defense system went terribly wrong.”
- “I made a working freight elevator.”
- “I killed a Colossus with a completely stupid contraption.”
- “I found this 500 metres underground.”

---

## 67. Emotional rhythm

The intended rhythm is:

**fear / curiosity -> expedition -> discovery -> recovery -> construction -> mastery -> new ambition**

Darkness creates vulnerability. Engineering creates ownership and control. The Deep then takes some of that control away and demands a new level of preparation.

---

## 68. The real progression currency

The true currency of RIFTWORKS is **capability**.

The player’s world should visually record that capability:

- illuminated roads;
- repaired rooms;
- cables;
- generators;
- lifts;
- machines;
- outposts;
- vehicles;
- harvested giant parts;
- automated systems.

---

## 69. One-sentence pitch

**A low-poly survival engineering sandbox set in a world trapped in darkness, where players scavenge abandoned cities and impossible underground depths to build machines, infrastructure and weapons capable of confronting colossal creatures.**

---

# DEVELOPMENT CHECKLIST

This checklist is part of the GDD and must be updated as implementation changes. A checked box means there is at least a working prototype implementation in the repository, not that the feature is production-final.

## Project foundation
- [x] Godot project initialized.
- [x] Main playable scene exists.
- [x] Basic third-person placeholder movement/camera.
- [ ] Production character controller.
- [ ] Input rebinding/settings UI.
- [ ] Save/load architecture.
- [ ] Seeded world persistence architecture.

## Visual / darkness
- [x] Permanent-night prototype environment.
- [x] Basic fog/night ambience.
- [x] Basic flashlight with battery drain.
- [x] Basic deployable floodlight.
- [ ] Forward+ high-quality lighting pass.
- [ ] Premium flashlight beam with projector/cookie.
- [ ] Volumetric flashlight scattering.
- [ ] High-quality soft dynamic shadows.
- [ ] SSAO/SSIL/GI quality preset.
- [ ] Exposure/tonemapping tuned for readable darkness.
- [ ] Weather lighting variation.
- [ ] Multiple functional light classes.

## World generation
- [x] Procedural-ish city blockout prototype.
- [x] Empty urban atmosphere prototype.
- [x] First underground descent/blockout.
- [x] First deep chamber/blockout.
- [ ] Seeded surface chunk generator.
- [ ] Road graph.
- [ ] District/POI rules.
- [ ] Natural biome transitions.
- [ ] Underground graph generator.
- [ ] Metro modules.
- [ ] Mine modules.
- [ ] Cave modules.
- [ ] Breach modules.
- [ ] Streaming/chunk budgets.

## Scavenging / inventory
- [x] Temporary abstract scavenge interaction.
- [x] Basic scrap resource.
- [ ] Physical salvage props.
- [ ] Interaction raycast/highlight.
- [ ] Inventory with weight/volume rules.
- [ ] Carry heavy component interaction.
- [ ] Dismantling.
- [ ] Salvage tiers/tools.
- [ ] Transportable industrial components.

## Power grid
- [x] Prototype power readout concept.
- [ ] Functional generator component.
- [ ] Functional battery component.
- [ ] Functional power consumer.
- [ ] Power links/cables.
- [ ] Grid generation/consumption calculation.
- [ ] Battery charge/discharge.
- [ ] Load priority.
- [ ] Device powered/unpowered feedback.
- [ ] Power signature.
- [ ] Player-built blackout automation.

## Functional Assembly System
- [ ] Placement/snap system.
- [ ] Structural platform.
- [ ] Weld joint.
- [ ] Hinge.
- [ ] Wheel.
- [ ] Motor.
- [ ] Button/switch.
- [ ] Signal link.
- [ ] Sensor.
- [ ] Slider/piston.
- [ ] Rope/cable physical connection.
- [ ] Blueprint serialization.
- [ ] Blueprint reconstruction.

## Bases / logistics
- [ ] Claim existing structure as base.
- [ ] Base inventory.
- [ ] Outposts.
- [ ] Heavy salvage logistics.
- [ ] Winch.
- [ ] Lift/elevator.
- [ ] Rail/cart transport.
- [ ] Conveyor/logistics system.

## Combat / AI
- [ ] Basic weapon framework.
- [ ] Reusable humanoid enemy controller.
- [ ] Humanoid perception of light/sound.
- [ ] Humanoid ranged combat.
- [ ] Simple drone/automaton.
- [ ] Simple non-humanoid creature.
- [ ] Loot/harvesting from enemies.

## Colossi
- [x] Colossus-scale visual placeholder.
- [ ] Functional roaming Colossus AI.
- [ ] Distant audio/footstep presence.
- [ ] Vulnerable zones.
- [ ] Environmental interaction.
- [ ] Engineered hunting support.
- [ ] Harvestable Colossus component.
- [ ] First component unlocks a new engineering property.

## Breach / fantasy engineering
- [ ] First Breach zone.
- [ ] Repulsion component.
- [ ] Attraction component.
- [ ] Luminance component.
- [ ] Thermal/Cryo component.
- [ ] Gravity component.
- [ ] High-end Phase concept.

## UI / feedback
- [x] Prototype HUD.
- [x] Battery display.
- [x] Scrap display.
- [x] Prototype grid-power display.
- [ ] Interaction prompt.
- [ ] Power-device status UI.
- [ ] Inventory UI.
- [ ] Build mode UI.
- [ ] Blueprint UI.
- [ ] Base/grid overview.

## Audio
- [ ] Footsteps by surface.
- [ ] Flashlight/electrical interactions.
- [ ] Generator loops.
- [ ] Underground reverb/ambience.
- [ ] Distant Colossus audio.
- [ ] Dynamic darkness ambience.

## Milestone gate
- [ ] The flashlight alone feels good enough to market in a GIF/video.
- [ ] Generator -> power link -> battery/light works end-to-end.
- [ ] Player scavenges a physical motor from a POI and installs it.
- [ ] Player builds at least one useful machine from generic components.
- [ ] Procedural surface -> underground expedition works end-to-end.
- [ ] First humanoid encounter works with reusable rig architecture.
- [ ] First Colossus can be seen, avoided, fought through preparation and harvested.

---

## Maintenance rule

**Always read/check this GDD before a substantial RIFTWORKS implementation session. Always update the checklist after the session. If the game diverges from the GDD because testing proves something better, update the GDD rather than silently letting design and code disagree.**

---

## Current implementation snapshot

- First-person perspective is now a locked project decision; the player camera uses eye-height positioning, FPS FOV, subtle movement bob and an invisible shadow-only body to avoid self-clipping.
- Desktop presentation defaults to true fullscreen at a 1920x1080 reference viewport and explicitly requests fullscreen at runtime outside headless CI.
- The flashlight is mounted slightly off the camera axis so its beam reads as a held physical light rather than a perfectly centered camera spotlight.
- Forward+ premium darkness stack remains active: dynamic flashlight shadows, projector cookie, volumetric fog interaction, AgX tonemapping, SSAO, SSIL and SDFGI.
- Procedural surface streaming, procedural underground graph, physical salvage, power grid, logic links, FAS assembly, blueprints, Breach repulsion, humanoid threats, drones, Colossus weakpoints/harvest, base claiming and save/load are present in prototype form.
- Godot CI strictly compiles every GDScript and boots the main scene on Godot 4.6.3 before changes are considered safe.
