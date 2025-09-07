/*
 * snd_opus.c - Opus audio codec support
 *
 * This file is temporarily disabled due to missing opus dependencies.
 * The opus codec functionality is not available until proper libraries are installed.
 */

#include "quakedef.h"

#ifdef USE_CODEC_OPUS

#include "snd_codec.h"
#include "snd_codeci.h"
#include "snd_opus.h"

// All opus functionality temporarily disabled
// This codec will not work until opus libraries are properly installed

// Dummy functions to prevent crashes
static qboolean S_OPUS_CodecInitialize(void) {
	// Do nothing - opus not available
	return true;
}

static void S_OPUS_CodecShutdown(void) {
	// Do nothing - opus not available
}

// Dummy codec structure to prevent linker errors
snd_codec_t opus_codec = {
	CODECTYPE_OPUS,
	false,	/* not available due to missing dependencies */
	"opus",
	S_OPUS_CodecInitialize,
	S_OPUS_CodecShutdown,
	NULL, /* open */
	NULL, /* read */
	NULL, /* rewind */
	NULL, /* jump */
	NULL, /* close */
	NULL  /* next */
};

#endif  /* USE_CODEC_OPUS */