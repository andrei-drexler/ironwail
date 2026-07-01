/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers
Copyright (C) 2024-2024 ironwail developers

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

/*
Implementation of the Quake File System, a virtual file system that can read
contents from pack files or file system files in the search directories.

Supported pack files are:

	*Regular Quake .pak files that the original Quake supports

	*Quake 3 .pk3 files (zip files with a different extension).

		This support is limited to either zip entries compressed with
		the regular DEFLATE method of zip files, or uncompressed entries.
*/
#include "quakedef.h"
#include "filesys.h"
#include "miniz.h"

typedef struct
{
	char	name[MAX_QPATH];
	int		filepos, filelen;
} packfile_t;

typedef struct file_handle_s
{
	void *data;
	void *impl_data;
	int fileno;
	qboolean owns_data;
	qboolean from_pak;
	qfileofs_t offs;
	qfileofs_t pak_offset;
	qfileofs_t start, endtrim;
	qfileofs_t (*filesize)(struct file_handle_s *handle);
	size_t (*read)(struct file_handle_s *handle, void* buf, size_t sz);
	void (*close)(struct file_handle_s *handle);
	int (*seek)(struct file_handle_s *handle, qfileofs_t pos);
} qfshandle_t;

typedef struct pack_s
{
	FILE* 	handle;
	char	filename[MAX_OSPATH];
	int		numfiles;
	packfile_t	*files;
	void* impl_data;
	int pakver;
	qfshandle_t* (*open_file)(struct pack_s* pack, int idx, qboolean reopen_pack);
} pack_t;
// on-disk pakfile
//
typedef struct
{
	char	name[56];
	int		filepos, filelen;
} dpackfile_t;

typedef struct
{
	char	id[4];
	int		dirofs;
	int		dirlen;
} dpackheader_t;

#define MAX_FILES_IN_PACK	2048
#define MAX_PACK_FILES 32

//Loaded pack files (.pak or .pk3)
//Index 0 is just a placeholder so 0 can be used to indicate error. First pack is loaded at index 1.
static pack_t* packs[1 + MAX_PACK_FILES] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

/*
=================
QFS_FreePackHandle

Close a pack file and free the data.
=================
*/
static void QFS_FreePackHandle (pack_t *pack)
{
	if (pack != NULL)
	{
		fclose (pack->handle);
		if (pack->impl_data)
		{
			if (pack->pakver == 3)
				mz_zip_reader_end ((mz_zip_archive*)pack->impl_data);
			free (pack->impl_data);
		}
		if (pack->files)
			free (pack->files);
		free (pack);
	}
}

/*
=================
QFS_RegisterPack

Add a pack to the program-wide array of packs.
Will return 0 if the array is already full and the pack
is closed and freed.
=================
*/
static int QFS_RegisterPack (pack_t *pack)
{
	for (size_t i = 1; i < countof(packs); ++i)
	{
		if (packs[i] == NULL)
		{
			packs[i] = pack;
			return (int)i;
		}
	}
	QFS_FreePackHandle (pack);

	Sys_Printf ("WARNING: Too many pack files loaded.");
	return 0;
}


static void QFS_CheckHandle (qfshandle_t* handle)
{
	if (handle == NULL || handle->data == NULL)
		Sys_Error ("Attempting to read from invalid file handle");
}

static void* QFS_Alloc (size_t sz)
{
	void *palloc = calloc (1, sz);
	if (palloc == NULL)
		Sys_Error ("QFS_Alloc of size %" SDL_PRIu64 " failed.", (uint64_t)sz);
	
	return palloc;
}

static qfshandle_t* QFS_AllocHandle (void)
{
	return (qfshandle_t *)QFS_Alloc (sizeof(qfshandle_t));
}

/*============
FS_*
file_handle_t implementation functions for regular disk files
============
*/
static void FS_Close (qfshandle_t* handle)
{
	if (handle)
	{
		if (handle->owns_data)
			fclose ((FILE*)handle->data);
		free (handle);
	}
}

