# Vendored libADLMIDI

This is a pruned copy of [libADLMIDI](https://github.com/Wohlstand/libADLMIDI),
the OPL3 FM-synth MIDI player. It is compiled directly into the WebAssembly
(Emscripten) build of the games, because the web build can't `dlopen` the
`midi-adlmidi` plugin the way native builds do.

- Upstream: https://github.com/Wohlstand/libADLMIDI
- Commit: `9760df4e0acfe27818dee38e786f3b1fea8c8e7f` (2026-06-24)
- Contents: only `src/` and `include/` (plus the license files); the CMake
  project, docs, tests, FM banks and examples were not copied.
- Pruned emulator cores: OPAL, Java, ESFMu, MAME, YMFM. Only the **NUKED**
  (default) and **DosBox** OPL3 cores are kept. The build defines
  `ADLMIDI_DISABLE_{OPAL,JAVA,ESFMU,MAME_OPL2,YMFM}_EMULATOR` accordingly
  (see `games/*/SConstruct`, `device == 'web'`).

Licensed under LGPL-2.1 / GPL-3 (see `LICENSE*`).
