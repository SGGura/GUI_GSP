# Dino Runner Game — Chrome T-Rex style

A classic Chrome offline dinosaur runner game implemented using the **GUI_GSP** library
for monochrome 128×64 displays.

## Features

| Feature | Description |
|---------|-------------|
| **T-Rex sprite** | 20×22 px running animation (2 frames), ducking pose (26×14 px) |
| **Jump physics** | Gravity-based parabolic jump, hold to jump higher |
| **Obstacles** | Small cactus, large cactus, cactus group, pterodactyl (animated) |
| **Ground scroll** | Dashed ground line scrolling with the game speed |
| **Clouds** | Two clouds drifting in the background |
| **Score / HI** | 5-digit score counter, high-score persistence within session |
| **Speed ramp** | Game accelerates every 100 score points (up to max speed) |
| **Day / Night** | Palette inverts every ~700 ticks for a night-mode effect |
| **Game Over** | Blinking "GAME OVER" box, press jump/OK to restart |

## Controls

| Key | Action |
|-----|--------|
| `DINO_KEY_JUMP` (default 1) | Jump / start game / restart |
| `DINO_KEY_DUCK` (default 2) | Duck (avoid birds) |
| `DINO_KEY_OK`   (default 3) | Same as jump |

Override key codes by defining `DINO_KEY_JUMP`, `DINO_KEY_DUCK`, `DINO_KEY_OK`
before including `dino_game.h`.

## Integration

1. Add **GUI_GSP** sources and your display/keyboard driver to the project
   (see `PORTING_GUIDE.md` in the repository root).
2. Add `dino_game.c` and `main.c` (or call `DinoGame_Create(NULL)` from your own code).
3. In your timer ISR, call `GUI_TimerClock(period_ms)` (e.g. every 10 ms).
4. The main loop only needs `GUI_Run()`.

## Files

| File | Purpose |
|------|---------|
| `main.c` | Entry point — initialises GUI_GSP and creates the game |
| `dino_game.h` | Public API, game structures, tuning constants |
| `dino_game.c` | All game logic, physics, sprite data, rendering |

## Screen Layout (128×64)

```
┌────────────────────────────────────────┐
│          ☁          HI 00123  00045    │  ← clouds, score
│    ☁                                   │
│                                        │
│          ▓▓▓▓                          │  ← T-Rex
│        ▓▓▓▓▓▓▓▓      ╻                │
│        ▓▓▓▓▓▓▓▓      ╹╻╻              │  ← cactus
│────────────────────────────────────────│  ← ground (y=54)
│  - -  - -  - -  - -  - -  - -  - -    │  ← ground texture
└────────────────────────────────────────┘
```
