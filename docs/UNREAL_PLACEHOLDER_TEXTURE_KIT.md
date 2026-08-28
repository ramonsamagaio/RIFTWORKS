# RIFTWORKS — Placeholder Texture / Material Kit

Goal: make the low-poly prototype immediately readable and atmospheric without creating a high-cost asset pipeline. These are not final hero textures. They are high-leverage materials and decals that can cover hundreds of procedural meshes.

## Priority A — world-defining surfaces

1. **Dark cracked asphalt** — seamless, wet-capable. Roads, parking lots, service yards.
2. **Dirty poured concrete** — seamless. Buildings, tunnels, bunkers, industrial floors.
3. **Painted concrete / faded plaster** — 2–3 tintable variants. Residential/commercial walls.
4. **Corrugated galvanized metal** — clean + rusty variation. Warehouses, roofs, shutters.
5. **Rusted dark steel** — machines, beams, pipes, salvage, Breach-industrial hybrids.
6. **Painted industrial steel** — tintable neutral material for generators, doors and machinery.
7. **Old brick / block masonry** — one dark desaturated atlas is enough for early towns.
8. **Damp cave rock** — tiling rock with strong normal + roughness response to flashlight.
9. **Packed dirt / mud** — forest edges, lots, underground transitions.
10. **Dark forest ground** — dirt + sparse leaves / moss, low contrast so flashlight remains dominant.

## Priority B — tiny textures with huge visual payoff

11. **Road marking atlas** — white/yellow lines, arrows, parking markings, crossings.
12. **Warning / industrial decal atlas** — hazard stripes, voltage signs, numbers, arrows, restricted areas.
13. **Grime / leak decal atlas** — water streaks, oil, soot, damp patches.
14. **Rust streak decal atlas** — under bolts, beams, drains and corrugated panels.
15. **Crack / damage decal atlas** — concrete cracks, chipped paint, small bullet damage.
16. **Poster / abandoned signage atlas** — generic fictional brands and public notices.
17. **Window atlas** — dark glass, dirty glass, broken glass and a few weak emissive windows.
18. **Chain-link / fence alpha texture** — one small masked texture can build huge industrial perimeters.
19. **Metal grate alpha / normal** — catwalks, drains, underground maintenance floors.
20. **Caution tape / striped trim** — useful for readable entrances and engineering areas.

## Priority C — night / flashlight eye candy

21. **Wetness macro mask** — use as a material function or vertex-painted blend, not a unique texture per asset.
22. **Puddle normal + roughness patch** — flashlight reflections will make cheap floors look far richer.
23. **Dust / dirt roughness variation mask** — subtle breakup for large flat low-poly walls.
24. **Fingerprint / smudge mask for glass and metal** — only for close interactive objects.
25. **Retroreflective road-sign material mask** — reacts strongly to the flashlight and immediately sells nighttime exploration.
26. **Reflective safety tape material** — vehicles, machinery, generators, doors.
27. **Small emissive LED atlas** — green/red/amber status lights for every engineering device.

## Priority D — Breach / fantasy layer

28. **Breach energy noise** — monochrome flowing noise, reusable for purple/blue/amber emissive materials.
29. **Arcane inscription atlas** — abstract symbols rather than readable fantasy language.
30. **Crystalline normal / roughness** — use with tint + emissive to generate many different cores.
31. **Organic mineral / vein mask** — blends onto cave rock to suggest the Breach spreading through geology.
32. **Black stone / impossible architecture material** — very rough dark base with subtle directional breakup.

## Vegetation

33. **Pine / dead-tree bark** — one bark texture can cover most trunks at this stage.
34. **Dark leaf / pine-needle card** — alpha masked; keep silhouette more important than texture detail.
35. **Dead grass clump atlas** — cheap scatter around roads, lots and abandoned buildings.
36. **Moss strip / edge decal** — high payoff in underground damp zones.

## Recommended texture packing

For most opaque materials use:

- Base Color
- Normal
- **ORM packed texture:** R = Ambient Occlusion, G = Roughness, B = Metallic

For masks/decals, prefer small atlases rather than dozens of files.

## Resolution budget for prototype

- Generic tiling materials: **1K** is usually enough while the art direction is still moving.
- High-frequency hero surfaces used right in front of the player: **2K**.
- Decal atlases: **2K** shared atlas.
- Tiny engineering LEDs / signs: **512–1K atlas**.
- Do not start building a 4K library yet.

## Material philosophy

The flashlight is the visual hero. Materials should therefore prioritize **roughness and normal response** over complicated Base Color artwork. A boring grey concrete with good roughness, normals, wet patches and decals will look much better at night than an elaborate diffuse texture with flat lighting.

The first six materials I would actually acquire/create are:

**asphalt + concrete + corrugated metal + cave rock + dirt + grime/leak decal atlas.**

Those six alone can transform the current prototype dramatically.
