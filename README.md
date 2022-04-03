# HZMoria
This is a fork of the last official release (5.6) of the classic roguelike UMoria.

It has a new name out of respect for the last official maintainer, David Grabiner, who has explicitly requested this of anyone planning to make significant changes (see `umoria.txt`).

## Introduction
UMoria is old, but I want to keep its memory alive because it was the first roguelike that really grabbed my attention as an IBM PC / MS-DOS gamer in the late 1980s. It's getting harder to make working ports of the vanilla source for modern OSes, though:
- UMoria is built around curses libraries for its I/O, but this is no longer a very good option for providing a consistent cross-platform experience in the age of GUIs.
- The UMoria codebase has been hacked to the depths and back to support platforms that are now considered thoroughly "retro", and trying to weave additional hacks into that for modern OS support is painful and error-prone.
- There's a lot of stuff that isn't really relevant anymore - and may even be a liability on modern OSes - such as multi-user support, being able to open a shell, etc.

I'd like to use this project as a launchpad to at least try cleaning up the source and porting to modern roguelike I/O libraries. I may even take it further and restructure things, or even refactor / convert to C++ etc., but I think I'll probably try to maintain the vanilla end user experience as much as possible (i.e. no major gameplay mechanics/features/balance changes that can't be disabled/reverted).

## Changes
So far, the following significant changes have been made:
- Moved the source files from what started as the `ibmpc` folder into the `source` folder.
- Removed all folders and files except for the contents of `docs`, `files`, and `source`.
- `docs` and MS-DOS config file `MORIA.CNF` are now under `files`, so that I can just grab the folder tree verbatim when making a release.
- Text files in `docs` have been renamed to have `.txt` extensions to make them more accessible to modern software.
- Created a CMake build and deploy system (currently only supports MinGW + PDCurses, but rectifying this is a near-term goal).

## License
The source code to UMoria 5.5.2 and older is licensed under a combination of GPLv2 and Public Domain, per the Free Moria project (http://free-moria.sourceforge.net/).

David Grabiner's UMoria 5.6 changes have been released under the GPLv3.

I will continue on with GPLv3 for this project in deference to those who came before me, despite my personal reservations about it.

# UMoria

I thought I should include some quick notes on the original, as well as my own involvement in it, as we both have a long history together at this point.

## History
UMoria is one of the oldest classic roguelikes, descended from BASIC and Pascal implementations on college VMS/VAX servers in the early 1980s. It was rewritten in K&R C in the mid-1980s, then ported to many home computers as well as other Unix systems.

UMoria's influence looms large: It is a direct ancestor of the popular Angband family of roguelikes, and it is also credited as an influence for Diablo.

## Involvement
My personal history with UMoria is long:
- 1980s: Discovered a 4.8-series release of the MS-DOS port, and it became a formative computer gaming experience!
- 1990s: Upon finally learning C, I immediately worked to produce the last official MS-DOS port: UMoria 5.5.2, built with DJGPP+PDCurses.
- 2000s: Supported an effort by fellow fan Ben Asselstine (who did most of the work in the end) to solicit open source / public domain license declarations from the original contributors: http://free-moria.sourceforge.net/
- 2010s: Collected the known history of the game's source releases in another GitHub project for posterity, along with branches+releases containing my attempts at modest Windows ports using various toolchains (MSVC, Cygwin, MinGW): https://github.com/HunterZ/umoria

I'm embarrassed to admit, however, that I've never actually conquered the game in all these years. It turns out that my interest in roguelikes is not quite matched by my skill at them!
