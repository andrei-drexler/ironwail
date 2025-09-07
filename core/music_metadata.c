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

#include "quakedef.h"
#include "music_metadata.h"

#define MAX_METADATA_STRING 256
#include "snd_codec.h"
#include "snd_codeci.h"

/* CVars for music info display */
cvar_t music_info_log = {"music_info_log", "1", CVAR_ARCHIVE};
cvar_t music_info_toaster = {"music_info_toaster", "1", CVAR_ARCHIVE};
cvar_t music_info_toaster_duration = {"music_info_toaster_duration", "5", CVAR_ARCHIVE};

/* Toaster notification system */
typedef struct music_toaster_s
{
	char message[512];
	char title[256];
	char artist[256];
	qboolean has_metadata;
	float start_time;
	float duration;
	qboolean active;
} music_toaster_t;

static music_toaster_t music_toaster = {0};

/* Forward declarations */
static qboolean ExtractMP3Metadata(const char *filename, music_metadata_t *metadata);
static qboolean ExtractOGGMetadata(const char *filename, music_metadata_t *metadata);
static qboolean ExtractFLACMetadata(const char *filename, music_metadata_t *metadata);
static void CleanString(char *str, int maxlen);

/*
=================
Music_MetadataInit
=================
*/
void Music_MetadataInit(void)
{
	Cvar_RegisterVariable(&music_info_log);
	Cvar_RegisterVariable(&music_info_toaster);
	Cvar_RegisterVariable(&music_info_toaster_duration);
	
	memset(&music_toaster, 0, sizeof(music_toaster));
}

/*
=================
Music_MetadataShutdown
=================
*/
void Music_MetadataShutdown(void)
{
	memset(&music_toaster, 0, sizeof(music_toaster));
}

/*
=================
CleanString
=================
*/
static void CleanString(char *str, int maxlen)
{
	int i, j;
	
	/* Remove null bytes and control characters */
	for (i = 0, j = 0; i < maxlen && str[i]; i++)
	{
		if (str[i] >= 32 && str[i] != 127) /* printable characters */
		{
			str[j++] = str[i];
		}
	}
	str[j] = '\0';
	
	/* Trim whitespace */
	while (j > 0 && (str[j-1] == ' ' || str[j-1] == '\t'))
	{
		str[--j] = '\0';
	}
}

/*
=================
ExtractMP3Metadata
=================
*/
static qboolean ExtractMP3Metadata(const char *filename, music_metadata_t *metadata)
{
	FILE *file;
	unsigned char buffer[128];
	long file_size;
	qboolean found_metadata = false;
	
	file = fopen(filename, "rb");
	if (!file)
		return false;
		
	/* Get file size */
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	
	/* Check for ID3v1 tag at the end */
	if (file_size >= 128)
	{
		fseek(file, -128, SEEK_END);
		if (fread(buffer, 1, 128, file) == 128)
		{
			if (memcmp(buffer, "TAG", 3) == 0)
			{
				/* Extract ID3v1 metadata */
				memcpy(metadata->title, buffer + 3, 30);
				metadata->title[30] = '\0';
				CleanString(metadata->title, 30);
				
				memcpy(metadata->artist, buffer + 33, 30);
				metadata->artist[30] = '\0';
				CleanString(metadata->artist, 30);
				
				memcpy(metadata->album, buffer + 63, 30);
				metadata->album[30] = '\0';
				CleanString(metadata->album, 30);
				
				memcpy(metadata->year, buffer + 93, 4);
				metadata->year[4] = '\0';
				CleanString(metadata->year, 4);
				
				found_metadata = true;
			}
		}
	}
	
	fclose(file);
	return found_metadata;
}

