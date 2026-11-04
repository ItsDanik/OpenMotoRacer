OpenMotoRacer @VERSION@ for Windows (64-bit)
===========================================

Moto Racer (Delphine Software, 1997, Windows 95), written again by reading
the original program. The game's software renderer, one player: every track
and bike, practice, single race and the championship, the time attack's
ghost, replays and the TV director, the game's menus, sounds, voices and
music. It plays GOG's 1998 version and the original 1997 CD, each one's own
way. Not in it: the Direct3D renderer and the network and modem race.

Beta. Please report problems at https://github.com/ItsDanik/OpenMotoRacer/issues

YOU NEED THE GAME
  OpenMotoRacer contains no part of the game. Put your copy in a folder
  named "Moto Racer" next to openmotoracer.exe, either one of:

      GOG's, as installed:        Moto Racer/game.EXE
                                  Moto Racer/data/...
                                  Moto Racer/02.wav ... 13.wav  (the music)

      the 1997 CD's files:        Moto Racer/DISK1/...
                                  Moto Racer/DATA/...
        and for its music either the CD's image (.cue and .bin) in that
        folder, the folder above it or a folder named CD there, or its
        audio tracks as files:    Moto Racer/music/track02.ogg ... track13.ogg
        (.mp3 or .wav do as well).

  (or anywhere, and start the program with  -data <that folder>). Upper or
  lower case does not matter; nothing has to be installed.

RUN
  Double-click openmotoracer.exe. Windows 10 or 11, 64-bit. If Windows warns
  about an unknown program: More info -> Run anyway.
  openmotoracer.exe --help in a command prompt lists the options.

GAME BEHAVIOUR
  Options -> Game Behaviour: 1997 CD or GOG RELEASE, with either copy's files.
  1997 CD rides as the original disc did (how the bike leans, steers and
  slides), its music stops at a track's end and holds during a pause.
  GOG RELEASE follows the 1998 patch, its music looping. Until you choose, it
  follows the copy you play.

KEYS
  Menus:  the mouse, or the arrows, ENTER and ESC. Alt+Enter: window or the
          whole screen.
  Riding (Options -> Config next to Controller changes them):
          UP / DOWN accelerate / brake, LEFT / RIGHT steer,
          A / Z gear up / down, SPACE turbo and stunts, TAB look back.
  Race:   ESC the race's menu, PAUSE pause, F1 the game's help with every
          other key, F2 / F3 the picture's size, F4 16:9, F5 camera distance,
          F6 on-board view, F7 dashboard, F8 turn signs, F9 map,
          F10 smooth motion on / off, F11 / F12 the sound and display columns.
  Game pad: pick it under Options -> Controller, and its buttons under
          Config. The right / left triggers accelerate / brake by degrees
          without being set; START pauses; the cross and the left stick move
          through the menus. It rumbles (Options, the settings page).

FILES IT KEEPS
  In the folder it is started from (openmotoracer.exe's, when double-clicked):
  the game's settings, records, championships and time attack ghosts, and
  openmotoracer.ini, the port's own settings (smooth motion, analog, rumble,
  game behaviour).
  (or -home <folder>)

LICENCE
  GPL-3.0 (LICENSE.txt). SDL2.dll: zlib licence (LICENSE-SDL2.txt).
  Source: https://github.com/ItsDanik/OpenMotoRacer
  Moto Racer is Delphine Software's; OpenMotoRacer is not connected with them
  or with its publishers.
