# Fishing Voyage (渔途)

A 2D top-down pixel-art ocean survival game built with C++ and the Qt 6 framework.

Sail your ship across **9 stages** of increasing difficulty. Catch fish, fight sea creatures, dodge obstacles and storms, upgrade your gear, and defeat the Bosses guarding the later stages.

---

## How to Run (Packaged Executable)

1. Download and unzip the release package.
2. Double-click **FishingVoyage.exe** — no separate Qt or Visual Studio installation is required; all required Qt DLLs are bundled in the folder (via `windeployqt`) and all sprite/audio assets are embedded in the executable itself.
3. Requires Windows 10 or 11 (64-bit).
4. Your save file, high-score table, and fish/enemy/boss encyclopedia are created automatically in the same folder the first time you run the game — no setup needed.

> If Windows SmartScreen shows a warning (because the executable isn't code-signed), click **More info → Run anyway**.

## Controls

| Key / Input      | Action                                                       |
| ---------------- | ------------------------------------------------------------ |
| `W` `A` `S` `D`  | Move the ship                                                |
| `Shift` (hold)   | Boost speed (consumes stamina)                               |
| Left Mouse Click | Fish (click repeatedly near a fish to complete the QTE catch) **or** attack, depending on your currently equipped tool |
| `1`–`6`          | Quick-select weapon slot                                     |
| `B`              | Open Backpack / Inventory                                    |
| `H`              | Open Encyclopedia (fish / enemies / bosses discovered so far) |
| `E`              | Emergency shockwave skill                                    |
| `Space`          | Dash / sprint burst                                          |
| `Q`              | Save and quit                                                |
| `N`              | New game (from main menu, or after defeat)                   |
| `C`              | Continue from your last save (main menu, if a save exists)   |
| `M`              | Return to main menu                                          |
| `Esc`            | Pause / back                                                 |

## Gameplay Overview

### The Basic Loop

Sail rightward through 9 stages, each longer and harder than the last. Catch fish for coins, fight or avoid enemies, watch out for obstacles and weather, and spend your coins at the shop between stages. Reach the end of a stage that has a Boss and you must defeat it to continue. The run ends when you clear Stage 9 or your ship's durability reaches zero.

### Fish (12 species)

Press left-click repeatedly while near a fish to complete a timed QTE catch. Rarer fish are worth much more but demand more clicks in a shorter window.

| Fish         | Type   | Value (coins) | Clicks needed | Time limit |
| ------------ | ------ | ------------- | ------------- | ---------- |
| Sardine      | Common | 5–15          | 3             | 3.0s       |
| Anchovy      | Common | 8–18          | 3             | ~3.2s      |
| Clownfish    | Common | 12–24         | 4             | 3.0s       |
| Tuna         | Common | 25–55         | 3             | 3.0s       |
| Mackerel     | Common | 35–65         | 4             | ~2.75s     |
| Sea Bream    | Common | 45–80         | 5             | ~2.6s      |
| Deep-Sea Eel | Rare   | 80–140        | 8             | 2.0s       |
| Lanternfish  | Rare   | 95–160        | 7             | ~1.75s     |
| Grouper      | Rare   | 110–190       | 7             | 2.0s       |
| Golden Fish  | Rare   | 150–250       | 10            | 1.25s      |
| Koi Fish     | Rare   | 180–280       | 9             | 1.5s       |
| Crystal Fish | Rare   | 240–380       | 11            | ~1.4s      |

Common fish flee once you're within ~120 units and change direction every couple of seconds; rare fish notice you from further away, flee faster, and change direction more often, making them harder to line up a catch on.

### Enemies (5 types, introduced gradually across the 9 stages)

- **Shark** — patrols and bites when close, with a cooldown between bites.
- **Swordfish** — freezes briefly to wind up, then charges in a straight line at high speed; watch for the warning cue before it charges.
- **Octopus** — periodically turns invisible for a short window, making it harder to track or avoid.
- **Electric Ray** — has a pulse-style attack with an area of effect around it.
- **Poison Jellyfish** — inflicts a lingering poison effect on contact that ticks damage over time.

### Bosses (3, guarding designated stages)

Every Boss becomes **enraged** once its health drops to half — dealing roughly 20% more damage and attacking faster — on top of its own unique phase-2 mechanic:

- **FiveHeadSharkBoss** — alternates between melee lunges, summoning shark minions, and a telegraphed area bombardment.
- **TaliMonsterBoss** — attacks with a tentacle mouth-strike and a sweeping eye-beam; in phase 2 it becomes briefly invulnerable while a clone is alive, so you must deal with the clone first.
- **SirenBoss** — casts a multi-beam "Soul Song" and an area-denial "Elegy" pulse, and in phase 2 raises three resonance pillars that must be broken down as its health crosses 75% / 50% / 25%.

### Obstacles

- **Reef** — colliding with one stuns you briefly, damages ship durability, and knocks you back away from it.
- **Whirlpool** — the longer you stay in one, the more it slows your ship (up to a cap); leaving lets you gradually recover.

### Weather & Waves

Weather blends smoothly between **Sunny**, **Fog** (reduced vision), and **Storm** (reduced speed, occasional lightning strikes that damage durability — but fish sell for up to 50% more while a storm is active). Waves are telegraphed a few seconds before they arrive: moving with a wave gives a speed boost, moving against it slows you down, and the effect ramps in and out smoothly rather than snapping on.

### Weapons & Shop

Five weapons — **Fishing Rod, Fish Net, Harpoon, Pistol, Shotgun** — each upgradeable through 3 tiers (higher price, more damage, more durability). You start with an infinite-durability Rod and Harpoon so you're never stuck without a tool. Between stages, visit the shop to sell fish, buy food and repair kits (3 tiers), repair or upgrade weapons, and upgrade your ship's speed, stamina, or durability cap.

### Backpack & Encyclopedia

Press `B` to manage your equipped and carried weapons/items, and `H` to view your encyclopedia — a running record of every fish, enemy, and Boss you've discovered (32 entries in total), which persists across play sessions.

### Scoring

Your final score combines six weighted categories — stage progress, time, fish caught, total fish value, kills, and survival — into a single run score, used both for your end-of-run grade and the high-score leaderboard.


## Building from Source

1. Install **Visual Studio 2022** with the **Qt Visual Studio Tools** extension.

2. Install **Qt 6.x** (MSVC 2022 64-bit kit).

3. Open `FishingVoyage.vcxproj` in Visual Studio.

4. Select the **Release** configuration and build (Ctrl+Shift+B).

5. To produce a standalone distributable folder, run `windeployqt` against the built `.exe`:

   ```
   <Qt install path>\<version>\msvc2022_64\bin\windeployqt.exe path\to\FishingVoyage.exe
   ```

## Project Structure

```
FishingVoyage/
├── GameManager.h/.cpp          # Main game loop, stage/spawn logic
├── GameWindow.h/.cpp           # Qt rendering, HUD, input handling
├── Player.h/.cpp               # Ship movement, stamina, durability
├── Fish.h/.cpp                 # 12-species fish hierarchy
├── Enemy.h/.cpp                # Shark / Swordfish / Octopus / ElectricRay / PoisonJellyfish
├── Boss.h/boss.cpp             # Shared Boss phase model + 3 concrete bosses
├── Item.h/.cpp, Weapon.h/.cpp  # Item/Weapon hierarchy and economy
├── InventorySystem.h/.cpp      # Backpack, quick-slots, consumables
├── ShopDialog.h/.cpp           # Shop UI
├── BackpackDialog.h/.cpp       # Backpack UI
├── EncyclopediaDialog.h/.cpp   # Fish/enemy/boss encyclopedia UI
├── Obstacle.h/obstacle.cpp     # Reefs & whirlpools
├── WaveSystem.h/wavesystem.cpp       # Directional wave system
├── WeatherSystem.h/weathersystem.cpp # Sunny/fog/storm weather
├── FileManager.h/Filemanager.cpp     # Versioned save / highscore / encyclopedia I/O
├── GameConfig.h                       # Centralised tunable constants and per-stage config
├── main.cpp                           # Entry point
└── assets/, FishingVoyage.qrc         # Embedded sprites and audio
```

## Team

Developed as a C++ course project by a 5-member team:
**王志泓** (Team Lead — Qt UI & Integration), **冯俊杰** (Shop & Save), **范天润** (Map, Weather & Player), **洪放** (Fish & Logbook), **张邵涵** (Boss System).