static size_t FS_Read(qfshandle_t* handle, void* buf, size_t sz)
{
	QFS_CheckHandle (handle);
	
	sz = fread (buf, 1, sz, (FILE*)handle->data);
	handle->offs += sz;
	return sz;
}

static qfileofs_t FS_FileSize (qfshandle_t* handle)
{
	return Sys_filelength ((FILE*)handle->data);
}

int FS_Seek(qfshandle_t* handle, qfileofs_t pos)
{
	return Sys_fseek ((FILE*)handle->data, pos, SEEK_SET);
}

static qfshandle_t* FS_Open (const char* filename)
{
	FILE *stdio_handle = fopen (filename, "rb");
	if (stdio_handle == NULL)
		return NULL;

	qfshandle_t* handle = QFS_AllocHandle ();
	if (handle)
	{
		handle->data = stdio_handle;
		handle->close = &FS_Close;
		handle->read = &FS_Read;
		handle->filesize = &FS_FileSize;
		handle->seek = &FS_Seek;
		handle->owns_data = true;
	}
	return handle;
}

/*
============
PAK_*
qfshandle_t implementation functions for PAK file content files
============
*/
static void PAK_Close (qfshandle_t* handle)
{
	if (handle)
	{
		if (handle->owns_data)
		{
			pack_t* pack = (pack_t*)handle->data;
			pack->files = NULL;
			QFS_FreePackHandle (pack);
		}
		if (handle->impl_data)
			free (handle->impl_data);
		free (handle);
	}
}

static size_t PAK_Read (qfshandle_t* handle, void* buf, size_t sz)
{
	pack_t* pack;
	qfileofs_t actualofs;
	size_t filesize;

	QFS_CheckHandle (handle);
	pack = (pack_t*)handle->data;
	actualofs = handle->offs + handle->start;
	
	filesize = (size_t)handle->filesize(handle);
	if (actualofs + sz > filesize)
		sz = filesize - (size_t)actualofs;
	
	if (sz > 0 && Sys_fseek (pack->handle, handle->pak_offset + handle->offs + handle->start, SEEK_SET) == 0)
	{
		sz = fread (buf, 1, sz, pack->handle);
		handle->offs += sz;
	}
	
	return sz;
}

static qfileofs_t PAK_FileSize (qfshandle_t* handle)
{
	pack_t* pack = (pack_t*)handle->data;
	return pack->files[handle->fileno].filelen;
}

int PAK_Seek(qfshandle_t* handle, qfileofs_t pos)
{
	handle->offs = pos;
	return 0;
}

static qfshandle_t* PAK_Open(pack_t* pack, int idx, qboolean reopen_pack)
{
	pack_t* refpack;
	qfshandle_t *handle = QFS_AllocHandle ();
	handle->fileno = idx;
	handle->close = &PAK_Close;
	handle->read = &PAK_Read;
	handle->filesize = &PAK_FileSize;
	handle->seek = &PAK_Seek;

	if (reopen_pack)
	{
		//This will create a shallow copy that doesn't copy all the file
		//entries, but references the original. This should be fine
		handle->owns_data = true;
		pack_t* newpack = (pack_t*)QFS_Alloc (sizeof(pack_t));
		*newpack = *pack;
		newpack->handle = fopen (pack->filename, "rb");
		if (newpack->handle == NULL)
			Sys_Error ("%s failed to reopen.", pack->filename);
		handle->data = newpack;
	}
	else
	{
		handle->data = (void*)pack;
	}

	//Position PAK at the selected file
	refpack = (pack_t*)handle->data;
	handle->pak_offset = refpack->files[idx].filepos;
	return handle;
}

/*
============
ZIP_*
qfshandle_t implementation functions for pk3 (zip) file content files
============
*/

typedef struct
{
	byte* inbuf;
	byte* outbuf;
	qfileofs_t foffs_in, foffs_out;
	size_t bsz_in, bsz_out;	//Max buffer sizes
	size_t readsz_in;	//Valid data in input buffer
	size_t p_out;	//Bytes already read from the out buffer
	size_t p_in;	//Bytes already consumed from input buffer
	size_t out_read_ptr;	//indicates how much of the out buffer has been read by user
	qboolean eof_flag;	//End reached on deflate stream
	mz_zip_archive_file_stat stat;
	tinfl_decompressor infl;
}
inflbuffers_t;