/*
=================
ExtractOGGMetadata
=================
*/
static qboolean ExtractOGGMetadata(const char *filename, music_metadata_t *metadata)
{
	FILE *file;
	unsigned char buffer[16384];  /* Larger buffer to find metadata */
	size_t bytes_read;
	int i, j;
	char full_path[256];

	/* Try to resolve the full path */
	if (filename[0] == '/')
	{
		/* Absolute path */
		q_strlcpy(full_path, filename, sizeof(full_path));
	}
	else
	{
		/* Relative path - try with game directory prefix */
		q_snprintf(full_path, sizeof(full_path), "carnifex-game/%s", filename);
	}

	file = fopen(full_path, "rb");
	if (!file)
	{
		/* Try the original filename */
		file = fopen(filename, "rb");
		if (!file)
		{
			return false;
		}
	}

	bytes_read = fread(buffer, 1, sizeof(buffer), file);
	fclose(file);

	if (bytes_read < 100)
	{
		return false;
	}
	
	/* Look for specific metadata strings directly */
	for (i = 0; i < (int)bytes_read - 20; i++)
	{
		/* Look for TITLE= */
		if (memcmp(buffer + i, "TITLE=", 6) == 0)
		{
			j = i + 6;
			int len = 0;
			while (j < (int)bytes_read && buffer[j] != 0 && len < MAX_METADATA_STRING - 1)
			{
				metadata->title[len++] = buffer[j++];
			}
			metadata->title[len] = '\0';
			CleanString(metadata->title, len);
			metadata->has_metadata = true;
		}
		/* Look for ARTIST= */
		else if (memcmp(buffer + i, "ARTIST=", 7) == 0)
		{
			j = i + 7;
			int len = 0;
			while (j < (int)bytes_read && buffer[j] != 0 && len < MAX_METADATA_STRING - 1)
			{
				metadata->artist[len++] = buffer[j++];
			}
			metadata->artist[len] = '\0';
			CleanString(metadata->artist, len);
			metadata->has_metadata = true;
		}
		/* Look for ALBUM= */
		else if (memcmp(buffer + i, "ALBUM=", 6) == 0)
		{
			j = i + 6;
			int len = 0;
			while (j < (int)bytes_read && buffer[j] != 0 && len < MAX_METADATA_STRING - 1)
			{
				metadata->album[len++] = buffer[j++];
			}
			metadata->album[len] = '\0';
			CleanString(metadata->album, len);
			metadata->has_metadata = true;
		}
		/* Look for DATE= */
		else if (memcmp(buffer + i, "DATE=", 5) == 0)
		{
			j = i + 5;
			int len = 0;
			while (j < (int)bytes_read && buffer[j] != 0 && len < MAX_METADATA_STRING - 1)
			{
				metadata->year[len++] = buffer[j++];
			}
			metadata->year[len] = '\0';
			CleanString(metadata->year, len);
			metadata->has_metadata = true;
		}
	}

	return metadata->has_metadata;
}

/*
=================
ExtractFLACMetadata
=================
*/
static qboolean ExtractFLACMetadata(const char *filename, music_metadata_t *metadata)
{
	/* FLAC metadata extraction would require libFLAC
	 * For now, return false to indicate no metadata found */
	return false;
}

/*
=================
Music_ExtractMetadata
=================
*/
qboolean Music_ExtractMetadata(const char *filename, music_metadata_t *metadata)
{
	const char *ext;
	qboolean found = false;
	
	/* Initialize metadata structure */
	memset(metadata, 0, sizeof(music_metadata_t));
	
	/* Get file extension */
	ext = COM_FileGetExtension(filename);
	if (!ext || !*ext)
		return false;
		
	/* Try to extract metadata based on file type */
	if (!q_strcasecmp(ext, "mp3"))
	{
		found = ExtractMP3Metadata(filename, metadata);
	}
	else if (!q_strcasecmp(ext, "ogg"))
	{
		found = ExtractOGGMetadata(filename, metadata);
	}
	else if (!q_strcasecmp(ext, "flac"))
	{
		found = ExtractFLACMetadata(filename, metadata);
	}
	
	metadata->has_metadata = found;
	return found;
}

