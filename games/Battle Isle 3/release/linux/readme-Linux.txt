Battle Isle 3 for Linux (x86)
Version 1.0.0

Original Battle Isle 3 is required for playing.
(version from GOG.com can be used for playing)

Installation
------------

Put files from this archive into the installed game's directory.
The game looks for the CDs in the DATA subdirectory (in installed game's directory).
If you have version from GOG.com, then the CDs are already copied there.
Otherwise you have to create the DATA subdirectory and copy the CDs there.
Alternatively you can copy the CDs elsewhere, but you need to edit the path to them in BattleIsle3.sh.
Run BattleIsle3.sh in the installed game's directory.


Music
-----

The game's MIDI music can be played using one of following libraries:
ALSA sequencer, WildMIDI, BASSMIDI, ADLMIDI

ADLMIDI is the default library, others can be selected in the configuration file.
ALSA sequencer can use hardware or software synth (like Fluidsynth or TiMidity++).
ADLMIDI requires no additional files for MIDI playback,
WildMIDI requirer GUS patches for MIDI playback,
BASSMIDI requires a soundfont for MIDI playback,
ADLMIDI uses OPL3 emulator for MIDI playback.

ALSA sequencer can detect usable synth automatically or it can be selected in the configuration file.

GUS patches can be installed anywhere, but the file timidity.cfg must be
either in the game's directory or in /etc/timidity/timidity.cfg
EawPats is a good sounding set of patches.

Soundfont (for BASSMIDI) can be either copied to the game's directory
or it can be stored anywhere, but the soundfont location must be written
in the configuration file.


Configuration
-------------

Configuration is stored in the file BI3.cfg.
Paths to games files can be set in the file BattleIsle3.sh.


Misc
----

The game requires Wine (https://www.winehq.org/) to run.

The game also requires following 32-bit libraries: quicktime2
On debian based distributions these libraries are in following packages: libquicktime2:i386

Source code is available on GitHub: https://github.com/M-HT/SR


Changes
-------

v1.0.0 (2021-06-27)
first Linux (x86) version
