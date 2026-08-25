# Albion

A browser version of [Albion](https://albion.wiki.gg/), the 1995 role-playing game by Blue Byte.

[▶ Play here](https://albion.is4.site/online/)

The game runs natively in WebAssembly thanks to the static recompiler from the [SR](https://github.com/M-HT/SR) port,
of which this repository is a fork with the aim to bring further improvements.

## Credits

* [M-HT](https://github.com/M-HT/SR): The original SR project without which this wouldn't be possible.
* [Jurie Horneman](https://github.com/jhorneman): Releasing Albion source codes which helped in analyzing the binary.
* [CSinkers](https://github.com/csinkers) and [Flo](https://github.com/a2flo): Earlier reverse-engineering effort.
* The Albion team.

## Features

### General

* Works in the browser; game data is downloaded only as it is needed.
* Saves are kept in the browser, and can be exported and imported as a ZIP file.
* Saved game state can be shared as a link with other players.
* Screenshots to a file or straight to the clipboard.
* Extra controls to make the game playable on phones.
* Mouse look and WASD movement.
* Support for additional languages.

### Graphics and sound

* The 3D view has adjustable field-of-view.
* The 2D view has adjustable zoom level.
* Intro and credits are remastered and play as native browser video.
* Supports FluidSynth and ADLMIDI to play the music using a SoundFont or OPL3.

## Building

See the [SR](https://github.com/M-HT/SR) project for build instructions.