/*
=================
Music_DisplayInfo
=================
*/
void Music_DisplayInfo(const music_metadata_t *metadata, const char *filename)
{
	char display_text[512];
	char log_text[512];
	
	if (!metadata)
		return;
		
	/* Create display text */
	if (metadata->has_metadata)
	{
		if (metadata->artist[0] && metadata->title[0])
		{
			q_snprintf(display_text, sizeof(display_text), "%s by %s", 
				metadata->title, metadata->artist);
		}
		else if (metadata->title[0])
		{
			q_strlcpy(display_text, metadata->title, sizeof(display_text));
		}
		else if (metadata->artist[0])
		{
			q_strlcpy(display_text, metadata->artist, sizeof(display_text));
		}
		else
		{
			/* Fallback to filename if no useful metadata */
			const char *basename = COM_SkipPath(filename);
			char name_without_ext[256];
			COM_StripExtension(basename, name_without_ext, sizeof(name_without_ext));
			q_strlcpy(display_text, name_without_ext, sizeof(display_text));
		}
	}
	else
	{
		/* No metadata found, use filename as fallback */
		const char *basename = COM_SkipPath(filename);
		char name_without_ext[256];
		COM_StripExtension(basename, name_without_ext, sizeof(name_without_ext));
		q_strlcpy(display_text, name_without_ext, sizeof(display_text));
	}
	
	/* Log to console if enabled */
	if (music_info_log.value)
	{
		q_snprintf(log_text, sizeof(log_text), "MUSIC: %s", display_text);
		Con_Printf("%s\n", log_text);
	}
	
	/* Show toaster notification if enabled */
	if (music_info_toaster.value)
	{
		/* For toaster, we need to store title and artist separately for colored display */
		if (metadata->has_metadata && metadata->title[0] && metadata->artist[0])
		{
			/* Store title and artist separately for colored display */
			q_strlcpy(music_toaster.title, metadata->title, sizeof(music_toaster.title));
			q_strlcpy(music_toaster.artist, metadata->artist, sizeof(music_toaster.artist));
			music_toaster.has_metadata = true;
		}
		else
		{
			/* Fallback to filename */
			q_strlcpy(music_toaster.message, display_text, sizeof(music_toaster.message));
			music_toaster.has_metadata = false;
		}
		music_toaster.start_time = realtime;
		music_toaster.duration = music_info_toaster_duration.value;
		music_toaster.active = true;
	}
}

/*
=================
Music_UpdateToaster
=================
*/
void Music_UpdateToaster(void)
{
	if (!music_toaster.active)
		return;
		
	/* Check if toaster should expire */
	if (realtime - music_toaster.start_time >= music_toaster.duration)
	{
		music_toaster.active = false;
		return;
	}
	
	/* Draw toaster notification */
	if (music_toaster.active)
	{
		int x, y;
		int title_width, artist_width, total_width;
		int title_x, artist_x;
		
		if (music_toaster.has_metadata)
		{
			/* Draw title and artist separately with different colors */
			title_width = strlen(music_toaster.title) * 8;
			artist_width = strlen(music_toaster.artist) * 8;
			total_width = title_width + artist_width + 16; /* 16 for " by " */
			
			/* Center the text horizontally */
			x = (vid.width - total_width) / 2;
			y = 100; /* Top middle of screen, moved down a bit */
			
			/* Draw background box */
			Draw_Fill(x - 8, y - 4, total_width + 16, 16, 0, 0.5f);
			
			/* Draw title in dark white */
			title_x = x;
			GL_PushCanvasColor(0.8f, 0.8f, 0.8f, 1.0f);
			Draw_String(title_x, y, music_toaster.title);
			GL_PopCanvasColor();
			
			/* Draw " by " in dark white */
			GL_PushCanvasColor(0.8f, 0.8f, 0.8f, 1.0f);
			Draw_String(title_x + title_width, y, " by ");
			GL_PopCanvasColor();
			
			/* Draw artist in light red */
			artist_x = title_x + title_width + 32; /* 32 for " by " */
			GL_PushCanvasColor(1.0f, 0.4f, 0.4f, 1.0f);
			Draw_String(artist_x, y, music_toaster.artist);
			GL_PopCanvasColor();
		}
		else
		{
			/* Fallback to filename display */
			int text_width = strlen(music_toaster.message) * 8;
			
			/* Center the text horizontally */
			x = (vid.width - text_width) / 2;
			y = 100; /* Top middle of screen, moved down a bit */
			
			/* Draw background box */
			Draw_Fill(x - 8, y - 4, text_width + 16, 16, 0, 0.5f);
			
			/* Draw filename in dark white */
			GL_PushCanvasColor(0.8f, 0.8f, 0.8f, 1.0f);
			Draw_String(x, y, music_toaster.message);
			GL_PopCanvasColor();
		}
	}
}
