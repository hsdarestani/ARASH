# ARASH

ARASH is a Persian-mythic action roguelite prototype built with Unreal Engine.

**Core pitch:** one mythic archer, one arrow, an army.

## Vertical slice goals

- Isometric / 3-quarter action camera
- Mouse aim + WASD movement
- Charged bow shot
- Arrow pierce, ricochet and return behavior
- Lightweight enemy archetypes
- Data-driven upgrades
- Persian mythic dark-fantasy art direction
- Niagara-driven combat feedback and Simurgh effects

## Current code

The bootstrap branch contains the initial Unreal C++ gameplay skeleton:

- `AArashCharacter` — movement, aiming, dodge and firing
- `UArashBowComponent` — charge/release logic
- `AMythicArrowProjectile` — damage, pierce, bounce and return behavior
- `AArashEnemyBase` — lightweight prototype enemy
- `UArrowUpgradeDefinition` — data-driven upgrade definitions
- `AArashGameModeBase` — prototype game mode

Binary Unreal assets (`.umap`, `.uasset`, animation, Niagara systems, materials and meshes) will be added once the first Unreal workstation/build runner is connected.

## Engine

Target engine: **Unreal Engine 5.8**.

The repository intentionally excludes Unreal-generated directories (`Binaries`, `Intermediate`, `Saved`, `DerivedDataCache`).

## Infrastructure

Public project endpoint: `arash.smarbiz.sbs`

`ops/bootstrap-server.sh` provisions the Ubuntu host with Nginx, a `/health` endpoint and optional Let's Encrypt TLS.

The GitHub workflow `.github/workflows/deploy-infra.yml` uses repository secrets `HOST` and `PASS`. It never stores their values in source control.

For long-term operation, password-based root SSH should be replaced with a restricted deployment user + SSH deploy key.

## Next implementation milestone

1. Compile project on a UE 5.8 workstation.
2. Create the first prototype map and placeholder arena.
3. Add three enemy archetypes + one elite.
4. Add an upgrade-choice screen and first 10 upgrades.
5. Add hit-stop, camera impulse, Niagara trails/impacts and sound hooks.
6. Produce a 10-minute playable vertical slice before expanding content.
