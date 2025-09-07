/*
 * Music metadata extraction for Carnifex Engine
 * Handles extraction of artist, title, and other metadata from music files
 *
 * Copyright (C) 2024 Carnifex Engine Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#ifndef _MUSIC_METADATA_H_
#define _MUSIC_METADATA_H_

typedef struct music_metadata_s
{
	char title[256];
	char artist[256];
	char album[256];
	char year[16];
	qboolean has_metadata;
} music_metadata_t;

/* Extract metadata from a music file */
qboolean Music_ExtractMetadata(const char *filename, music_metadata_t *metadata);

/* Display music info in log and optionally as toaster */
void Music_DisplayInfo(const music_metadata_t *metadata, const char *filename);

/* Initialize music metadata system */
void Music_MetadataInit(void);

/* Shutdown music metadata system */
void Music_MetadataShutdown(void);

/* Update toaster notification display */
void Music_UpdateToaster(void);

#endif /* _MUSIC_METADATA_H_ */
