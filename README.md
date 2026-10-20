# MagiaSmash

A 2D arena brawler that blends **Blood+-style sword combat** with **magical-girl themes**, built on the [orx](https://orx-project.org/) portable game engine.

Fight waves of enemies in a single arena: move, dash, jump, and cut your way
through with light and heavy attacks. Clear five waves to win.

> **Version:** 0.2.0 · **Status:** playable prototype · **License:** zlib

---

## Features

- **Player combat** — move, jump, dash, light attack, heavy attack, with
  hit detection, knockback, invulnerability frames, and dash cooldown.
- **Enemy types** — `Chiropteran` and the larger `BigChiropteran`, each with
  their own stats and attack patterns.
- **Wave progression** — five waves of increasing difficulty; health carries
  between waves. Win by clearing all waves, lose by running out of health.
- **Camera shake & FX** — hit sparks, slash arcs, dash pulses, and screen shake
  driven by effect data.
- **HUD** — health bar, wave label, enemy counter, and centered messages.
- **Fully data-driven content** — every fighter, move, enemy, wave, effect,
  color, and UI element is defined in config/Python. No recompilation needed to
  tune balance or add content.

## Architecture

The project is deliberately split between engine-level systems and content:

| Layer | Location | Responsibility |
| --- | --- | --- |
| **Game systems (C++)** | `src/`, `include/` | `Fighter` (a single combat system used by *all* combatants), `GameHUD`, and the Scroll framework glue (`MagiaSmash.cpp`). |
| **Game director (Python)** | `data/python/MagiaSmash.py` | Scene creation, wave spawning, win/lose flow, restart. Polls `GameState` every frame. |
| **Content (INI)** | `data/config/MagiaSmash.ini` | Fighters, moves, enemies, waves, FX, colors, UI layout, arena geometry. |

C++ and Python communicate through a shared `[MatchState]` config section:
the C++ side writes player health, position, enemies alive, wave, and game
state; Python reads them and drives spawning and flow.

Config parenthood is used for inheritance — e.g. `[Saya@Fighter]` automatically
gains the `Fighter` class and inherits its defaults, overriding only what it
needs.

## Requirements

- **Linux x86_64**; an X11 display with OpenGL 2.1+
- **orx** engine, built as a shared library (`liborx.so`)
- A C++11 compiler; builds use `-fno-exceptions -fno-rtti`

## Building

The build system is [premake4](build/premake4) with generated makefiles under
`build/linux/gmake/`. Set `ORX` to your orx checkout:

```sh
make -C build/linux/gmake config=release64 ORX=/path/to/orx
```

Available 64-bit configurations: `debug64`, `profile64`, `release64`,
`bundle64` (plus 32-bit and web variants). Output binaries land in `bin/`:

| Config | Binary | Notes |
| --- | --- | --- |
| `debug64` | `bin/MagiaSmashd` | Asserts + logging; loads `MagiaSmashd.ini` |
| `profile64` | `bin/MagiaSmashp` | Optimized; FPS + profiler overlay |
| `release64` | `bin/MagiaSmash` | Optimized, shipping build |

The post-build step copies the orx shared libraries into `bin/`.

## Running

Run from `bin/`, which is a sibling of `data/` (orx resolves the game config
and `${../data}/…` resources relative to this layout):

```sh
cd bin
./MagiaSmash
```

### Controls

| Action | Key |
| --- | --- |
| Move | `Left` / `Right` |
| Jump | `Up` |
| Dash | `Left Shift` |
| Light attack | `J` |
| Heavy attack | `K` |
| Restart | `R` or `Enter` |
| Quit | `Esc` |

## Distribution layout

Release bundles are self-contained and must keep this layout:

```
MagiaSmash-<version>-linux-x86_64/
├── bin/
│   ├── MagiaSmash      # release executable
│   └── liborx.so       # bundled orx runtime (found via RUNPATH ./)
├── data/               # config, Python scripts, textures, fonts, audio
├── LICENSE
└── README.md
```

## Downloads

Prebuilt Linux x86_64 builds are attached to
[GitHub Releases](https://github.com/AnonymousOSTeam/MagiaSmash/releases).

## License

Released under the **zlib license** — see [`LICENSE`](LICENSE).

The bundled orx engine is also zlib-licensed:
Copyright (c) 2008- Orx-Project. orx incorporates additional third-party
libraries (pocketpy, FastNoiseLite, and others), each retaining its own license.
