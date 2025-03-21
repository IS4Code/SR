Septerra Core for Linux (x86)
Version 1.04.0.7

Original Septerra Core version 1.04 is required for playing.
(version from GOG.com can be used for playing)

Installation
------------

Put files from this archive into the installed game's directory.
Run Septerra.sh in the installed game's directory.

Configuration
-------------

Configuration is stored in the file Septerra.cfg.


Misc
----

The game requires following 32-bit libraries: SDL2, mpg123, quicktime2
On debian based distributions these libraries are in following packages: libsdl2-2.0-0:i386 libmpg123-0:i386 libquicktime2:i386

Source code is available on GitHub: https://github.com/M-HT/SR

Using mouse polling rate higher than default 125Hz can result in mouse stuttering.
Setting CPU_SleepMode (in configuration file) to "reduced" can help. If not, then set CPU_SleepMode to "nosleep".


Changes
-------
v1.04.0.7 (2022-01-08)
* add option to only use integer scaling

v1.04.0.6 (2021-11-04)
* add option to reduce cpu sleep, to help with higher mouse poll rates

v1.04.0.5 (2021-01-21)
* add scaling option without bilinear filtering
* add options to set command line parameters
* add option to set delay after image flip
* add options to enable cheats
* add options to switch WSAD and arrow keys

v1.04.0.4 (2020-04-13)
* fix playing .avi versions of movies from GOG Windows installer
* allow setting size of audio buffer

v1.04.0.3 (2020-04-13)
* fix writing configuration file

v1.04.0.2 (2019-10-11)
* optimize floating point operations
* add sleep to prevent too much CPU utilization

v1.04.0.1 (2019-09-29)
first Linux (x86) version
