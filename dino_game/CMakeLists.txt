cmake_minimum_required(VERSION 3.10)
project(dino_game C)

set(CMAKE_C_STANDARD 99)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# LVGL
file(GLOB_RECURSE LVGL_SOURCES
    lvgl/src/*.c
)

# lv_drivers (SDL)
set(LV_DRV_SOURCES
    lv_drivers/sdl/sdl.c
    lv_drivers/sdl/sdl_common.c
)

add_executable(dino_game
    main.c
    dino_game.c
    ${LVGL_SOURCES}
    ${LV_DRV_SOURCES}
)

target_include_directories(dino_game PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}/lv_drivers
    ${SDL2_INCLUDE_DIRS}
)

target_compile_definitions(dino_game PRIVATE
    LV_CONF_INCLUDE_SIMPLE
    LV_DRV_CONF_INCLUDE_SIMPLE
)

target_link_libraries(dino_game PRIVATE
    ${SDL2_LIBRARIES}
    m
    pthread
)
