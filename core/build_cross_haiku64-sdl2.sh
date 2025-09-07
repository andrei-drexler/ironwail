#!/bin/sh

# Change this script to meet your needs and/or environment.

TARGET=x86_64-unknown-haiku

MAKE_CMD=make

CC="$TARGET-gcc"
AS="$TARGET-as"
RANLIB="$TARGET-ranlib"
AR="$TARGET-ar"
STRIP="$TARGET-strip"
export CC AS AR RANLIB STRIP

exec $MAKE_CMD HOST_OS=haiku USE_SDL2=1 CC=$CC AS=$AS RANLIB=$RANLIB AR=$AR STRIP=$STRIP -f Makefile $*
