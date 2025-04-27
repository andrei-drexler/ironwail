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
#ifndef QUAKE_FILESYSTEM_H
#define QUAKE_FILESYSTEM_H

//Opaque file handle that is used for Quake file system operations
typedef struct file_handle_s qfshandle_t;

/*
===========
QFS_OpenFile

Attempts to open the requested file, returns NULL if it is not found.
Filename never has a leading slash, but may contain directory walks

Files opened with this will all use the same FILE* for the pack file
===========
*/
qfshandle_t* QFS_OpenFile (const char* filename, unsigned int* path_id);

/*
===========
QFS_FOpenFile

If the requested file is inside a packfile, a new FILE* will be opened
into the file pack. This can be a good idea if the file is used from
another thread.
===========
*/
qfshandle_t* QFS_FOpenFile (const char *filename, unsigned int *path_id);


/*
===========
QFS_FileExists

Returns whether the file is found in the quake filesystem.
===========
*/
qboolean QFS_FileExists (const char *filename, unsigned int *path_id);

/*
============
QFS_CloseFile

Must be called when you are done with the file to free used resources
============
*/
void QFS_CloseFile (qfshandle_t *handle);

/*
============
QFS_ReadFile

Read binary data from the file, returns the number of bytes read from the file.
============
*/
size_t QFS_ReadFile (qfshandle_t* handle, void* buf, size_t size);

/*
============
QFS_Eof

Returns true if end of file has been reached on the handle
============
*/
qboolean QFS_Eof (qfshandle_t* handle);

/*
============
QFS_FileSize

Returns the total size, in bytes, of the opened file.
============
*/
qfileofs_t QFS_FileSize (qfshandle_t* handle);

/*
============
QFS_Seek

Move to a specific position in the file.
whence can be one of SEEK_SET, SEEK_CUR, or SEEK_END and works like fseek in C.
Like fseek, it returns 0 on success and -1 on failure.
Unlike fseek, errno is not set on failure.

When using .pk3 files this is a more expensive operation than .pak or regular files,
especially when seeking backwards. For optimal results it is recommended to store
already compressed music files without deflate compression inside pk3 files - in
these cases the seek will be as efficient as a regular .pak file.
============
*/
qfileofs_t QFS_Seek (qfshandle_t* handle, qfileofs_t offs, int whence);

/*
============
QFS_Tell

Determine the current seek position in the file.
============
*/
qfileofs_t QFS_Tell (qfshandle_t* handle);

/*
============
QFS_IgnoreBytes

Specify a number of bytes that the file length should be shortened with.
This could be useful for ignoring garbage at the end of a file such as id3 tags.

whence can be either SEEK_END or SEEK_SET, SEEK_END will cut off at the end
and SEEK_SET will cut off in the beginning.

If the current file position is inside the removed area, the file cursor
will be moved either to the beginning (SEEK_SET) or the end (SEEK_END)

If you specify whence as SEEK_CUR and cut as 0, the ignore effect will be reset.
============
*/
qboolean QFS_IgnoreBytes (qfshandle_t* handle, qfileofs_t cut, int whence);


// these procedures open a file using COM_FindFile and loads it into a proper
// buffer. the buffer is allocated with a total size of file size + 1. the
// procedures differ by their buffer allocation method.
// If you don't need the file size, you can give ldsize as NULL. 
byte *QFS_LoadHunkFile (const char *path, unsigned int *path_id, size_t* ldsize);
	// allocates the buffer on the hunk.
byte *QFS_LoadMallocFile (const char *path, unsigned int *path_id, size_t* ldsize);
	// allocates the buffer on the system mem (malloc).

/*
============
QFS_LoadPackFile

Load a pack file and return the id of the new pack
============
*/
int QFS_LoadPackFile (const char *packfile);

/*
============
QFS_FreePack

Close handle and release all resources associated with the pack
============
*/
void QFS_FreePack (int packid);

/*
============
QFS_Shutdown

Close all packs that are open
============
*/
void QFS_Shutdown (void);

/*
============
QFS_PackInfoName

Returns the pack filename of the pack with the id or NULL if the pack doesn't exist
============
*/
const char* QFS_PackInfoName (int packid);

/*
============
QFS_PackInfoName

Returns the number of files in the pack with the id or 0 if the pack doesn't exist
============
*/
int QFS_PackInfoNumFiles (int packid);

/*
============
QFS_PackInfoEntrySize

Returns the total size, in bytes, of specified file index in the pack or 0 if it does not exist
============
*/
qfileofs_t QFS_PackInfoEntrySize (int packid, int idx);

/*
============
QFS_PackInfoEntryName

Returns the file name, of the specified file index in the pack or NULL if it does not exist
============
*/
const char* QFS_PackInfoEntryName (int packid, int idx);

/*
============
QFS_GetChar

Reads a single text character from the file. It is the basis
for implementing other text mode read functions.
returns 0 and sets eof_flag if there are no characters to read
============
*/
char QFS_GetChar (qfshandle_t* handle, qboolean* eof_flag);

/*
============
QFS_GetLine

Reads a single line of text from the file. Returns the number of
characters copied to buf. This will skip '\r' always.
Newline '\n' is considered end of the line, '\n' will
not be copied to buf.

If buf is full before encountering '\n' the string will be truncated.
The buf will always be NUL-terminated so it can at most
extract bufsz-1 characters.
============
*/
size_t QFS_GetLine (qfshandle_t* handle, char *buf, size_t bufsz);

#endif 	/* QUAKE_FILESYSTEM_H */
