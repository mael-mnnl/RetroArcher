# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

RetroArcher is a pixel-art roguelike arcade game built with **Qt Framework (C++17)**. Players navigate procedurally-generated dungeons, defeat enemies, collect skills/relics, and face bosses across 6 worlds. The codebase is approximately 5,600 lines of C++, with comments and some variable names written in French.

## Build Commands

```bash
# Generate Makefile from Qt project
qmake RetroArcher.pro

# Build the executable
make

# Run the game
./RetroArcher
```

The project uses **Qt qmake** as its build system. There are no tests or linters configured. Build artifacts (`*.o`, `moc_*`, `qrc_*`, `build/`, executables) are gitignored.

## Architecture

The game follows a strict **MVC pattern** with 14 independent controller systems.

### Entry Points
- `main.cpp` — application entry, creates `GameWidget`
- `gamewidget.cpp` — owns the 60 FPS `QTimer` → `tick()` game loop, connects all systems, holds `GameModel`

### Model (`model/`)
Central state is the `GameModel` struct (`gamemodel.h`), which contains all mutable game state: player, enemies, bullets, particles, skills, progression. There is no inheritance — all entities (`Player`, `Enemy`, `Bullet`, `Particle`) are plain structs defined in `entities.h`. Enums for `GameState`, `EnemyType`, `Skills`, etc. live in `types.h`.

### View (`view/`)
- `GameView` — renders the in-game scene (HUD, room, sprites) each frame via `QPainter`
- `MenuView` — renders all UI screens (menus, skill selection, shop, leaderboard)
- `AssetManager` — loads and caches PNG spritesheets; applies hue-shifting for color variants

Rendering is immediate-mode: the entire frame is repainted every tick.

### Controller (`controller/`)
Fourteen namespace-scoped systems, each processing a slice of game logic per tick. Execution order in `gamewidget.cpp`:
1. `PlayerController` — input, movement, dash, animation
2. `EnemyAI` — movement, attack patterns, phase transitions
3. `CombatSystem` — bullets, damage, knockback, grenades
4. `SkillSystem` — passive aura effects (fire trail, decay, regen)
5. `CollisionSystem` — player-enemy, bullet-enemy, pickup detection
6. `PickupSystem` — gold/potion/heart drops
7. `ScoreSystem` — combo tracking, leaderboard
8. `RelicSystem` / `CurseSystem` — modifier effects
9. `RoomBuilder` — spawns enemies and room content
10. `BiomeFXSystem` — world-specific visual particles
11. `DialogSystem` / `TutorialSystem` / `CheatSystem` — auxiliary systems

### Core (`core/`)
- `constants.h` — game canvas is 544×416 px at 2× display scale (1088×960 px final), HUD is 64 px
- `palette.h` — shared color constants
- `utils.h/.cpp` — math helpers

## Key Data Flow Patterns

**Adding a new skill:** Define its enum in `types.h`, add metadata in `skillsdata.cpp`, handle its effect in `skillsystem.cpp` (passive) or `combatsystem.cpp` / `playercontroller.cpp` (active).

**Adding a new enemy:** Add enum to `types.h`, define AI behavior in `enemyai.cpp`, add spawn logic to `roombuilder.cpp`, add sprite loading in `assetmanager.cpp`.

**Game state transitions** are managed via `GameModel::state` (a `GameState` enum). `GameWidget::tick()` reads this to decide which systems run and which view renders.

**Persistence** uses `QSettings` (platform-native config storage), managed through `savemanager.cpp`.

## Display & Assets

All sprites are PNG sheets in `assets/`. Animations are frame-based (manual index advancement per tick). Boss sprites follow the naming convention `boss_<name>.png`; elite variants live in `assets/elite/`; VFX in `assets/fx/`.

Game resolution: 544×416 px logical, rendered at 2× scale.
