# Dino Game (LVGL)

Chrome-style dinosaur runner game built with C and LVGL 8.3, rendered via SDL2.

## Features

- Pixel-art dino, cacti, pterodactyls, clouds
- Jumping (SPACE / UP arrow) and ducking (DOWN arrow)
- Score counter with high-score tracking
- Increasing speed over time
- Day / night mode toggle every 700 points
- Ground parallax with decorative dots

## Controls

| Key | Action |
|-----|--------|
| SPACE / UP | Jump (or start / restart) |
| DOWN | Duck (faster fall when airborne) |

## Building

```bash
# Prerequisites: CMake, GCC/Clang, SDL2
sudo apt install -y libsdl2-dev cmake build-essential

cd dino_game
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./dino_game
```

## Project structure

```
dino_game/
├── CMakeLists.txt       # Build configuration
├── lv_conf.h            # LVGL configuration
├── lv_drv_conf.h        # LVGL drivers configuration (SDL)
├── main.c               # SDL + LVGL initialisation
├── dino_game.h          # Game types and constants
├── dino_game.c          # Game logic (sprites, physics, obstacles)
├── lvgl/                # LVGL 8.3 (git clone)
└── lv_drivers/          # LVGL SDL drivers (git clone)
```