static size_t ZIP_LowLevelRead (void *opaque, mz_uint64 ofs, void *buf, size_t n)
{
	FILE *handle = (FILE*)opaque;

	if (ofs > LONG_MAX || Sys_fseek (handle, ofs, SEEK_SET) != 0)
		Sys_Error ("Invalid read of at offset %" SDL_PRIu64, (uint64_t)n);

	return fread (buf, 1, n, handle);
}

static void ZIP_Close (qfshandle_t* handle)
{
	inflbuffers_t* zip = (inflbuffers_t*)handle->impl_data;
	handle->impl_data = NULL;
	if (zip)
	{
		if (zip->inbuf)
			free (zip->inbuf);
		if (zip->outbuf)
			free (zip->outbuf);
		free (zip);
	}
	
	PAK_Close (handle);
}

static size_t ZIP_Read (qfshandle_t* handle, void* buf, size_t sz)
{
	inflbuffers_t *p = (inflbuffers_t*)handle->impl_data;
	pack_t *pack = (pack_t*)handle->data;
	mz_zip_archive *z = (mz_zip_archive*)pack->impl_data;
	
	if (p->stat.m_is_directory || p->stat.m_uncomp_size == 0 || sz == 0 )
	{
		return 0;
	}

	byte* outbuf = (byte*)buf;
	size_t rd = 0;

	tinfl_status status = TINFL_STATUS_DONE;
	for (;;)
	{
		if (p->p_out >= p->bsz_out || p->eof_flag)
		{
			size_t ncpy = q_min (p->p_out - p->out_read_ptr, sz - rd);
			if (outbuf != NULL)
			{
				memcpy (outbuf, p->outbuf + p->out_read_ptr, ncpy);
				outbuf += ncpy;
			}
			rd += ncpy;
			p->out_read_ptr += ncpy;

			handle->offs += ncpy;
			
			if (p->out_read_ptr >= p->p_out)
				p->out_read_ptr = p->p_out = 0;
			
			if (rd >= sz || (p->p_out == 0 && p->eof_flag))
				return rd;
		}

		if (p->p_in >= p->readsz_in)
		{
			size_t sz = (size_t)q_min ((qfileofs_t)p->bsz_in, (qfileofs_t)p->stat.m_comp_size - p->foffs_in);
			p->readsz_in = z->m_pRead (z->m_pIO_opaque,
				(mz_uint64)(handle->pak_offset + p->foffs_in),
				p->inbuf, sz);
			
			if (p->readsz_in != sz)
				Sys_Error ("File I/O error on %s", pack->filename);
			
			p->p_in = 0;
			p->foffs_in += p->readsz_in;
		}

		size_t szin = p->readsz_in - p->p_in, szout = p->bsz_out - p->p_out;

		status = tinfl_decompress (&p->infl,
				(mz_uint8*)p->inbuf + p->p_in, &szin,
				(mz_uint8*)p->outbuf,
				(mz_uint8*)p->outbuf + p->p_out, &szout,
				(qfileofs_t)p->stat.m_comp_size >= p->foffs_in ? TINFL_FLAG_HAS_MORE_INPUT : 0 | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
		
		p->p_in += szin;
		p->p_out += szout;
		p->foffs_out += szout;
		p->eof_flag = status == TINFL_STATUS_DONE;

		if (status < TINFL_STATUS_DONE)
			Sys_Error ("Failed to inflate %s in %s", p->stat.m_filename, pack->filename);
	}
}

int ZIP_Seek(qfshandle_t* handle, qfileofs_t pos)
{
	inflbuffers_t *p = (inflbuffers_t*)handle->impl_data;
	qfileofs_t buf_start = handle->offs - (qfileofs_t)p->out_read_ptr;

	if (pos >= buf_start && pos - buf_start <= (qfileofs_t)p->p_out)
	{
		//good, we're still inside our output buffer
		p->out_read_ptr = (size_t)(pos - buf_start);
		return 0;
	}
	else if (pos > buf_start + (qfileofs_t)p->p_out)
	{
		//We need to skip forward
		size_t skipcnt = (size_t)(pos - handle->offs);
		return ZIP_Read (handle, NULL, skipcnt) == skipcnt ? 0 : -1;
	}
	else
	{
		//Start from the beginning
		p->out_read_ptr = p->p_out = p->readsz_in = 0;
		p->foffs_out = p->foffs_in = handle->offs = 0;
		p->eof_flag = false;
		tinfl_init (&p->infl);
		return ZIP_Read (handle, NULL, (size_t)pos) == (size_t)pos ? 0 : -1;
	}
}

static qfshandle_t* ZIP_Open (pack_t* pack, int idx, qboolean reopen_pack)
{
	mz_zip_archive* za;
	pack_t* refpack = pack;
	uint32_t local_header1 = 0;
	uint16_t local_header2[2];
	qfshandle_t *handle = PAK_Open (pack, idx, reopen_pack);
	
	handle->close = &ZIP_Close;

	inflbuffers_t *zip = (inflbuffers_t*)QFS_Alloc (sizeof(inflbuffers_t));

	if (reopen_pack)
	{
		pack_t* newpack = (pack_t*)handle->data;	//PAK_Open created a clone for us
		za = (mz_zip_archive*)QFS_Alloc (sizeof (mz_zip_archive));
		za->m_pRead = &ZIP_LowLevelRead;
		za->m_pIO_opaque = newpack->handle;

		if (!mz_zip_reader_init (za, Sys_filelength (refpack->handle), 0))
			Sys_Error ("%s failed to reopen.", refpack->filename);
		newpack->impl_data = za;
		refpack = newpack;
	}

	za = (mz_zip_archive*)refpack->impl_data;
	
	mz_zip_reader_file_stat (za, refpack->files[idx].filepos, &zip->stat);
	if (!zip->stat.m_is_supported)
		Sys_Error("Unsupported zip file entry %s", refpack->files[idx].name);
	
	if (za->m_pRead(za->m_pIO_opaque, zip->stat.m_local_header_ofs, &local_header1, sizeof(local_header1)) != sizeof(local_header1)
		|| SDL_SwapLE32(local_header1) != 0x04034b50
		|| za->m_pRead(za->m_pIO_opaque, zip->stat.m_local_header_ofs + 26, &local_header2, sizeof(local_header2)) != sizeof(local_header2))
	{
		Sys_Error ("Truncated or corrupt directory entry in %s", refpack->filename);
	}
	
	handle->pak_offset = zip->stat.m_local_header_ofs + 30
		+ SDL_SwapLE16 (local_header2[0])
		+ SDL_SwapLE16 (local_header2[1]);
	
	if (handle->pak_offset + zip->stat.m_comp_size > za->m_archive_size)
		Sys_Error ("Truncated zip file %s", refpack->filename);

	if (zip->stat.m_method)
	{
		handle->impl_data = zip;

		zip->bsz_in = q_min ((size_t)zip->stat.m_comp_size, MZ_ZIP_MAX_IO_BUF_SIZE / 2);
		zip->inbuf = (byte*)malloc(zip->bsz_in);

		zip->bsz_out = MZ_ZIP_MAX_IO_BUF_SIZE;
		zip->outbuf = (byte*)malloc (zip->bsz_out);

		tinfl_init (&zip->infl);
		handle->read = &ZIP_Read;
		handle->seek = &ZIP_Seek;
	}
	else
	{
		//An uncompressed zip entry - we can just read with the
		//regular PAK read functions
		handle->read = &PAK_Read;
		handle->seek = &PAK_Seek;
		free (zip);
	}

	return handle;
}

/*
=================
QFS_GetPack

Get the pack with the specified number, or NULL of there is none.
If parameter unregister is set, it will be removed from the
program wide list and the caller becomes the new owner.
=================
*/
static pack_t* QFS_GetPack (int num, qboolean unregister)
{
	if (num > 0 && num < MAX_PACK_FILES)
	{
		pack_t* pack = packs[num];
		if (unregister)
			packs[num] = NULL;
		return pack;
	}
	return NULL;
}

const char* QFS_PackInfoName (int packid)
{
	pack_t* pack = QFS_GetPack (packid, false);
	return pack ? pack->filename : NULL;
}

int QFS_PackInfoNumFiles (int packid)
{
	pack_t* pack = QFS_GetPack (packid, false);
	return pack ? pack->numfiles : 0;
}

void QFS_FreePack (int packid)
{
	pack_t* pack = QFS_GetPack (packid, true);
	QFS_FreePackHandle (pack);
}

void QFS_Shutdown (void)
{
	for (size_t i = 1; i < countof(packs); ++i)
	{
		if (packs[i] != NULL)
		{
			QFS_FreePackHandle(packs[i]);
			packs[i] = NULL;
		}
	}
}

qfileofs_t QFS_PackInfoEntrySize (int packid, int idx)
{
	pack_t* pack = QFS_GetPack (packid, false);
	if (pack && idx >= 0 && idx < pack->numfiles)
		return pack->files[idx].filelen;

	return 0; 
}

const char* QFS_PackInfoEntryName (int packid, int idx)
{
	pack_t* pack = QFS_GetPack (packid, false);
	if (pack && idx >= 0 && idx < pack->numfiles)
		return pack->files[idx].name;
	
	return NULL;
}
/*
=================
QFS_LoadPAKFile -- johnfitz -- modified based on topaz's tutorial

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
static int QFS_LoadPAKFile (const char *packfile)
{
	dpackheader_t	header;
	int		i;
	packfile_t	*newfiles;
	int		numpackfiles;
	pack_t		*pack;
	FILE		*packhandle;
	dpackfile_t	info[MAX_FILES_IN_PACK];

	packhandle = fopen(packfile, "rb");
	if (packhandle == NULL)
		return 0;

	if (fread(&header, 1, sizeof(header), packhandle) != sizeof(header) ||
	    header.id[0] != 'P' || header.id[1] != 'A' || header.id[2] != 'C' || header.id[3] != 'K')
		Sys_Error ("%s is not a packfile", packfile);

	header.dirofs = (header.dirofs);
	header.dirlen = (header.dirlen);

	numpackfiles = header.dirlen / sizeof(dpackfile_t);

	if (header.dirlen < 0 || header.dirofs < 0)
	{
		Sys_Error ("Invalid packfile %s (dirlen: %i, dirofs: %i)",
					packfile, header.dirlen, header.dirofs);
	}
	if (!numpackfiles)
	{
		printf ("WARNING: %s has no files, ignored\n", packfile);
		fclose (packhandle);
		return 0;
	}
	if (numpackfiles > MAX_FILES_IN_PACK)
		Sys_Error ("%s has %i files", packfile, numpackfiles);

	newfiles = (packfile_t *) QFS_Alloc(numpackfiles * sizeof(packfile_t));

	fseek (packhandle, header.dirofs, SEEK_SET);
	if ((int)fread(info, 1, header.dirlen, packhandle) != header.dirlen)
		Sys_Error ("Error reading %s", packfile);

	// parse the directory
	for (i = 0; i < numpackfiles; i++)
	{
		q_strlcpy (newfiles[i].name, info[i].name, sizeof(newfiles[i].name));
		newfiles[i].filepos = (info[i].filepos);
		newfiles[i].filelen = (info[i].filelen);
	}

	pack = (pack_t *) QFS_Alloc(sizeof (pack_t));
	q_strlcpy (pack->filename, packfile, sizeof(pack->filename));
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;
	pack->open_file = &PAK_Open;
	pack->pakver = 1;

	return QFS_RegisterPack (pack);
}

static int QFS_LoadPK3File(const char *packfile)
{
	mz_zip_archive *pk3;
	FILE *pk3handle;
	mz_uint i, numpackfiles;
	packfile_t	*newfiles;
	
	pk3handle = fopen (packfile, "rb");
	if (!pk3handle)
		return 0;

	pk3 = (mz_zip_archive*)QFS_Alloc (sizeof(mz_zip_archive));
	pk3->m_pRead = ZIP_LowLevelRead;
	pk3->m_pIO_opaque = pk3handle;
	char buf[MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE];

	if (!mz_zip_reader_init (pk3, (mz_uint64)Sys_filelength (pk3handle), 0))
		Sys_Error("%s can not be opened as a .pk3 file.", packfile);
	
	mz_uint entrycount = pk3->m_total_files;
	if (!entrycount)
	{
		printf("WARNING: %s has no files, ignored\n", packfile);
		fclose(pk3handle);
		free(pk3);
		return 0;
	}
	
	newfiles = (packfile_t *)QFS_Alloc(entrycount * sizeof(packfile_t));

	for (i = 0, numpackfiles = 0; i < entrycount; ++i)
	{
		size_t len;
		mz_zip_archive_file_stat st;
		if (!mz_zip_reader_file_stat(pk3, i, &st))
			Sys_Error ("Failed to get status of %s in %s.", buf, packfile);

		if (!st.m_is_directory)
		{
			if (st.m_uncomp_size > INT_MAX)
				Sys_Error("File %s in %s is too large.", buf, packfile);
			
			len = (size_t)mz_zip_reader_get_filename (pk3, i, buf, countof(buf));
			if ((st.m_bit_flag & (1 << 11)) == 0 && !q_strascii (buf))
			{
				//A legacy encoding is used for the filename, by popular convention this is assumed to be IBM437 nowadays
				char convbuf[sizeof(buf) * 3];
				len = UTF8_FromIBM437 (convbuf, sizeof(convbuf), buf);
				if (len <= sizeof(buf))
					memcpy (buf, convbuf, len);
				
			}
			if (len >= sizeof(newfiles[numpackfiles].name))
				Sys_Error ("File name %s in %s exceeds maximum allowed length.", buf, packfile);

			newfiles[numpackfiles].filelen = (int)st.m_uncomp_size;
			newfiles[numpackfiles].filepos = (int)st.m_file_index;

			q_strlcpy (newfiles[numpackfiles].name, buf, sizeof(newfiles[numpackfiles].name));
			++numpackfiles;
		}
	}

	pack_t* pack = (pack_t*) QFS_Alloc(sizeof(pack_t));
	q_strlcpy(pack->filename, packfile, sizeof(pack->filename));
	pack->handle = pk3handle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;
	pack->open_file = &ZIP_Open;
	pack->impl_data = pk3;
	pack->pakver = 3;

	return QFS_RegisterPack (pack);
}

int QFS_LoadPackFile(const char *packfile)
{
	if (q_strcasecmp (COM_FileGetExtension (packfile), "pk3") == 0)
		return QFS_LoadPK3File (packfile);
	else
		return QFS_LoadPAKFile (packfile);		
}

/*
===========
QFS_FindFile

Finds the file in the search path.
Sets file to a new file handle, if reopen is true and file is in a pak,
a new handle to the pak will be opened.
If file is not set, it can be used for detecting a file's presence.
===========
*/
static qfileofs_t QFS_FindFile (const char *filename, qfshandle_t** file, qboolean reopen, unsigned int *path_id)
{
	searchpath_t	*search;
	char		netpath[MAX_OSPATH];
	pack_t		*pak;
	int			i;

	if (file)
		*file = NULL;
//
// search through the path, one element at a time
//
	for (search = com_searchpaths; search; search = search->next)
	{
		if (search->pack)	/* look through all the pak file elements */
		{
			pak = QFS_GetPack (search->pack, false);
			if (!pak)
				Sys_Error ("QFS_FindFile: invalid pack id.");

			for (i = 0; i < pak->numfiles; i++)
			{
				if (strcmp(pak->files[i].name, filename) != 0)
					continue;
				// found it!
				if (path_id)
					*path_id = search->path_id;

				if (file)
					*file = pak->open_file(pak, i, reopen);
				
				return pak->files[i].filelen;
			}
		}
		else	/* check a file in the directory tree */
		{
			if (!registered.value)
			{ /* if not a registered version, don't ever go beyond base */
				if ( strchr (filename, '/') || strchr (filename,'\\'))
					continue;
			}

			q_snprintf (netpath, sizeof(netpath), "%s/%s",search->filename, filename);
			if (! (Sys_FileType(netpath) & FS_ENT_FILE))
				continue;

			if (path_id)
				*path_id = search->path_id;
			
			if (file)
			{
				*file = FS_Open (netpath);
				if (*file == NULL)
					return -1;
				return Sys_filelength ((FILE*)(*file)->data);
			}
			else
			{
				return 0; /* dummy valid value for QFS_FileExists() */
			}
		}
	}

	if (developer.value)
	{
		const char *ext = COM_FileGetExtension (filename);

		if (strcmp(ext, "pcx") != 0 &&
			strcmp(ext, "tga") != 0 &&
			strcmp(ext, "lit") != 0 &&
			strcmp(ext, "png") != 0 &&
			strcmp(ext, "jpg") != 0 &&
			strcmp(ext, "lmp") != 0 &&
			// music formats
			strcmp (ext, "ogg") != 0 &&
			strcmp (ext, "opus") != 0 &&
			strcmp (ext, "flac") != 0 &&
			strcmp (ext, "wav") != 0 &&
			strcmp (ext, "it") != 0 &&
			strcmp (ext, "s3m") != 0 &&
			strcmp (ext, "xm") != 0 &&
			strcmp (ext, "mod") != 0 &&
			strcmp (ext, "umx") != 0 &&
			// alternate model formats
			strcmp(ext, "md5mesh") != 0 &&
			strcmp (ext, "md3") != 0 &&
			strcmp (ext, "skin") != 0 &&
			// optional map files
			strcmp(ext, "lit") != 0 &&
			strcmp(ext, "vis") != 0 &&
			strcmp(ext, "ent") != 0)
			
			Con_DPrintf ("FindFile: can't find %s\n", filename);
		else
			Con_DPrintf2 ("FindFile: can't find %s\n", filename);
	}

	if (file)
		*file = NULL;
		
	return -1;
}

typedef enum
{
	LOADFILE_HUNK,
	LOADFILE_MALLOC
} loadfile_alloc_t;

byte *QFS_LoadFile (const char *path, loadfile_alloc_t method, unsigned int *path_id, size_t* ldsize)
{
	byte	*buf;
	char	base[32];
	size_t	len, nread;

	buf = NULL;	// quiet compiler warning

// look for it in the filesystem or pack files
	qfshandle_t *h = QFS_OpenFile (path, path_id);
	if (h == NULL)
		return NULL;
	
	len = (size_t)QFS_FileSize (h);
	if (ldsize)
		*ldsize = len;

// extract the filename base name for hunk tag
	COM_FileBase (path, base, sizeof(base));

	switch (method)
	{
	case LOADFILE_HUNK:
		buf = (byte *) Hunk_AllocNameNoFill (len+1, base);
		break;
	case LOADFILE_MALLOC:
		buf = (byte *) malloc (len+1);
		break;
	default:
		Sys_Error ("QFS_LoadFile: bad usehunk");
	}

	if (!buf)
		Sys_Error ("QFS_LoadFile: not enough space for %s", path);

	((byte *)buf)[len] = 0;

	nread = QFS_ReadFile (h, buf, len);
	QFS_CloseFile (h);
	if (nread != len)
		Sys_Error ("QFS_LoadFile: Error reading %s", path);

	return buf;
}

byte *QFS_LoadHunkFile (const char *path, unsigned int *path_id, size_t* ldsize)
{
	return QFS_LoadFile (path, LOADFILE_HUNK, path_id, ldsize);
}

// returns malloc'd memory
byte *QFS_LoadMallocFile (const char *path, unsigned int *path_id, size_t* ldsize)
{
	return QFS_LoadFile (path, LOADFILE_MALLOC, path_id, ldsize);
}

qboolean QFS_FileExists (const char *filename, unsigned int *path_id)
{
	qfileofs_t ret = QFS_FindFile (filename, NULL, false, path_id);
	return (ret == -1) ? false : true;
}

qfshandle_t* QFS_OpenFile (const char *filename, unsigned int *path_id)
{
	qfshandle_t* handle;
	if (QFS_FindFile (filename, &handle, false, path_id) >= 0)
		return handle;
	return NULL;
}

qfshandle_t* QFS_FOpenFile (const char *filename, unsigned int *path_id)
{
	qfshandle_t* handle;
	if (QFS_FindFile (filename, &handle, true, path_id) >= 0)
		return handle;
	return NULL;
}

qboolean QFS_Eof (qfshandle_t* handle)
{
	if (!handle || handle->offs - handle->endtrim >= handle->filesize(handle) - handle->start)
		return true;
	return false;
}

size_t QFS_ReadFile(qfshandle_t* handle, void* buf, size_t size)
{
	if (handle)
	{
		qfileofs_t filesize = handle->filesize(handle);
		if (handle->offs + (qfileofs_t)size > filesize - handle->endtrim)
			size = (size_t)(filesize - handle->endtrim);

		return handle->read(handle, buf, size);
	}
	return 0;
}

qfileofs_t QFS_FileSize (qfshandle_t* handle)
{
	return handle ? handle->filesize(handle) - handle->start - handle->endtrim : 0;
}

void QFS_CloseFile (qfshandle_t *handle)
{
	if (handle)
		handle->close (handle);
}

qfileofs_t QFS_Seek (qfshandle_t* handle, qfileofs_t offs, int whence)
{
	qfileofs_t actual_pos;
	if (!handle)
		return -1;

	switch (whence)
	{
	case SEEK_SET:
		actual_pos = handle->start + offs;
		break;
	case SEEK_CUR:
		actual_pos = handle->start + handle->offs + offs;
		break;
	case SEEK_END:
		actual_pos = handle->filesize(handle) - handle->endtrim + offs;
		break;
	default:
		return -1;
	}
	if (actual_pos < handle->start || actual_pos > handle->filesize(handle) - (handle->start + handle->endtrim))
		return -1;
	
	if (handle->seek(handle, actual_pos) == 0)
	{
		handle->offs = actual_pos;
		return 0;
	}
	return -1;
}

qfileofs_t QFS_Tell (qfshandle_t* handle)
{
	return handle ? handle->offs - handle->start : 0;
}

qboolean QFS_IgnoreBytes (qfshandle_t* handle, qfileofs_t cut, int whence)
{
	if (handle)
	{
		qfileofs_t filesize = handle->filesize(handle);
		if (whence == SEEK_SET && cut <= filesize - handle->endtrim)
			handle->start = cut;
		else if (whence == SEEK_END && cut <= filesize - handle->start)
			handle->endtrim = cut;
		else if (whence == SEEK_SET && cut == 0)
			handle->endtrim = handle->start = 0;
		else
			return false;
		
		if (handle->offs < handle->start)
			return handle->seek (handle, handle->start) == 0;
		if (handle->offs > handle->start - handle->endtrim)
			return handle->seek (handle, filesize - handle->start - handle->endtrim) == 0;
	}
	return false;
}

char QFS_GetChar (qfshandle_t* handle, qboolean* eof_flag)
{
	char ch = '\0';
	if (QFS_ReadFile (handle, &ch, 1) == 1)
	{
		if (eof_flag)
			*eof_flag = false;
		return ch;
	}
	if (eof_flag)
		*eof_flag = true;
	return '\0';
}

size_t QFS_GetLine (qfshandle_t* handle, char *buf, size_t bufsz)
{
	size_t i, o;
	qboolean eof_flag = false;
	if (bufsz < 1)
		return 0;

	for (i = 0, o = 0; o < bufsz - 1; ++i)
	{
		char ch = QFS_GetChar (handle, &eof_flag);
		if (ch == '\n' || ch == '\0' || eof_flag)
			break;
		else if (ch != '\r')
			buf[o++] = ch;
	}

	buf[o] = '\0';
	return o;
}

