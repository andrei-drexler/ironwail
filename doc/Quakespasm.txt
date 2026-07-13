  QuakeSpasm

  ______________________________________________________________________


  Page last edited: July 2024.


  1. About

  QuakeSpasm is a modern cross-platform Quake engine based on FitzQuake.

  It includes support for 64 bit CPUs and custom music playback, a new
  sound driver, some graphical niceities, and numerous bug-fixes and
  other improvements.

  Quakespasm utilizes SDL-1.2 or SDL2 frameworks, choose one which works
  best for you. SDL2 has nicer features, smoother mouse input and better
  support for newer operating systems, but no CD-ROM support.


  2. Downloads

  -  Project Downloads: http://quakespasm.sourceforge.net/download.htm

  -  Latest code repository:
     https://sourceforge.net/p/quakespasm/quakespasm/ci/master/tree/

  -  Github mirror:
     https://github.com/sezero/quakespasm


  3. Hints

  Visit the FitzQuake homepage <https://celephais.net/fitzquake/> for a
  full run-down of the engine's commands and variables.

  -  To disable some changes, use "quakespasm -fitz"

  -  Quakespasm's custom data is stored in "quakespasm.pak". Install
     this file alongside your id1 directory to enable the custom console
     background and other minor features.

  -  For different sound backend drivers use :
     "SDL_AUDIODRIVER=DRIVER ./quakespasm"
     where DRIVER may be alsa, dsp, pulse, esd ...

  -  Shift+Escape draws the Console.

  -  From the console, use UP to browse the command line history and TAB
     to autocomplete command and map names.

  -  There is currently no CD Music volume support and SDL2 doesn't
     support CD audio. cd_sdl.c needs replacing with cd_linux.c,
     cd_bsd.c etc..

  -  In windows, alternative CD drives are accessible by
     "quakespasm -cddev F" (for example)

  -  Quakespasm allows loading new games (mods) on the fly with
     "game GAMENAME {-quoth/hipnotic/rogue}"

  -  Use "quakespasm -condebug" to save console log to "qconsole.log".
     SDL2 builds no longer generate stdout.txt/stderr.txt.


  Music Playback:

  Quakespasm can play various external music formats, including MP3, OGG
  and FLAC.

  -  Tracks should be named like "track02.ogg", "track03.ogg" ... (there
     is no track01) and placed into "Quake/id1/music".

  -  Unix users may need some extra libraries installed: "libmad" or
     "libmpg123" for MP3, and "libogg" and "libvorbis" for OGG.

  -  Use the "-noextmusic" option to disable this feature.

  -  As of 0.90.0, music is played back at 44100 Hz by default with no
     need to adjust "-sndspeed".

  -  See Quakespasm-Music.txt for more details.


  Controller Support:

  The SDL2 variant of Quakespasm supports Xbox 360 style game
  controllers.

  The default configuration uses the left analog stick for movement and
  the right for looking.

  If your controller doesn't work you can try placing this file
  https://raw.githubusercontent.com/gabomdq/SDL_GameControllerDB/master/gamecontrollerdb.txt
  in your Quake directory, it is a community-maintained database that
  adds support for more controllers to SDL2.

  Controller Support - Cvars:

  -  joy_deadzone_look - Fraction of look stick travel to be deadzone,
     between 0 and 1. Default is 0.175.

  -  joy_deadzone_move - Same as above, but for the move stick. Default
     is 0.175.

  -  joy_deadzone_trigger - Fraction of trigger range required to
     register a button press on the analog triggers, between 0 and 1.
     Default 0.2.

  -  joy_outer_threshold_look - Outer deadzone for the look stick,
     between 0 and 1. Default 0.02.

  -  joy_outer_threshold_move - Outer deadzone for the move stick,
     between 0 and 1. Default 0.02.

  -  joy_sensitivity_yaw/pitch - Max angular speed in degrees/second
     when looking. Defaults are 240 for yaw (turning left/right) and 130
     for pitch (up/down).

  -  joy_exponent - For the look stick, the stick displacement (between
     0 and 1) is raised to this power. Default is 2. A value of 1 would
     give a linear relationship between stick displacement and fraction
     of the maximum angular speed.

  -  joy_exponent_move - Same as joy_exponent but for the move stick.
     Default is 2.

  -  joy_invert - Set to 1 to invert the vertical axis of the look
     stick.

  -  joy_swapmovelook - Set to 1 to swap the left and right analog stick
     functions. Default is 0, move on the left stick, look on the right
     stick.

  -  joy_enable - Set to 0 to disable controller support. Default is 1.

  Controller Support - Buttons:

  Some of the controller buttons are hardcoded to allow navigating the
  menu:

  -  Back - alias for TAB
  -  Start - alias for ESC
  -  DPad, analog sticks - mapped to arrow keys
  -  A Button - alias for ENTER in menus
  -  B Button - alias for ESC in menus

  These buttons can be bound normally:

  -  LTRIGGER - Left trigger
  -  RTRIGGER - Right trigger
  -  LSHOULDER - Left shoulder button
  -  RSHOULDER - Right shoulder button
  -  LTHUMB - Clicking the left thumbstick
  -  RTHUMB - Clicking the right thumbstick
  -  ABUTTON
  -  BBUTTON
  -  XBUTTON
  -  YBUTTON

  quakespasm.pak contains a default.cfg which has been updated to give
  some default bindings. L/R shoulder buttons are bound to weapon
  switching, and L/R triggers are jump and attack.

  The controller support started as Jeremiah Sypult's implementation in
  Quakespasm-Rift, and also uses ideas/code from LordHavoc's DarkPlaces.


  4. Compiling and Installation

  See the Downloads section to get the sourcecode, then below for
  platform specific instructions.  Quakespasm can also be built with the
  cross-platform Codeblocks.
  Quakespasm's (optional) custom data is now stored in the file
  quakespasm.pak. This file should be placed alongside your quakespasm
  binary and id1 directory.

  Compiling for Linux/Unix:

  After extracting the source tarball, "cd Quake" and edit the Makefile
  for music options.

  Streaming music playback requires "libmad" or "libmpg123" for MP3, and
  "libogg" and "libvorbis" for OGG files.

  Then type "make" to compile the program, and copy the "quakespasm"
  binary (and "quakespasm.pak") to your Quake game directory.
  Compile time options include

  -  make DO_USERDIRS=1 to include user directories support

  -  make DEBUG=1 for debugging

  -  make SDL_CONFIG=/PATH/TO/sdl-config for unusual SDL installations

  -  make USE_SDL2=1 to compile against SDL2 instead of SDL-1.2

  Compiling for Windows:

  QuakeSpasm developers cross-compile windows binaries using MinGW-w64
  <http://mingw-w64.sf.net> and MinGW <https://mingw.osdn.io>.  The
  project can also be built using Microsoft Visual Studio 2005 and newer
  versions.

  Compiling for Mac OS X:

  A Quakespasm App (including program launcher and update framework) can
  be made using the Xcode template found in the MacOSX directory.

  Alternatively, have a look at Makefile.darwin for more instructions on
  building from a console.


  Quake '2021 re-release':

  QuakeSpasm 0.94 has support for playing the 2021 re-release content:
  Copy the quakespasm binary to your rerelease installation and run as
  you normally do.


  5. Changes

  Changes in 0.96.3:

  -  Fix potential infinite loop when playing sounds with loop start >=
     end (e.g. misc/forcefield.wav from Madfox's kaptlog.zip)

  -  Fix possible overflow when resampling very big sounds (e.g.
     gram1.wav from the 'A day like no other' mod.)

  -  Change Sky_ClipPoly to allocate on the heap in if MAX_CLIP_VERTS
     limit is reached (e.g. with lim_daviddg from Liminal Spaces Jam)

  -  Minor code cleanups. Updates to third party code.

  -  Thanks to Andrei Drexler, Vincent Sonnier, Alexey Lysiuk, and
     Jaycie Ewald for patches.


  Changes in 0.96.2:

  -  Fix stack buffer overrun Mod_PolyForUnlitSurface: fixes crash when
     loading lim_daviddg.bsp from Liminal Spaces Jam.

  -  Fix potential buffer overflow in progs global string functions.

  -  Fix potential buffer overflow in Mod_LoadAliasFrame()

  -  Optimize BoxOnPlaneSide()

  -  Reserve enough space in PF_makestatic() for worst case scenarios.

  -  Improve handling of huge entities in SV_FindTouchedLeafs()

  -  Warn about bad maps with only 1 or 2 edges per face (e.g.: mj4m?,
     alk_dancing, ej3_bizz, rotj_entsoy...)

  -  Apply FOV gun scaling for r_showtris

  -  Sliders with visible values in options menu.

  -  game command now refuses setting a non-existing game directory

  -  Fix build in C23 mode.

  -  Minor code cleanups. Updated third party code, e.g. SDL, music
     codecs, etc.

  -  Thanks to Andrei Drexler, Alexey Lysiuk, Diordany van Hemert, and
     Jaime Moreira for patches.


  Changes in 0.96.1:

  -  Fix demo recording as client-only after connection to server (was
     broken by signon changes in 0.96.0. Thanks to Jozsef Szalontai for
     issue report.)

  -  Fix potential buffer overflow in COM_Parse(), e.g. with maps with
     oversized 'wad' fields. (Thanks to Andrei Drexler.)

  -  Minor code cleanups.


  Changes in 0.96.0:

  -  Adjustments to joystick defaults and behaviour.  See the
     "Controller support / Cvars" section above.

  -  Fix for a mouse grabbing issue on macOS.

  -  Add missing support for -ip command line option on unix / macOS.

  -  Backport angled sprites code from FTEQW (spriteframetype: 2, must
     have 8 frames per group.)

  -  Bump the progs execution runaway loop limit to 16M.

  -  Add signon buffer fragmentation to fix SZ_GetSpace errors on
     certain maps when using protocol 999.

  -  Increased MAX_DATAGRAM to 64000 to overcome packet overflows.

  -  Bump the MAXALIASFRAMES limit to 1024.

  -  Fix arrays to have proper sizes in gl mesh code, reflecting
     MAXALIASTRIS and MAXALIASVERTS correctly. (fixes SIGSEGV in
     mj4m4/mj4m5.)

  -  Increased default value of gl_farclip to 65536.

  -  Increase chase cam target trace distance to allow for chasecam to
     function in large open maps (useful e.g. for the func_vehicle mod.)

  -  Raised default maximum number of particles to 16384 (can be set
     up to 32768 with -particles on the command line.)

  -  Fix on-screen keyboard showing up suddenly on Steam Deck when
     starting a map.

  -  Fix viewmodel interpolation with >10Hz animations.

  -  Fix a memory leak when exiting game in SDL2 builds.

  -  Fix a possible crash when handling clipboard data on macOS.

  -  Fix a potential crash after loading of saved game.

  -  Fix possible out-of-bound reads when handling progs type sizes.

  -  Fix Dutch angle VP_PARALLEL_UPRIGHT sprites.

  -  Fix an issue with lights blending by using 10 bit color depth for
     lightmaps. (For GLSL mode only.) Disable with "-nopackedpixels",
     if necessary.

  -  No relative motions when the window is not focused.

  -  Status bar and intermission screen tweaks.

  -  Properly display monster counts > 3 digits.

  -  Fix console animation with scr_conspeed <= 0.

  -  Fix mouselook not working upon game quickload during a demo play.

  -  Haiku operating system support.

  -  Fix console when server is run noninteractively with stdin
     redirected from /dev/null.

  -  Updated Visual Studio project files.

  -  Minor code cleanups. Updated third party code, e.g. SDL, music
     codecs, etc.

  -  Thanks to Andrei Drexler, Alexey Lysiuk, Andrey Budko, Boris I.
     Bendovsky, Chris Cowan, Maciej Olędzki, Simon McVittie, OscarL,
     Eric Wasylishen, Jaycie Ewald and Spike for patches.


  Changes in 0.95.1:

  -  Allow mixing lit and unlit textures in the same map.

  -  Water texture mipmapping support - fixes glistening effect on
     distant water.

  -  Fix screen flashes in some maps with AMD's new drivers.

  -  Defend against zero-sized textures, e.g. __TB_empty in qbj_grue.bsp

  -  Fix possible file pointer leak in game load code.

  -  Minor code cleanups. Updated third party code, e.g. SDL, music
     codecs, etc.

  -  Thanks to 'temx' for his patches.


  Changes in 0.95.0:

  -  Add support for lit water (patch contributed by Josiah Jack, with
     fixes from Eric Wasylishen.)

  -  Add model scale support -- requires protocol 999. (Original patch
     contributed by Josiah Jack with fixes from temx and Andrei Drexler.
     Thanks to Eric Wasylishen, MH and Spike for useful discussions.)

  -  Add sv_cheats cvar for 2021 rerelease (patch from Andrei Drexler).

  -  Clear the startdemo list on game change (patch by Andrei Drexler).

  -  Try light trace from entity origin first (patch by Andrei Drexler).

  -  Backport a few fixes to the bundled SDL2-2.0.22 version.


  Changes in 0.94.7:

  -  Fix console history buffer wrapping.

  -  Fix wrong external texture use after vid_restart.

  -  Update lodepng from mainstream git.

  -  Miscellaneous source code cleanups.


  Changes in 0.94.6:

  -  Server protocol size check fixes for sounds and particles.

  -  An invalid memory access fix in the jpg screenshot writer.

  -  Basic dependency tracking in Makefiles.

  -  Backported a few fixes to the bundled SDL2-2.0.22 version.

  -  Minor build fix for C++ compilers.

  -  Other small improvements elsewhere in the code.


  Changes in 0.94.5:

  -  Compatibility with new SDL2 versioning scheme.

  -  Revised min/max/clamp macros' usage.

  -  Fixed a potential undefined behavior in R_DrawAliasModel.

  -  Fixed parsing of the time argument of svc_fog server message. (it
     has been broken for more than 20 years and has never seem to have
     been used.)

  -  Other small improvements elsewhere in the code.

  -  Backported a few fixes to the bundled SDL2-2.0.22 version.


  Changes in 0.94.4:

  -  Fixed getting stuck with loading plaque upon attempting to load a
     bad save from the menu

  -  Fixed SZ_GetSpace server crashes, e.g. in ad_magna or ad_mountain

  -  Fixed intermission camera angles during demo playback

  -  Don't reset onground flag on every new server message

  -  Re-enabled title demos by default (turn off with cl_startdemos 0)

  -  Don't autoplay next demo after a timedemo

  -  Fixed bad parsing of cdtrack in hipdemo1 from Scourge of Armagon

  -  Fixed serverinfo off-by-one nummodels/numsounds for protocol 15

  -  Support for Quake 2021 update 3

  -  Implemented quad/pentagram dlights for Quake 2021 rerelease

  -  Implemented 2021 rerelease localsound builtin & svc_localsound (56)

  -  Several other improvements to Quake 2021 rerelease support

  -  Several other small fixes and improvements all over the code

  -  Updated third party code (SDL, lodepng, miniz, some of the codecs,
     etc.)

  -  Thanks to Andrei Drexler, 'atsb' and 'temx' for their several
     patches.


  Changes in 0.94.3:

  -  Handle sky textures with non-standard sizes and warn about them
     (e.g. ad_tears)

  -  Allow loading textures not aligned to 16 pixels and warn about them
     (eg. the horde2 map of Dimension of the Machine from 2021 rerelease
     update/2.)

  -  Updated included SDL2 to latest version 2.0.18.


  Changes in 0.94.2:

  -  2021 rerelease: Support for playing the latest update.

  -  2021 rerelease: Fix pitch black models in lit areas in DOTM.

  -  2021 rerelease: Fix black candles in the DOTM start map.

  -  2021 rerelease: Look for QuakeEX.kpf under userdir, too.


  Changes in 0.94.1:

  -  Fix lightmap issues after vkQuake surface mark/cull optimizations
     merge (sf.net bug/50)


  Changes in 0.94.0:

  -  Initial support for playing the 'Quake 2021 re-release' content
     (thanks to Andrei Drexler for bulk of the work, Guillaume Plourde
     for Q64 bsp format support.)

  -  Fix rendering bug when cl_bobcycle was set to zero (sf.net bug/41)

  -  Fixed buffer overflow with large char skybox names (sf.net bug/38)

  -  Fixed a missing MAXALIASFRAMES bounds check (sf.net bug/37)

  -  OpenGL: workaround Intel UHD 600 driver bug (sf.net bug/39)

  -  OpenGL: merged surface mark & cull optimizations from vkQuake.

  -  Compensate viewmodel distortion at fov > 90 (based on code from
     Qrack, thanks to Andrei Drexler for the patch.)

  -  Raised MAX_GLTEXTURES limit from 2048 to 4096 for now.

  -  Changed 'model has a skin taller than 480' error into a warning

  -  Reject lit files if they're the wrong size (eg hipnotic/start.bsp
     vs id1/start.lit or just a bsp that no longer has any coloured
     lits, etc)

  -  External ent files are now versioned using 4 digit crc of the
     original map's ents, like e1m1@c49d.ent, which is much safer. The
     old method (e.g. e1m1.ent) still works but isn't recommended.

  -  Support for external vis files.

  -  Save: remove CR/LFs from level name to avoids broken saves, e.g.
     with autumn_sp map.

  -  Music: improvements to mp3 tag detection / skipping.

  -  Music: umx loader fix for malformed / crafted files.

  -  New console command music_jump: Jump to given order in music, like
     Unreal's music change - only for module (tracker) music.

  -  Updated third-party libraries (music codecs, SDL, etc.) Other minor
     fixes, tidy-ups and protability tweaks.

  -  Source repository moved to git.


  Changes in 0.93.2:

  -  Lightmaps are now dynamically allocated (from QSS), and
     BLOCK_WIDTH/HEIGHT raised from 128 to 256.

  -  Fixed several uncheked limits that would crash the Slayer's
     Testaments mod (sf.net bug #33).

  -  Raised MAXALIASTRIS from 2048 to 4096, so that the Slayer's
     Testaments mod works.

  -  Fixed 'pants' and 'shirt' types so that those textures load
     correctly on platforms where char type is unsigned by default
     (sf.net bug #28).

  -  Windows audio: WASAPI-enabled SDL2 dlls function properly now.

  -  Update the third-party libraries. Other fixes/cleanups.


  Changes in 0.93.1:

  -  Fixed a fog regression which was introduced in 0.93.0.

  -  Fixed a crash (buffer overflow) with invalid vis data.

  -  Fixed buttons crushing players in 64-bit builds.

  -  Change controller movement to use cubic easing by default; added
     "joy_exponent_move" cvar.

  -  config.cfg is no longer written in case of a Sys_Error.

  -  Fixed Opus encoded cd tracks not being recognized as ripped tracks.

  -  Update the third-party libraries. Other fixes/cleanups.


  Changes in 0.93.0:

  -  Raise default "joy_deadzone_trigger" cvar to 0.2.

  -  Raise console buffer size to 1MB.

  -  Raise MAX_STATIC_ENTITIES from 512 to 4096.

  -  Raise MAX_STACK_DEPTH from 32 to 64.

  -  Raise command buffer size from 8K to 256K to support large configs.

  -  Remove MAX_EFRAGS and MAX_MAP_LEAFS limits.

  -  Remove "Loadgame buffer overflow" limit, which could happen when
     loading DP or QSS saves.

  -  Adjust "exceeds standard limit of" debug warnings to include the
     actual QS limit.

  -  Change "game" command to now exec quake.rc.

  -  Change "games" / "mods" commands to list all subdirectories.

  -  Restore vid_refreshrate from fitzquake-0.85 for SDL2 builds.

  -  Alpha-masked model support. (MF_HOLEY: 0x4000).

  -  Invalid skin index now draws skin 0 (WinQuake behaviour) instead of
     blue checkerboard.

  -  Change default screenshot format to png. The 'screenshot' command
     now supports optional format (tga, png or jpg) and quality (1-100)
     arguments.

  -  Revert "always run" changes from 0.85.9 and move the QuakeSpasm
     customizations to a new "cl_alwaysrun" cvar: Set to 1 in order to
     scale forward/side/up speed by "cl_movespeedkey" (usually 2), and
     to make "speedkey" act as "slowkey".

  -  Change the "always run" menu option to offer three choices:
     off        (cl_alwaysrun 0, cl_forwardspeed 200, cl_backspeed 200)
     vanilla    (cl_alwaysrun 0, cl_forwardspeed 400, cl_backspeed 400)
     quakespasm (cl_alwaysrun 1, cl_forwardspeed 200, cl_backspeed 200)

  -  New "r_scale" cvar. Set to 2, 3, or 4 to render the view at 1/2,
     1/3, or 1/4 resolution.

  -  New "r_viewmodel_quake" cvar. Set to 1 for WinQuake gun position
     (from MarkV).

  -  New "find" / "apropos" command, searches for commands/cvar names
     for the given substring (from Spike).

  -  New "randmap" command for loading a random map.

  -  New "gl_cshiftpercent_contents", "gl_cshiftpercent_damage",
     "gl_cshiftpercent_bonus", "gl_cshiftpercent_powerup" cvars for
     tuning the strength of specic view blends.

  -  GL2 renderer: use a GLSL shader for world faces. Fixes reports of
     integrated+discrete GPU laptops having inconsistent fog rendering.

  -  Fix macOS startup delay (avoid calling gethostbyname() for ".local"
     hostnames).

  -  Fix memory corruption in PF_lightstyle with out of bounds
     lightstyles.

  -  Fix crash in BoundPoly with polygons extending beyond +/-9999.

  -  Fix QS window to stay on the current monitor when changing video
     modes (SDL2 only).

  -  Fix possible freeze in SV_TouchLinks regardless of what QC does in
     the touch function.

  -  Fix for maps with empty strings for vector keys (e.g. "origin");
     don't read uninitialized memory.

  -  Support for Open Watcom compiler.

  -  Update the third-party libraries.


  Changes in 0.92.1:

  -  Fixed large menu scale factors (was broken in 0.92.0).

  -  Fixed PAUSE key (was broken in 0.92.0).

  -  Updated some of the third-party libraries.


  Changes in 0.92.0:

  -  SDL2 Game Controller support.

  -  Contrast support with new "contrast" cvar, behaving the same as
     MarkV. It may be a useful alternative to the existing gamma control
     for laptops in a bright environment, etc. Raising contrast gives
     less of a gray/washed out look than raising gamma, but at a
     disadvantage: colors near white get clipped to white.

  -  RMQ protocol (999) support, adapted from RMQEngine.

  -  New "-protocol x" command line option. Accepted values for 'x' are
     15 (NetQuake), 666 (FitzQuake, default), and 999 (RMQ).

  -  New "setpos" console command.

  -  New "vid_borderless" cvar for getting a borderless window.

  -  Increased MAX_MAP_LEAFS from 65535 to 70000 and MAX_LIGHTMAPS from
     256 to 512 in order to handle the oms3 map pack.

  -  Server edicts are now allocated using malloc instead of allocating
     on the hunk.

  -  gl_clear now defaults to 1.

  -  Fix items falling out of the world on oms3.bsp on SSE builds.

  -  Worked around an OSX 10.6 driver bug when using FSAA, which was
     leading to an unplayable HOM effect on the rest of the screen.

  -  Fix wrong trace endpoint from the tracepos console command.

  -  Updated some of the third-party libraries. Other fixes/clean-ups.


  Changes in 0.91.0:

  Bugfixes:

  -  Fix unwanted fog mode change upon video restart.

  -  Work around Intel 855 bug in status bar drawing with "r_oldwater 0"
     and "scr_sbaralpha 0".

  -  Fix an obscure GLSL bug where changing gamma would result in the
     screen turning to noise.

  -  Fix GLSL gamma causing the tiled screen border to turn white when
     "sizedown" is used.

  -  Fix an alias model VBO renderer bug where a model not precached
     during map start wouldn't be drawn.

  -  Fix the order of OpenGL context creation and window creation in
     SDL2 video.

  -  Fix a calling convention issue in windows DPI awareness function
     pointers.

  -  Fix a random texture recoloring after video mode change.

  -  Fix a liquid turning to garbage after several video mode changes
     and "r_oldwater 0".

  -  Fix a wrong alpha-sorting bug introduced in 0.90.1.

  -  Fix "flush" command not reloading mdl's from disk (bug introduced
     in 0.90.1).

  -  Prevent a possible buffer overflow in Cbuf_Execute (old Q1/Q2 bug).

  -  Prevent a possible vulnerability in MSG_ReadString (old Q1/Q2 bug).

  Visual improvements:

  -  New cvars r_lavaalpha, r_slimealpha, r_telealpha for fine-tuning
     specific liquid opacities (from DirectQ/RMQEngine, non-archived,
     default to 0), and new worldspawn keys _wateralpha, _lavaalpha,
     _slimealpha, _telealpha, _skyfog (unique to Quakespasm, similar to
     the behaviour of the "fog" worldspawn key).

  -  GLSL gamma is now supported on older hardware without NPOT
     extension.

  Interface improvements:

  -  New r_pos command to show player position.

  -  NaN detection in traceline with "developer 1" set now warns instead
     of errors.

  Code cleanup / Other:

  -  Update third-party libraries.

  Raised limits:

  -  Default max_edicts 8192 (was 2048) and no longer saved to
     config.cfg.

  -  Default heapsize 256 MB (was 64 MB).

  -  Default zone 4 MB (was 384 KB).

  -  Raised MAX_SFX to 1024 (was 512).


  Changes in 0.90.1:

  Bugfixes:

  -  Fix dynamic light artifact where changing lightmap are rendered one
     frame late (bug introduced in 0.90.0).

  -  Fix texture memory leak when changing video modes with SDL2.

  -  Fix a rare incorrect mdl lighting on 64-bit builds. (details here:
     https://forums.insideqc.com/viewtopic.php?f=3&t=5620)

  -  Fix fullbrights turning black after "kill" command (bug introduced
     in 0.90.0).

  -  Clear all fog values on map change to prevent colored fog carrying
     over to jam3_tronyn.bsp.

  -  Allow loading saves with } character in quoted strings, fixes issue
     with retrojam1_skacky.bsp.

  -  Fix viewmodel not lerping on extended-limit maps.

  -  Fix crash on out-of-bounds skin number.

  Performance:

  -  Use multithreaded OpenGL on OS X for better performance.

  -  New, faster mdl renderer using GLSL. Disable with "-noglslalias".

  Visual improvements:

  -  New gamma correction implementation using GLSL. Fixes all known
     gamma issues (affecting the full display, persisting after
     quitting, or darkening the screen on OS X). Disable with
     "-noglslgamma".

  -  Use high-quality water by default (r_oldwater 0).

  -  Shadows use stencil buffer to avoid overlapping artifacts (from
     MarkV.)

  -  r_noshadow_list cvar added (from MarkV.)

  Interface improvements:

  -  Support pausing demo playback with the "pause" command.

  -  Autocompletion for "game", "record", "playdemo".

  -  Experimental windowed fullscreen mode available with
     vid_desktopfullscreen 1 (only in SDL2 builds, takes effect upon
     entering fullscreen mode the next time.)

  -  Silence "exceeded standard limit" messages unless developer cvar is
     >= 1.

  -  Some spam moved from developer 1 to 2: "can't find tga/lit/ent",
     "trying to load ent", "bad chunk length", "meshing",
     "PR_AlocStringSlots: realloc'ing"

  Code cleanup:

  -  Clean up IDE project files to build on fresh systems.

  -  Update 3rd-party libraries.


  Changes in 0.90.0:

  -  Fix issues on Windows systems with DPI scaling.

  -  Unix/Mac user directories support. Disabled by default,
     'make DO_USERDIRS=1' to enable it.

  -  SDL2 support. Disabled by default, 'make USE_SDL2=1' to enable it.

  -  Revised keyboard input code.

  -  Revised/improved the 'game' command, i.e. on-the-fly mod changing.
     It now accepts an optional second argument for mission packs or
     quoth support i.e. -hipnotic, -rogue, or -quoth. For example, for
     WarpSpasm: "game warp -quoth"

  -  Command line: "-game {quoth/hipnotic/rogue}" is now treated the
     same as -quoth, -hipnotic, or -rogue.

  -  Console speed now resolution-independent.

  -  Disabled gl_zfix, which caused glitches and is undesirable for new
     maps. Replacement .ent files to fix z-fighting for several id1 maps
     added to quakespasm.pak.

  -  PF_VarString buffer bumped to 1024, avoids truncated centerprints
     from the 'In The Shadows' mod.

  -  Support for opengl non-power-of-two-textures extension (disable
     with command line: "-notexturenpot".)

  -  Support for OpenGL vertex buffer objects (VBO, OpenGL 1.5 or newer)
     for world and brush models (disable with command line: "-novbo".)

  -  Antialiasing (FSAA) support (command line: -fsaa x, where x can be
     0, 2, 4, 8).

  -  Fence textures support.

  -  Dynamic light speedup. Speedup loading of tga and pcx external
     images.

  -  Brush model drawing speedup.

  -  Support for BSP2 and 2PSB map formats.

  -  Support for Opus, FLAC, and tracker music (S3M, IT, UMX, etc.), as
     compile-time options.

  -  Music and sfx now mixed at 44100 Hz to avoid downsampling music.
     Low-pass filter applied to the sfx if -sndspeed is 11025 (the
     default), to preserve the same sound quality as 0.85.9.  New
     -mixspeed option sets the rate for mixing sfx and music, and output
     to the OS (default 44100), setting it to 11025 reverts to 0.85.9
     behaviour. New snd_filterquality cvar, value can be between 1
     (emulate OS X resampler) and 5 (emulate Windows resampler),
     controls the sound of the low-pass filter.

  -  Better Hor+ field of view (FOV) scaling behavior.

  -  Better cross-map demo playback support.

  -  Fix screenshots when screen width isn't a multiple of 4.

  -  Fix a lighting glitch due to floating point precision.

  -  Fix a looping sounds glitch.

  -  Fix a vulnerability in file extension handling. Tighten path
     handling safety.

  -  Initialize opengl with 24-bit depth buffer at 32 bpp.

  -  Reset all models upon gamedir changes. (Fixes failures with mods
     using custom content.)

  -  Fix broken behavior upon gamedir changes if -basedir is specified
     on the command line.

  -  NET_MAXMESSAGE and MAX_MSGLEN limits bumped to 64000.

  -  MAX_EFRAGS bumped to 4096, and MAX_CHANNELS to 1024.

  -  MAX_ENT_LEAFS bumped from 16 to 32 to work around disappearing or
     flickering brush models in some situations. Also, if an entity is
     visible from MAX_ENT_LEAFS or more leafs, we now always send it to
     the client.

  -  Fix cvar cycle command not working sometimes.

  -  Host_Error upon missing models. (Prevents segmentation faults.)

  -  Change sv_aim default value to 1 (i.e. turn off autoaim)

  -  Add 'prev' and 'next' keywords to the 'cd' command.

  -  Work around a linux cdrom issue (playback might not start for a
     while after a stop).

  -  Quakespasm content customization moved from engine-embedded into a
     new optional quakespasm.pak file.

  -  Version bumped to 0.90.0 (because Quakespasm has a decent life of
     it's own)

  -  Other fixes and clean-ups.


  Changes in 0.85.9:

  -  Fixes for several undefined behaviors in C code (gcc-4.8 support.)

  -  Implemented Hor+ style field of view (FOV) scaling, useful for
     widescreen resolutions. Configured by new cvar fov_adapt: set it to
     1 and your fov will be scaled automatically according to the
     resolution. Enabled by default.

  -  Adjusted string buffers for PR_ValueString and friends to fix
     crashes with excessively long global strings seen in some rude
     mods.

  -  Toned down warning messages from PF_VarString() a bit.

  -  Fixed Fitzquake's map existence check in changelevel (used to leak
     file handles which would end up in a Sys_Error() due to consuming
     all free handles if many maps reside not in pak files.)

  -  Fixes/cleanups in chat mode handling. Client no longer gets stuck
     in chat mode upon disconnect.

  -  Mouse grab/key_dest fixes and key cleanups.

  -  The "speedkey" now acts as "slowkey" when "always run" is on.

  -  Support for demo recording after connection to server. (thanks to
     Baker for a patch)

  -  Corner case fixes in COM_Parse() for quoted strings and support for
     C-style /*..*/ comments.

  -  Changed lightmaps to GL_RGBA instead of GL_RGB.

  -  Better parse for opengl extensions list (from quakeforge.)

  -  Vsync saving/loading fixes.

  -  Fixed pointfile loading.

  -  Multiple cleanups in gl_vidsdl.c.

  -  Opus music decoding support (as an optional patch only.)

  -  Several other minor fixes/cleanups.


  Changes in 0.85.8:

  -  Made Quake shareware 1.00 and 1.01 versions to be recognized
     properly.

  -  Fixed control-character handling in unicode mode. Keyboard input
     tweaks.

  -  Made the keypad keys to send separate key events in game mode.

  -  Text pasting support from OS clipboard to console. (windows and
     macosx.)

  -  Support for the Apple (Command) key on macosx.

  -  Fixed increased (more than 32) dynamic lights.

  -  Music playback: Made sure that the file's channels count is
     supported.

  -  Support for Solaris.

  -  Switched to using libmad instead of libmpg123 for MP3 playback on
     Mac OS X.

  -  Better support for building the Mac OS X version using a makefile,
     support for cross-compiling on Linux.

  -  Fixed a minor intermissions glitch.

  -  Increased string buffer size from 256 to 384 for PF_VarString to
     work around broken mods such as UQC.

  -  Restored original behavior for Quake registered version detection.

  -  Minor demo recording/playback tweaks.

  -  Minor tweaks to the scale menu option.

  -  unbindall before loading stored bindings (configurable by new cvar
     cfg_unbindall, enabled by default.)

  -  New icon.

  -  Miscellaneous source code cleanups.


  Changes in 0.85.7:

  -  Added support for cross-level demo playback

  -  gl_texturemode is reimplemented as a cvar with a callback and the
     setting is automatically saved to the config

  -  Fixed execution of external files without a newline at the end

  -  Reduced memory usage during reloading of textures

  -  Fixed compilation on GNU/kFreeBSD (Debian bug #657793)

  -  Fixed backspace key on Mac OS X

  -  Disable mouse acceleration in Mac OS X

  -  Worked around recursive calling of the anisotropic filter callback

  -  Console word wrap and long input line fixes

  -  Verified correct compilation by clang (using v3.0)

  -  Several other small changes mostly invisible to the end-user


  Changes in 0.85.6:

  -  More work for string buffer safety

  -  Reverted v0.85.5 change of not allowing deathmatch and coop cvars
     to be set at the same time (was reported for possibility of causing
     compatibility issues with mods)

  -  Several cleanups/changes in the cvar layer

  -  Minor SDL video fixes.


  Changes in 0.85.5:

  -  SDL input driver updated adding native keymap and dead key support
     to the console

  -  Fixed a crash in net play in maps with extended limits

  -  Verified successful compilation using gcc-4.6.x

  -  Added workaround against GL texture flicker (z fighting),
     controlled by new cvar 'gl_zfix'

  -  Read video variables early so that a vid_restart isn't necessary
     after init

  -  mlook and lookspring fixes

  -  Added support for loading external entity files, controlled by new
     cvar 'external_ents'

  -  Made mp3 playback to allocate system memory instead of zone

  -  Some updates to the progs interpreter code

  -  Fixed r_nolerp_list parsing code of fitzquake

  -  Made sure that deathmatch and coop are not set at the same time

  -  Several code updates from uHexen2 project, several code cleanups.


  Changes in 0.85.4:

  -  Implement music (OGG, MP3, WAV) playback

  -  A better fix for the infamous SV_TouchLinks problem, no more hard
     lockups with maps such as "whiteroom"

  -  Add support for mouse buttons 4 and 5

  -  Fix the "unalias" console command

  -  Restore the "screen size" menu item

  -  Fixed an erroneous protocol check in the server code

  -  Raised the default zone memory size to 384 kb

  -  Raised the default max_edicts from 1024 to 2048

  -  Revised lit file loading, the lit file must be from the same game
     directory as the map itself or from a searchpath with a higher
     priority

  -  Fixed rest of the compiler warnings

  -  Other minor sound and cdaudio updates


  Changes in 0.85.3:

  -  Fix the "-dedicated" option (thanks Oz) and add platform specific
     networking code (default) rather than SDL_net

  -  Much needed OSX framework stuff from Kristian

  -  Add a persistent history feature (thanks Baker)

  -  Add a slider for scr_sbaralpha, which now defaults to 0.95
     (slightly transparent, allowing for a nicer status bar)

  -  Allow player messages longer than 32 characters

  -  Sockaddr fix for FreeBSD/OSX/etc networking

  -  Connect status bar size to the scale slider

  -  Include an ISNAN (is not-a-number) fix to catch the occassional
     quake C bug giving traceline problems

  -  Enumerate options menus

  -  Add a "prev weapon" menu item (from Sander)

  -  Small fix to Sound Block/Unblock on win32

  -  Lots of code fixes (some from uhexen2)

  -  Sys_Error calls Host_Shutdown

  -  Added MS Visual Studio support

  -  Add a "-cd" option to let the CD Player work in dedicated mode, and
     some other CD tweaks.


  Changes in 0.85.2:

  -  Replace the old "Screen size" slider with a "Scale" slider

  -  Don't constantly open and close condebug log

  -  Heap of C clean-ups

  -  Fix mapname sorting

  -  Alias the "mods" command to "games"

  -  Block/Unblock sound upon focus loss/gain

  -  NAT (networking protocol) fix

  -  SDLNet_ResolveHost bug-fix allowing connection to ports other than
     26000

  -  Bumped array size of sv_main.c::localmodels from 5 to 6 fixing an
     old fitzquake-0.85 bug which used to cause segfaults depending on
     the compiler.

  -  Accept commandline options like "+connect ip:port"


  Changes in 0.85.1:

  -  64 bit CPU support

  -  Restructured SDL sound driver

  -  Custom conback

  -  Tweaked the command line completion and added a map/changelevel
     autocompletion function

  -  Alt+Enter toggles fullscreen

  -  Disable Draw_BeginDisc which causes core dumps when called
     excessively

  -  Show helpful info on start-up

  -  Include real map name (sv.name) and skill in the status bar

  -  Remove confirm quit dialog

  -  Don't spam the console with PackFile seek requests

  -  Default to window mode

  -  Withdraw console when playing demos

  -  Don't play demos on program init

  -  Default heapsize is 64 MB.

  -  Changes to default console alpha, speed

  -  Changes to cvar persistence gl_flashblend (default 0), r_shadow,
     r_wateralpha, r_dynamic, r_novis.


  6. Copyright

  -  Quake and Quakespasm are released under the GNU GENERAL PUBLIC
     LICENSE Version 2: http://www.gnu.org/licenses/gpl-2.0.html
  -  Quakespasm console background image by AAS, released under the
     CREATIVE COMMONS PUBLIC LICENSE:
     http://creativecommons.org/licenses/by/3.0/legalcode


  7. Contact

  -  QuakeSpasm Project page:
     https://sourceforge.net/projects/quakespasm/
  -  Bug reports:
     https://sourceforge.net/p/quakespasm/bugs/


  8. Links

  -  QuakeSpasm Homepage: http://quakespasm.sourceforge.net
  -  Downloads: http://quakespasm.sourceforge.net/download.htm
  -  FitzQuake Homepage: https://celephais.net/fitzquake/
  -  Func Quakespasm forum:
     https://celephais.net/board/view_thread.php?id=60452
  -  Inside3D forums: https://forums.insideqc.com
