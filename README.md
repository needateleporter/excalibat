# Excalibat
A modern source port of the 1995 DOS game [Rise of the Triad](https://www.giantbomb.com/rise-of-the-triad-dark-war/3030-5403/). It is based on [the icculus.org ROTT port](https://icculus.org/rott/) and aims to maintain compatibility with the original game files (and the game's "feel") while improving support for modern systems.

```
NOTE FROM THE DEV:

This project is far from being complete. Although there are
many features and bugfixes I'd like to implement (such as
better memory management, sound emulation, online multiplayer
with client-side prediction, and full modern gamepad support),
I no longer have time nor interest in maintaining it.

I've been working on this on-and-off for years now in secret
and I hope this gets fans of the game interested. Feel free 
to fork this project and make it into something awesome.

- Nes (10/24/2017)
```

## Features
 * Uses the [SDL2](https://www.libsdl.org/) and [SDL2_mixer](https://www.libsdl.org/projects/SDL_mixer/) libraries and designed to be portable with modern operating systems.
 
 * Support for numerous video resolutions (including widescreen resolutions), multiple screen scaling options, and automatic aspect ratio correction (for more authenticity).
 
 * All retail and shareware versions of the game are now handled by the same executable based on the availability of the data files (WAD) and level files (RTL/RTC) in the game directory. For directories with both a retail and a shareware version, you can use the `shareware` parameter to force use of the latter.
 
 * Support for multiple level packs in the same game session (retail versions only), including separate save game files and high score tables for each level pack. (Currently limited to darkwar, huntbgin, and extreme. The `filertl` parameter can also be used for custom level packs)
 
 * New configuration, save game, and high score file structure to differentiate with the original files.
 
 * An optional parameter, `pure`, which disables most of the modern features for a more authentic experience.
 
 * Major source code overhaul for easier maintenance and modification. (Incomplete)
 
This port has been tested on Windows 10, compiled with [MinGW](http://www.mingw.org/) using the [Code::Blocks IDE](http://www.codeblocks.org/).

You can buy the original game [here](https://www.gog.com/game/rise_of_the_triad__dark_war), [here](http://store.steampowered.com/app/238050/The_Apogee_Throwback_Pack/), or [here](http://store.steampowered.com/app/358410/Rise_of_the_Triad_Dark_War/). The original game's source code can be found [here](https://github.com/videogamepreservation/rott).

This project is licensed under the GNU General Public License (see LICENSE for more information) and is not affiliated with Apogee Software, Ltd.