# GNU Makefile for Ironwail unix targets.
# You need the SDL library fully installed.
# "make DEBUG=1" to build a debug client.
# "make SDL_CONFIG=/path/to/sdl-config" for unusual SDL installations.
# "make DO_USERDIRS=1" to enable user directories support

# Enable/Disable user directories support
DO_USERDIRS=1

### Enable/Disable SDL2
USE_SDL2=1

### Enable/Disable Curl
USE_CURL=1

### Enable/Disable codecs for streaming music support
USE_CODEC_WAVE=1
USE_CODEC_FLAC=0
USE_CODEC_MP3=1
USE_CODEC_VORBIS=1
USE_CODEC_OPUS=0
# either xmp or mikmod (or modplug)
USE_CODEC_MIKMOD=0
USE_CODEC_XMP=0
USE_CODEC_MODPLUG=0
USE_CODEC_UMX=0

# which library to use for mp3 decoding: mad or mpg123
MP3LIB=mpg123
# which library to use for ogg decoding: vorbis or tremor
VORBISLIB=vorbis

# ---------------------------
# Helper functions
# ---------------------------

check_gcc = $(shell if echo | $(CC) $(1) -Werror -S -o /dev/null -xc - > /dev/null 2>&1; then echo "$(1)"; else echo "$(2)"; fi;)

# ---------------------------

