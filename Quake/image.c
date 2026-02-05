/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
//image.c -- image loading

#include "quakedef.h"

static byte *Image_LoadPCX (qfshandle_t *f, int *width, int *height);
static byte *Image_LoadLMP (qfshandle_t *f, int *width, int *height);

#ifdef __GNUC__
	// Suppress unused function warnings on GCC/clang
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_LINEAR
#include "stb_image.h"

#ifdef __GNUC__
	// Restore unused function warnings on GCC/clang
	#pragma GCC diagnostic pop
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "stb_image_write.h"

#define LODEPNG_NO_COMPILE_DECODER
#define LODEPNG_NO_COMPILE_CPP
#define LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS
#define LODEPNG_NO_COMPILE_ERROR_TEXT
#include "lodepng.h"
#include "lodepng.c"

static char loadfilename[MAX_OSPATH]; //file scope so that error messages can use it

typedef struct io_buffer_s {
	qfshandle_t *f;
	unsigned char buffer[1024];
	size_t size;
	size_t pos;
} io_buffer_t;

static io_buffer_t *Buf_Alloc(qfshandle_t *f)
{
	io_buffer_t *buf = (io_buffer_t *) calloc(1, sizeof(io_buffer_t));
	if (!buf)
		Sys_Error ("Buf_Alloc: out of memory");
	buf->f = f;
	return buf;
}

static void Buf_Free(io_buffer_t *buf)
{
	free(buf);
}

static inline int Buf_GetC(io_buffer_t *buf)
{
	if (buf->pos >= buf->size)
	{
		buf->size = QFS_ReadFile(buf->f, buf->buffer, sizeof(buf->buffer));
		buf->pos = 0;
		
		if (buf->size == 0)
			return EOF;
	}

	return buf->buffer[buf->pos++];
}

/*
Callback functions that stb can use to read from QFS files
*/

static int STBCB_Read(void *f, char *data, int size)
{
	return (int)QFS_ReadFile((qfshandle_t*)f, data, (size_t)size);
}

static void STBCB_Skip(void *f, int n)
{
	QFS_Seek((qfshandle_t*)f, (qfileofs_t)n, SEEK_CUR);
}

static int STBCB_Eof(void* f)
{
	return QFS_Eof((qfshandle_t*)f) ? 1 : 0;
}

/*
============
Image_LoadImage

returns a pointer to hunk allocated RGBA data
============
*/
byte *Image_LoadImage (const char *name, int *width, int *height, enum srcformat *fmt)
{
	static const char *const stbi_formats[] = {"png", "tga", "jpg", NULL};
	qfshandle_t	*f;
	stbi_io_callbacks callbacks;
	int		i;

	for (i = 0; stbi_formats[i]; i++)
	{
		const char *ext = stbi_formats[i];
		q_snprintf (loadfilename, sizeof(loadfilename), "%s.%s", name, ext);
		f = QFS_FOpenFile (loadfilename, NULL);
		if (f)
		{
			callbacks.read = &STBCB_Read;
			callbacks.skip = &STBCB_Skip;
			callbacks.eof = &STBCB_Eof;

			byte *data = stbi_load_from_callbacks (&callbacks, f, width, height, NULL, 4);
			if (data)
			{
				int numbytes = (*width) * (*height) * 4;
				byte *hunkdata = (byte *) Hunk_AllocNameNoFill (numbytes, ext);
				memcpy (hunkdata, data, numbytes);
				free (data);
				data = hunkdata;
				*fmt = SRC_RGBA;
				if ((developer.value || map_checks.value) && strcmp (ext, "tga") != 0)
					Con_Warning ("%s not supported by QS, consider tga\n", loadfilename);
			}
			else
				Con_Warning ("couldn't load %s (%s)\n", loadfilename, stbi_failure_reason ());
			QFS_CloseFile (f);
			return data;
		}
	}

	q_snprintf (loadfilename, sizeof(loadfilename), "%s.pcx", name);
	f = QFS_FOpenFile (loadfilename, NULL);
	if (f)
	{
		*fmt = SRC_RGBA;
		return Image_LoadPCX(f, width, height);
	}

	q_snprintf (loadfilename, sizeof(loadfilename), "%s.lmp", name);
	f = QFS_FOpenFile (loadfilename, NULL);
	if (f)
	{
		*fmt = SRC_INDEXED;
		return Image_LoadLMP (f, width, height);
	}

	return NULL;
}

//==============================================================================
//
//  TGA
//
//==============================================================================

#define TARGAHEADERSIZE 18		/* size on disk */

