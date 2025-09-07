#!/bin/sh

# Change this script to meet your needs and/or environment.

TARGET=i586-pc-haiku

MAKE_CMD=make

CC="$TARGET-gcc"
AS="$TARGET-as"
RANLIB="$TARGET-ranlib"
AR="$TARGET-ar"
STRIP="$TARGET-strip"
LDFLAGS=-L/usr/lib32 # hack
export CC AS AR RANLIB STRIP LDFLAGS

exec $MAKE_CMD HOST_OS=haiku USE_SDL2=1 LDFLAGS=$LDFLAGS CC=$CC AS=$AS RANLIB=$RANLIB AR=$AR STRIP=$STRIP -f Makefile $*