HOST_OS = $(shell uname|sed -e s/_.*//|tr '[:upper:]' '[:lower:]')

DEBUG ?= 0

# ---------------------------
# build variables
# ---------------------------

CC ?= gcc
LINKER = $(CC)

STRIP ?= strip
PKG_CONFIG ?= pkg-config

CPUFLAGS=
LDFLAGS?=
DFLAGS ?=
CFLAGS ?= -Wall -Wno-trigraphs
CFLAGS += -D_FILE_OFFSET_BITS=64
CFLAGS += $(call check_gcc,-std=gnu11,)
CFLAGS += $(call check_gcc,-Werror=format,)
CFLAGS += $(CPUFLAGS)
ifneq ($(DEBUG),0)
DFLAGS += -DDEBUG
CFLAGS += -g
do_strip=
else
DFLAGS += -DNDEBUG
CFLAGS += -O2
#CFLAGS += -fno-asynchronous-unwind-tables
CFLAGS += $(call check_gcc,-fweb,)
CFLAGS += $(call check_gcc,-frename-registers,)
cmd_strip=$(STRIP) $(1)
define do_strip
	@echo Stripping $(1) && \
	$(call cmd_strip,$(1));
endef
endif

ifeq ($(DO_USERDIRS),1)
CFLAGS += -DDO_USERDIRS=1
endif

ifeq ($(USE_SDL2),1)
CFLAGS += -DUSE_SDL2
endif

ifeq ($(USE_SDL2),1)
SDL_CONFIG ?= sdl2-config
else
SDL_CONFIG ?= sdl-config
endif
SDL_CFLAGS = $(shell $(SDL_CONFIG) --cflags)
SDL_LIBS   = $(shell $(SDL_CONFIG) --libs)

NET_LIBS   =
ifeq ($(HOST_OS),sunos)
NET_LIBS   =-lsocket -lnsl -lresolv
endif
ifeq ($(HOST_OS),haiku)
NET_LIBS   =-lnetwork
endif

ifeq ($(USE_CURL),1)
NET_LIBS   += -lcurl
CFLAGS     += $(shell $(PKG_CONFIG) --cflags libcurl)
else
CFLAGS     += -DWITHOUT_CURL
endif

ifneq ($(VORBISLIB),vorbis)
ifneq ($(VORBISLIB),tremor)
$(error Invalid VORBISLIB setting)
endif
endif
ifneq ($(MP3LIB),mpg123)
ifneq ($(MP3LIB),mad)
$(error Invalid MP3LIB setting)
endif
endif
ifeq ($(MP3LIB),mad)
mp3_obj=snd_mp3
lib_mp3dec=-lmad
endif
ifeq ($(MP3LIB),mpg123)
mp3_obj=snd_mpg123
lib_mp3dec=-lmpg123
endif
ifeq ($(VORBISLIB),vorbis)
cpp_vorbisdec=
lib_vorbisdec=-lvorbisfile -lvorbis -logg
endif
ifeq ($(VORBISLIB),tremor)
cpp_vorbisdec=-DVORBIS_USE_TREMOR
lib_vorbisdec=-lvorbisidec -logg
endif

CODECLIBS  =
ifeq ($(USE_CODEC_WAVE),1)
CFLAGS+= -DUSE_CODEC_WAVE
endif
ifeq ($(USE_CODEC_FLAC),1)
CFLAGS+= -DUSE_CODEC_FLAC
CODECLIBS+= -lFLAC
endif
ifeq ($(USE_CODEC_OPUS),1)
# opus and opusfile put their *.h under <includedir>/opus,
# but they include the headers without the opus directory
# prefix and rely on pkg-config. ewww...
CFLAGS+= -DUSE_CODEC_OPUS
CFLAGS+= $(shell $(PKG_CONFIG) --cflags opusfile)
CODECLIBS+= $(shell $(PKG_CONFIG) --libs   opusfile)
endif
ifeq ($(USE_CODEC_VORBIS),1)
CFLAGS+= -DUSE_CODEC_VORBIS $(cpp_vorbisdec)
CODECLIBS+= $(lib_vorbisdec)
endif
ifeq ($(USE_CODEC_MP3),1)
CFLAGS+= -DUSE_CODEC_MP3
CODECLIBS+= $(lib_mp3dec)
endif
ifeq ($(USE_CODEC_MIKMOD),1)
CFLAGS+= -DUSE_CODEC_MIKMOD
CODECLIBS+= -lmikmod
endif
ifeq ($(USE_CODEC_XMP),1)
CFLAGS+= -DUSE_CODEC_XMP
CODECLIBS+= -lxmp
endif
ifeq ($(USE_CODEC_MODPLUG),1)
CFLAGS+= -DUSE_CODEC_MODPLUG
CODECLIBS+= -lmodplug
endif
ifeq ($(USE_CODEC_UMX),1)
CFLAGS+= -DUSE_CODEC_UMX
endif

ifeq ($(HOST_OS),haiku)
COMMON_LIBS= -lGL -ldl
else
COMMON_LIBS= -lGL -ldl -lm
endif

LIBS = $(COMMON_LIBS) $(NET_LIBS) $(CODECLIBS)

# name of this makefile, so we can use it as a dependency
MAKEFILE := $(lastword $(MAKEFILE_LIST))

# ---------------------------
# objects
# ---------------------------

MUSIC_OBJS= bgmusic.o \
	snd_codec.o \
	snd_flac.o \
	snd_wave.o \
	snd_vorbis.o \
	snd_opus.o \
	$(mp3_obj).o \
	snd_mp3tag.o \
	snd_mikmod.o \
	snd_modplug.o \
	snd_xmp.o \
	snd_umx.o
COMOBJ_SND = snd_dma.o snd_mix.o snd_mem.o $(MUSIC_OBJS)
SYSOBJ_SND = snd_sdl.o
SYSOBJ_CDA = cd_null.o
SYSOBJ_INPUT = in_sdl.o
SYSOBJ_GL_VID= gl_vidsdl.o
SYSOBJ_NET = net_bsd.o net_udp.o
SYSOBJ_SYS = pl_linux.o sys_sdl_unix.o
SYSOBJ_MAIN= main_sdl.o

GLOBJS = \
	gl_refrag.o \
	gl_rlight.o \
	gl_rmain.o \
	gl_fog.o \
	gl_rmisc.o \
	r_part.o \
	r_world.o \
	gl_screen.o \
	gl_shaders.o \
	gl_sky.o \
	gl_warp.o \
	$(SYSOBJ_GL_VID) \
	gl_draw.o \
	image.o \
	gl_texmgr.o \
	gl_mesh.o \
	r_sprite.o \
	r_alias.o \
	r_brush.o \
	gl_model.o

OBJS = strlcat.o \
	strlcpy.o \
	$(GLOBJS) \
	$(SYSOBJ_INPUT) \
	$(COMOBJ_SND) \
	$(SYSOBJ_SND) \
	$(SYSOBJ_CDA) \
	$(SYSOBJ_NET) \
	net_dgrm.o \
	net_loop.o \
	net_main.o \
	chase.o \
	cl_demo.o \
	cl_input.o \
	cl_main.o \
	cl_parse.o \
	cl_tent.o \
	console.o \
	keys.o \
	menu.o \
	sbar.o \
	view.o \
	wad.o \
	cmd.o \
	common.o \
	steam.o \
	json.o \
	miniz.o \
	crc.o \
	cvar.o \
	cfgfile.o \
	host.o \
	host_cmd.o \
	mathlib.o \
	pr_cmds.o \
	pr_edict.o \
	pr_exec.o \
	sv_main.o \
	sv_move.o \
	sv_phys.o \
	sv_user.o \
	world.o \
	zone.o \
	$(SYSOBJ_SYS) $(SYSOBJ_MAIN)

OBJDEPS := $(OBJS:%.o=%.d)
OBJS += $(SYSOBJ_RES)

# ---------------------------
# targets / rules
# ---------------------------

.PHONY:	clean debug release

DEFAULT_TARGET = ironwail
all: $(DEFAULT_TARGET)

%.d:	%.c $(MAKEFILE)
	@echo "Generating dependencies for $<" && \
	$(CC) $(DFLAGS) -c $(CFLAGS) $(SDL_CFLAGS) -MM $< -MF $@ -MT $@
%.o:	%.c %.d $(MAKEFILE)
	@echo "Compiling $<" && \
	$(CC) $(DFLAGS) -c $(CFLAGS) $(SDL_CFLAGS) -o $@ $<

ironwail:	$(OBJS) $(MAKEFILE)
	@echo "Linking $@" && \
	$(LINKER) $(OBJS) $(LDFLAGS) $(LIBS) $(SDL_LIBS) -o $@
	$(call do_strip,$@)

release:	ironwail
debug:
	$(error Use "make DEBUG=1")

clean: $(MAKEFILE)
	$(RM) *.o *.d $(DEFAULT_TARGET)

ifeq ($(HOST_OS),haiku)
IW_APP_DIR=$(shell finddir B_APPS_DIRECTORY)/ironwail/
install:	ironwail
	mkdir -p $(IW_APP_DIR)
	cp ironwail $(IW_APP_DIR)
	cp ironwail.pak $(IW_APP_DIR)
else
install:	ironwail
	cp ironwail /usr/local/games/quake
	cp ironwail.pak /usr/local/games/quake
endif

#---------------------------------------------------------------
# include dependencies (if not running 'clean' target)
#---------------------------------------------------------------

ifneq ($(strip $(MAKECMDGOALS)),clean)
ifneq ($(strip $(OBJDEPS)),)
-include $(OBJDEPS)
endif
endif
