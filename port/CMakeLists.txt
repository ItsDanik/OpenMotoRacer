cmake_minimum_required(VERSION 3.16)
project(openmotoracer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)    # for editors: port/build/compile_commands.json

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -O2 -fwrapv -ffp-contract=off -fsigned-char)   # (char as on the PC: ARM's is unsigned)

# SDL2, linked dynamically
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

set(OMR_SOURCES
  src/main.c
  src/files.c
  src/exe.c
  src/video.c
  src/bitmap.c
  src/clock.c
  src/menu.c
  src/page_main.c
  src/page_intro.c
  src/r3d.c
  src/model.c
  src/rand.c
  src/sound.c
  src/obj3d.c
  src/config.c src/choices.c src/page_mode.c src/page_track.c src/page_bike.c src/bike3d.c src/pose.c src/shadow.c src/effects.c src/race.c src/race_sound.c src/bikephys.c src/cpu.c src/camera.c src/replay.c src/ghost.c src/generic.c src/rules.c src/dialog.c src/pointer.c src/hud.c src/input.c src/joy.c src/page_top5.c src/page_load.c src/page_champ.c src/page_podium.c src/page_options.c src/saves.c src/scores.c src/bikes.c src/land.c src/horizon.c src/sprite.c src/signs.c src/trackmap.c src/racekeys.c
  src/page_name.c
  src/pad.c src/cdaudio.c
  src/script.c
  src/font.c
  src/text.c
)

add_executable(openmotoracer ${OMR_SOURCES})
target_include_directories(openmotoracer PRIVATE ${SDL2_INCLUDE_DIRS})
target_compile_options(openmotoracer PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_libraries(openmotoracer PRIVATE ${SDL2_LDFLAGS} m)
