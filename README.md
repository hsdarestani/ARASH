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

## One-command CC0 art test

The repository includes a reproducible environment-art smoke test built from a pinned subset of **KayKit Dungeon Remastered** under CC0 1.0.

On the Windows Unreal workstation, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\Tools\Art\run_cc0_vertical_slice.ps1
```

The command downloads the source pack, imports and verifies the required static meshes, and opens the prototype arena. Downloaded source art remains ignored and is not committed.

See [`Docs/CC0_ART_SMOKE_TEST.md`](Docs/CC0_ART_SMOKE_TEST.md) for options and troubleshooting.

Binary Unreal assets (`.umap`, `.uasset`, animation, Niagara systems, materials and meshes) are generated or imported on an Unreal workstation rather than stored by the bootstrap scripts.

## Engine

Target engine: **Unreal Engine 5.8**.

The repository intentionally excludes Unreal-generated directories (`Binaries`, `Intermediate`, `Saved`, `DerivedDataCache`).

## Infrastructure

Public project endpoint: `arash.smarbiz.sbs`

`ops/bootstrap-server.sh` provisions the Ubuntu host with Nginx, a `/health` endpoint and optional Let's Encrypt TLS.

The GitHub workflow `.github/workflows/deploy-infra.yml` uses repository secrets `HOST` and `PASS`. It never stores their values in source control.

For long-term operation, password-based root SSH should be replaced with a restricted deployment user + SSH deploy key.

## Next implementation milestone

1. Run the CC0 art smoke test on the UE 5.8 workstation.
2. Tune imported mesh scale, material palette and arena composition.
3. Add three enemy archetypes + one elite.
4. Add an upgrade-choice screen and first 10 upgrades.
5. Add hit-stop, camera impulse, Niagara trails/impacts and sound hooks.
6. Produce a 10-minute playable vertical slice before expanding content.
