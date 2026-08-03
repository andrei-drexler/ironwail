<a href="https://github.com/andrei-drexler/ironwail/releases/latest">![GitHub Release](https://img.shields.io/github/v/release/andrei-drexler/ironwail?display_name=release&style=for-the-badge&label=Download)</a>

# What's this?
A fork of the popular GLQuake descendant [QuakeSpasm](https://sourceforge.net/projects/quakespasm/) with a focus on high performance instead of maximum compatibility, with a few extra features sprinkled on top.

## Does performance still matter, though? I'm getting 1000 fps in QS on e1m1
On most maps performance is indeed not much of a concern on a modern system. In recent years, however, some mappers have tried more ambitious/unconventional designs with poly counts far exceeding those of the original id Software levels from 25 years ago. It's also not uncommon for players of such an old game to be using hardware that is maybe not the latest and greatest, struggling on complex maps when using traditional renderers. By moving work from the CPU to the GPU (culling, lightmap updates) and taking advantage of more modern OpenGL features (instancing, compute shaders, persistent buffer mapping, indirect multi-draw, bindless textures), this fork is capable of handling even the [most](https://www.quaddicted.com/reviews/ter_shibboleth_drake_redux.html) [demanding](https://www.quaddicted.com/forum/viewtopic.php?id=1171) [maps](https://www.quaddicted.com/reviews/ravenkeep.html) at very high framerates. To avoid physics issues the renderer is also decoupled from the server (using code from [QSS](https://github.com/Shpoike/Quakespasm/), via [vkQuake](https://github.com/Novum/vkQuake)).

## Bonus features
- ability to play the [2021 release content](https://store.steampowered.com/app/2310/QUAKE/) with zero setup: if you have [Quake](https://store.steampowered.com/app/2310/QUAKE/) on [Steam](https://store.steampowered.com/app/2310/QUAKE/), you can unzip the [latest Ironwail release](https://github.com/andrei-drexler/ironwail/releases/latest) in any folder (that doesn't already contain a valid Quake installation) and simply run the executable to play the game, including any add-ons you have already downloaded
- new *Mods* menu, for quick access to any add-ons you've already installed
- ability to change weapon key bindings using the UI, not just the console
- ability to use the mouse to control the UI
- alternative HUD styles based on the Q64 layout (classic one is still available, of course)
- real-time palettization (with optional dithering) for a more authentic look
- classic underwater warp effect
- more options exposed in the UI, most of them taking effect instantly (no vid_restart needed)
- support for lightmapped liquid surfaces
- lightstyle interpolation (e.g. smoothly pulsating lighting in [ad_tears](https://www.moddb.com/mods/arcane-dimensions))
- reduced heap usage (e.g. you can play [tershib/shib1_drake](https://www.quaddicted.com/reviews/ter_shibboleth_drake_redux.html) and [peril/tavistock](https://www.quaddicted.com/forum/viewtopic.php?id=1171) without using -heapsize on the command line)
- reduced loading time for jumbo maps
- slightly higher color/depth buffer precision to avoid banding/z-fighting artifacts
- a more precise ~hack~work-around for the z-fighting issues present in the original levels
- capped framerate when no map is loaded
- ability to run the game from a folder containing Unicode characters

## System requirements

| | Minimum GPU | Recommended GPU |
|:--|:--|:--|
|NVIDIA|GeForce GT 420 ("Fermi" 2010)|GeForce GT 630 or newer ("Kepler" 2012)|
|AMD|Radeon HD 5450 ("TeraScale 2" 2009) |Radeon HD 7700 series or newer ("GCN" 2012)|
|Intel|HD Graphics 4200 ("Haswell" 2012)|HD Graphics 620 ("Kaby Lake" 2016) or newer|

Notes:
1) These requirements might not be 100% accurate since they are based solely on reported OpenGL capabilities. There could still be unforeseen compatibility issues.
2) Mac OS is not supported at this time due to the use of OpenGL 4.3 (for compute shaders), since Apple has deprecated OpenGL after version 4.1.