/*
============
Image_WriteTGA -- writes RGB or RGBA data to a TGA file

returns true if successful

TODO: support BGRA and BGR formats (since opengl can return them, and we don't have to swap)
============
*/
qboolean Image_WriteTGA (const char *name, byte *data, int width, int height, int bpp, qboolean upsidedown)
{
	int		i, size, temp, bytes;
	char	pathname[MAX_OSPATH];
	byte	header[TARGAHEADERSIZE];
	FILE	*file;
	qboolean ret;

	q_snprintf (pathname, sizeof(pathname), "%s/%s", com_gamedir, name);
	file = Sys_fopen (pathname, "wb");
	if (!file)
		return false;

	Q_memset (header, 0, TARGAHEADERSIZE);
	header[2] = 2; // uncompressed type
	header[12] = width&255;
	header[13] = width>>8;
	header[14] = height&255;
	header[15] = height>>8;
	header[16] = bpp; // pixel size
	if (upsidedown)
		header[17] = 0x20; //upside-down attribute

	// swap red and blue bytes
	bytes = bpp/8;
	size = width*height*bytes;
	for (i=0; i<size; i+=bytes)
	{
		temp = data[i];
		data[i] = data[i+2];
		data[i+2] = temp;
	}

	ret =
		fwrite (header, TARGAHEADERSIZE, 1, file) == 1 &&
		fwrite (data, 1, size, file) == size
	;

	fclose (file);

	return ret;
}

//==============================================================================
//
//  PCX
//
//==============================================================================

typedef struct
{
    char			signature;
    char			version;
    char			encoding;
    char			bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hdpi,vdpi;
    byte			colortable[48];
    char			reserved;
    char			color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char			filler[58];
} pcxheader_t;

/*
============
Image_LoadPCX
============
*/
static byte *Image_LoadPCX (qfshandle_t *f, int *width, int *height)
{
	pcxheader_t	pcx;
	int			x, y, w, h, readbyte, runlength;
	byte		*p, *data;
	byte		palette[768];
	io_buffer_t  *buf;

	if (QFS_ReadFile(f, &pcx, sizeof(pcx)) != sizeof(pcx))
		Sys_Error ("Failed reading header from '%s'", loadfilename);

	pcx.xmin = (unsigned short)LittleShort (pcx.xmin);
	pcx.ymin = (unsigned short)LittleShort (pcx.ymin);
	pcx.xmax = (unsigned short)LittleShort (pcx.xmax);
	pcx.ymax = (unsigned short)LittleShort (pcx.ymax);
	pcx.bytes_per_line = (unsigned short)LittleShort (pcx.bytes_per_line);

	if (pcx.signature != 0x0A)
		Sys_Error ("'%s' is not a valid PCX file", loadfilename);

	if (pcx.version != 5)
		Sys_Error ("'%s' is version %i, should be 5", loadfilename, pcx.version);

	if (pcx.encoding != 1 || pcx.bits_per_pixel != 8 || pcx.color_planes != 1)
		Sys_Error ("'%s' has wrong encoding or bit depth", loadfilename);

	w = pcx.xmax - pcx.xmin + 1;
	h = pcx.ymax - pcx.ymin + 1;

	data = (byte *) Hunk_AllocNoFill ((w*h+1)*4); //+1 to allow reading padding byte on last line

	//load palette
	if (QFS_Seek (f, QFS_FileSize(f) - sizeof(palette), SEEK_SET) != 0
		|| QFS_ReadFile (f, palette, sizeof(palette)) != sizeof(palette))
	{
		Sys_Error ("Failed reading palette from '%s'", loadfilename);
	}

	//back to start of image data
	QFS_Seek (f, sizeof(pcx), SEEK_SET);

	buf = Buf_Alloc(f);

	for (y=0; y<h; y++)
	{
		p = data + y * w * 4;

		for (x=0; x<(pcx.bytes_per_line); ) //read the extra padding byte if necessary
		{
			readbyte = Buf_GetC(buf);

			if(readbyte >= 0xC0)
			{
				runlength = readbyte & 0x3F;
				readbyte = Buf_GetC(buf);
			}
			else
				runlength = 1;

			while(runlength--)
			{
				p[0] = palette[readbyte*3];
				p[1] = palette[readbyte*3+1];
				p[2] = palette[readbyte*3+2];
				p[3] = 255;
				p += 4;
				x++;
			}
		}
	}

	Buf_Free(buf);
	QFS_CloseFile(f);

	*width = w;
	*height = h;
	return data;
}

//==============================================================================
//
//  QPIC (aka '.lmp')
//
//==============================================================================

typedef struct
{
	unsigned int width, height;
} lmpheader_t;

