# RIFTWORKS — Unreal 5.8 Verification Checklist

Use this after every significant Unreal runtime pass. The goal is to validate what actually happens in PIE, not what the source code claims should happen.

## 1. First-person movement

- [ ] WASD moves normally.
- [ ] Mouse look works.
- [ ] Left Shift sprints.
- [ ] Left Ctrl visibly crouches: camera lowers and speed decreases.
- [ ] Left Ctrl again returns to standing height.

## 2. Flashlight / darkness

- [ ] F toggles the flashlight.
- [ ] Flashlight does NOT create a large opaque white fog cone in front of the camera.
- [ ] Nearby walls retain readable detail instead of becoming pure white.
- [ ] Distant darkness remains dark.
- [ ] Fog adds depth only; it should not look like smoke emitters attached to the player.
- [ ] No old flashlight cookie / strange circular artifacts remain.

## 3. Humanoid NPCs

- [ ] NPC mannequin is visible and correctly oriented.
- [ ] Idle animation plays while stationary.
- [ ] NPC patrols / walks when not alerted.
- [ ] Walk/run animation plays while moving.
- [ ] NPC detects flashlight/player and approaches if out of firing range.
- [ ] NPC turns toward player and fires when it has line of sight.
- [ ] Pistol-shoot animation plays during attack if the imported clip is available.
- [ ] One rifle hit produces visible hit response when the clip exists.
- [ ] Approximately two default rifle hits kill a default humanoid.
- [ ] Dead NPC stops AI/movement and either plays death animation or visibly falls sideways.

## 4. Buildings / city blockout

- [ ] Procedural buildings are hollow rather than solid cubes.
- [ ] Main entrances have physical doorway gaps.
- [ ] Player can walk inside at least most generated structures.
- [ ] Roof, floor and wall collision work.
- [ ] Industrial buildings have loading canopies / columns / rooftop shapes.
- [ ] Smaller buildings have porches/sign silhouettes and no longer read as plain grey boxes.

## 5. Underground

- [ ] There is an actual enclosed descending service tunnel.
- [ ] Player can walk down it.
- [ ] Tunnel has floor, walls and roof.
- [ ] Supports repeat along the route.
- [ ] Larger underground chambers are enterable.
- [ ] Chambers have openings/doorways rather than being solid blocks.
- [ ] Deep Breach light is visible but not a giant volumetric blob.

## 6. Colossus

- [ ] Old block-built Walker has been replaced by giant mannequin prototype.
- [ ] Giant mannequin uses same shared humanoid asset/rig.
- [ ] Colossus visibly moves around its route.
- [ ] Walk animation plays while it moves.
- [ ] Invisible legs/body/head collision zones can be shot.
- [ ] Existing harpoon/weakpoint/harvest logic still recognizes the Colossus.

## 7. Construction

- [ ] B toggles Build Mode.
- [ ] A build preview appears in front of the player.
- [ ] Mouse wheel cycles Platform / Beam / Wheel / MotorWheel.
- [ ] R rotates preview.
- [ ] Q toggles anchored vs physics placement.
- [ ] RMB places the selected part.
- [ ] Anchored pieces remain fixed.
- [ ] Physics pieces fall/react to Chaos.
- [ ] Existing engineering joints / logic / Breach systems can interact with these parts afterward.

## 8. Placeholder visual readability

- [ ] Roads, structures, terrain, trunks and foliage are visually separable at night.
- [ ] Silhouettes read from flashlight range.
- [ ] The scene no longer looks like one uniform grey blockout.

## Reporting format

Send only the failed items plus a screenshot/video when useful. Example:

- 2.3 wall still clips to white
- 3.3 NPC never patrols
- 6.2 mannequin invisible
- 7.2 build preview missing

That keeps iteration fast and prevents fixing code that is already behaving correctly.
