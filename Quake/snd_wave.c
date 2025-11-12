/*
 * WAV streaming music support. Adapted from ioquake3 with changes.
 *
 * Copyright (C) 1999-2005 Id Software, Inc.
 * Copyright (C) 2005 Stuart Dalton <badcdev@gmail.com>
 * Copyright (C) 2010-2012 O.Sezer <sezero@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "quakedef.h"

#if defined(USE_CODEC_WAVE)
#include "snd_codec.h"
#include "snd_codeci.h"
#include "snd_wave.h"

/*
=================
FGetLittleLong
=================
*/
static int32_t FGetLittleLong (qfshandle_t *f, qboolean *ok)
{
	int32_t v;
	*ok &= QFS_ReadFile(f, &v, sizeof(v)) == sizeof(v);
	return LittleLong(v);
}

/*
=================
FGetLittleShort
=================
*/
static short FGetLittleShort(qfshandle_t *f, qboolean *ok)
{
	int16_t v;
	*ok &= QFS_ReadFile(f, &v, sizeof(v)) == sizeof(v);
	return LittleShort(v);
}

/*
=================
WAV_ReadChunkInfo
=================
*/
static int WAV_ReadChunkInfo(qfshandle_t *f, char *name)
{
	int len, r;
	qboolean ok = true;

	name[4] = 0;

	r = QFS_ReadFile(f, name, 4);
	if (r != 4)
		return -1;

	len = FGetLittleLong(f, &ok);
	if (!ok)
	{
		Con_Printf("WAV: couldn't read chunk length\n");
		return -1;
	}

	if (len < 0)
	{
		Con_Printf("WAV: Negative chunk length\n");
		return -1;
	}

	return len;
}

/*
=================
WAV_FindRIFFChunk

Returns the length of the data in the chunk, or -1 if not found
=================
*/
static int WAV_FindRIFFChunk(qfshandle_t *f, const char *chunk)
{
	char	name[5];
	int		len;

	while ((len = WAV_ReadChunkInfo(f, name)) >= 0)
	{
		/* If this is the right chunk, return */
		if (!strncmp(name, chunk, 4))
			return len;
		len = ((len + 1) & ~1);	/* pad by 2 . */

		/* Not the right chunk - skip it */
		QFS_Seek(f, len, SEEK_CUR);
	}

	return -1;
}

/*
=================
WAV_ReadRIFFHeader
=================
*/
static qboolean WAV_ReadRIFFHeader(const char *name, qfshandle_t *file, snd_info_t *info)
{
	char dump[16];
	int wav_format;
	int fmtlen = 0;
	qboolean ok = true;

	if (QFS_ReadFile(file, dump, 12) < 12 ||
	    strncmp(dump, "RIFF", 4) != 0 ||
	    strncmp(&dump[8], "WAVE", 4) != 0)
	{
		Con_Printf("%s is missing RIFF/WAVE chunks\n", name);
		return false;
	}

	/* Scan for the format chunk */
	if ((fmtlen = WAV_FindRIFFChunk(file, "fmt ")) < 0)
	{
		Con_Printf("%s is missing fmt chunk\n", name);
		return false;
	}

	/* Save the parameters */
	wav_format = FGetLittleShort(file, &ok);
	if (!ok || wav_format != WAV_FORMAT_PCM)
	{
		Con_Printf("%s is not Microsoft PCM format\n", name);
		return false;
	}

	info->channels = FGetLittleShort(file, &ok);
	info->rate = FGetLittleLong(file, &ok);
	FGetLittleLong(file, &ok);
	FGetLittleShort(file, &ok);
	info->bits = FGetLittleShort(file, &ok);
	if (!ok)
	{
		Con_Printf("%s is missing chunk info\n", name);
		return false;
	}

	if (info->bits != 8 && info->bits != 16)
	{
		Con_Printf("%s is not 8 or 16 bit\n", name);
		return false;
	}

	info->width = info->bits / 8;
	info->dataofs = 0;

	/* Skip the rest of the format chunk if required */
	if (fmtlen > 16)
	{
		fmtlen -= 16;
		QFS_Seek(file, fmtlen, SEEK_CUR);
	}

	/* Scan for the data chunk */
	if ((info->size = WAV_FindRIFFChunk(file, "data")) < 0)
	{
		Con_Printf("%s is missing data chunk\n", name);
		return false;
	}

	if (info->channels != 1 && info->channels != 2)
	{
		Con_Printf("Unsupported number of channels %d in %s\n",
						info->channels, name);
		return false;
	}
	info->samples = (info->size / info->width) / info->channels;
	if (info->samples == 0)
	{
		Con_Printf("%s has zero samples\n", name);
		return false;
	}

	return true;
}

/*
=================
S_WAV_CodecOpenStream
=================
*/
static qboolean S_WAV_CodecOpenStream(snd_stream_t *stream)
{
	/* Read the RIFF header */

	if (!WAV_ReadRIFFHeader(stream->name, stream->fh, &stream->info))
		return false;

	if (QFS_Tell(stream->fh) + stream->info.size > QFS_FileSize(stream->fh))
	{
		Con_Printf("%s data size mismatch\n", stream->name);
		return false;
	}

	return true;
}

/*
=================
S_WAV_CodecReadStream
=================
*/
int S_WAV_CodecReadStream(snd_stream_t *stream, int bytes, void *buffer)
{
	int remaining = stream->info.size - (int)QFS_Tell(stream->fh);
	int i, samples;

	if (remaining <= 0)
		return 0;
	if (bytes > remaining)
		bytes = remaining;

	if (QFS_ReadFile(stream->fh, buffer, bytes) != (qfileofs_t)bytes)
		Sys_Error ("S_WAV_CodecReadStream: read error on %d bytes (%s)", bytes, stream->name);
	if (stream->info.width == 2)
	{
		samples = bytes / 2;
		for (i = 0; i < samples; i++)
			((short *)buffer)[i] = LittleShort( ((short *)buffer)[i] );
	}
	return bytes;
}

static void S_WAV_CodecCloseStream (snd_stream_t *stream)
{
	S_CodecUtilClose(&stream);
}

static int S_WAV_CodecRewindStream (snd_stream_t *stream)
{
	QFS_Seek(stream->fh, 0, SEEK_SET);
	return 0;
}

static qboolean S_WAV_CodecInitialize (void)
{
	return true;
}

static void S_WAV_CodecShutdown (void)
{
}

snd_codec_t wav_codec =
{
	CODECTYPE_WAVE,
	true,	/* always available. */
	"wav",
	S_WAV_CodecInitialize,
	S_WAV_CodecShutdown,
	S_WAV_CodecOpenStream,
	S_WAV_CodecReadStream,
	S_WAV_CodecRewindStream,
	NULL, /* jump */
	S_WAV_CodecCloseStream,
	NULL
};

#endif	/* USE_CODEC_WAVE */