/*
============
Image_LoadLMP
============
*/
static byte *Image_LoadLMP (qfshandle_t *f, int *width, int *height)
{
	lmpheader_t	qpic;
	size_t		pix;
	int			mark;
	void		*data;

	if (QFS_ReadFile (f, &qpic, sizeof(qpic)) != sizeof(qpic))
	{
		QFS_CloseFile (f);
		return NULL;
	}
	qpic.width = LittleLong (qpic.width);
	qpic.height = LittleLong (qpic.height);

	pix = qpic.width*qpic.height;

	if (QFS_FileSize(f) != (qfileofs_t)(sizeof (qpic) + pix))
	{
		QFS_CloseFile (f);
		return NULL;
	}

	mark = Hunk_LowMark ();
	data = (byte *) Hunk_AllocNoFill (pix);
	if (QFS_ReadFile (f, data, pix) != pix)
	{
		Hunk_FreeToLowMark (mark);
		QFS_CloseFile (f);
		return NULL;
	}
	QFS_CloseFile (f);

	*width = qpic.width;
	*height = qpic.height;

	return data;
}

//==============================================================================
//
//  STB_IMAGE_WRITE
//
//==============================================================================

byte* Image_CopyFlipped (const void *src, int width, int height, int bpp)
{
	int			y, rowsize;
	const byte	*data = (const byte *)src;
	byte		*flipped;

	rowsize = width * (bpp / 8);
	flipped = (byte *) malloc(height * rowsize);
	if (!flipped)
		return NULL;

	for (y=0; y<height; y++)
	{
		memcpy(&flipped[y * rowsize], &data[(height - 1 - y) * rowsize], rowsize);
	}
	return flipped;
}

/*
============
Image_WriteJPG -- writes using stb_image_write

returns true if successful
============
*/
qboolean Image_WriteJPG (const char *name, byte *data, int width, int height, int bpp, int quality, qboolean upsidedown)
{
	unsigned error;
	char	pathname[MAX_OSPATH];
	byte	*flipped;
	int	bytes_per_pixel;

	if (!(bpp == 32 || bpp == 24))
		Sys_Error ("bpp not 24 or 32");

	bytes_per_pixel = bpp / 8;

	q_snprintf (pathname, sizeof(pathname), "%s/%s", com_gamedir, name);

	if (!upsidedown)
	{
		flipped = Image_CopyFlipped (data, width, height, bpp);
		if (!flipped)
			return false;
	}
	else
		flipped = data;

	error = stbi_write_jpg (pathname, width, height, bytes_per_pixel, flipped, quality);
	if (!upsidedown)
		free (flipped);

	return (error != 0);
}

qboolean Image_WritePNG (const char *name, byte *data, int width, int height, int bpp, qboolean upsidedown)
{
	unsigned error;
	char	pathname[MAX_OSPATH];
	byte	*flipped;
	unsigned char	*filters;
	unsigned char	*png;
	size_t		pngsize;
	LodePNGState	state;

	if (!(bpp == 32 || bpp == 24))
		Sys_Error("bpp not 24 or 32");

	q_snprintf (pathname, sizeof(pathname), "%s/%s", com_gamedir, name);

	flipped = (!upsidedown)? Image_CopyFlipped (data, width, height, bpp) : data;
	filters = (unsigned char *) malloc (height);
	if (!filters || !flipped)
	{
		if (!upsidedown)
		  free (flipped);
		free (filters);
		return false;
	}

// set some options for faster compression
	lodepng_state_init(&state);
	state.encoder.zlibsettings.use_lz77 = 0;
	state.encoder.auto_convert = 0;
	state.encoder.filter_strategy = LFS_PREDEFINED;
	memset(filters, 1, height); //use filter 1; see https://www.w3.org/TR/PNG-Filters.html
	state.encoder.predefined_filters = filters;

	if (bpp == 24)
	{
		state.info_raw.colortype = LCT_RGB;
		state.info_png.color.colortype = LCT_RGB;
	}
	else
	{
		state.info_raw.colortype = LCT_RGBA;
		state.info_png.color.colortype = LCT_RGBA;
	}

	error = lodepng_encode (&png, &pngsize, flipped, width, height, &state);
	if (error == 0)
		error = lodepng_save_file (png, pngsize, pathname);
#ifdef LODEPNG_COMPILE_ERROR_TEXT
	else Con_Printf("WritePNG: %s\n", lodepng_error_text (error));
#endif

	lodepng_state_cleanup (&state);
	lodepng_free (png); /* png was allocated by lodepng */
	free (filters);
	if (!upsidedown) {
	  free (flipped);
	}

	return (error == 0);
}
