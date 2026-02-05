/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2005 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
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

#include "quakedef.h"
#include "steam.h"

#ifndef MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS
#define MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS 0
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <mmsystem.h>
#include <winreg.h>
#include <versionhelpers.h>
#include <tlhelp32.h>

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4091) // 'typedef ': ignored on left of 'tagGPFIDL_FLAGS' when no variable is declared
#endif 
#include <shlobj.h>
#ifdef _MSC_VER
#pragma warning (pop)
#endif

#include <sys/types.h>
#include <errno.h>
#include <io.h>
#include <direct.h>

#if defined(SDL_FRAMEWORK) || defined(NO_SDL_CONFIG)
#include <SDL2/SDL.h>
#else
#include "SDL.h"
#endif

// Prefer running on dedicated GPUs to avoid bugs in ancient Intel drivers
__declspec(dllexport) DWORD NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

qboolean		isDedicated;

static HANDLE		hinput, houtput;

static double rcp_counter_freq;

static void UTF8ToWideString (const char *src, wchar_t *dst, size_t maxchars)
{
	if (!MultiByteToWideChar (CP_UTF8, 0, src, -1, dst, maxchars))
		Sys_Error ("MultiByteToWideChar failed: %lu", GetLastError ());
}

static void WideStringToUTF8 (const wchar_t *src, char *dst, size_t maxbytes)
{
	if (!WideCharToMultiByte (CP_UTF8, 0, src, -1, dst, maxbytes, NULL, NULL))
		Sys_Error ("WideCharToMultiByte failed: %lu", GetLastError ());
}

FILE *Sys_fopen (const char *path, const char *mode)
{
	wchar_t	wpath[MAX_PATH];
	wchar_t	wmode[8];
	int		i;
	FILE	*f;
	
	for (i = 0; mode[i]; i++)
	{
		if (i == countof (wmode) - 1)
			Sys_Error ("Sys_fopen: invalid mode \"%s\"", mode);
		wmode[i] = mode[i];
	}
	wmode[i] = 0;

	UTF8ToWideString (path, wpath, countof (wpath));

	if (wpath[0] && strchr (mode, 'w'))
	{
		// create directory structure
		for (i = 1; wpath[i]; i++)
		{
			DWORD attr;
			wchar_t wc;
			if (wpath[i] != L'\\' && wpath[i] != L'/')
				continue;

			// keep the trailing slash
			wc = wpath[i + 1];
			wpath[i + 1] = L'\0';

			attr = GetFileAttributesW (wpath);
			if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
				return NULL;

			if (attr == INVALID_FILE_ATTRIBUTES && !CreateDirectoryW (wpath, NULL))
			{
				DWORD err = GetLastError ();
				if (err != ERROR_ALREADY_EXISTS)
					return NULL;
			}

			wpath[i + 1] = wc;
		}
	}

	f = _wfopen (wpath, wmode);

	return f;
}

int Sys_fseek (FILE *file, qfileofs_t ofs, int origin)
{
	return _fseeki64 (file, ofs, origin);
}

qfileofs_t Sys_ftell (FILE *file)
{
	return _ftelli64 (file);
}

int Sys_remove (const char *path)
{
	wchar_t	wpath[MAX_PATH];
	UTF8ToWideString (path, wpath, countof (wpath));
	return _wremove (wpath);
}

int Sys_rename (const char *oldname, const char *newname)
{
	wchar_t	oldnamew[MAX_PATH];
	wchar_t	newnamew[MAX_PATH];
	UTF8ToWideString (oldname, oldnamew, countof (oldnamew));
	UTF8ToWideString (newname, newnamew, countof (newnamew));
	return _wrename (oldnamew, newnamew);
}

qfileofs_t Sys_filelength (FILE *f)
{
	qfileofs_t	pos, end;

	pos = Sys_ftell (f);
	Sys_fseek (f, 0, SEEK_END);
	end = Sys_ftell (f);
	Sys_fseek (f, pos, SEEK_SET);

	return end;
}

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES	((DWORD)-1)
#endif

int Sys_FileType (const char *path)
{
	wchar_t		wpath[MAX_PATH];
	DWORD		result;

	UTF8ToWideString (path, wpath, countof (wpath));
	result = GetFileAttributesW (wpath);

	if (result == INVALID_FILE_ATTRIBUTES)
		return FS_ENT_NONE;
	if (result & FILE_ATTRIBUTE_DIRECTORY)
		return FS_ENT_DIRECTORY;

	return FS_ENT_FILE;
}

qboolean Sys_FileExists (const char *path)
{
	wchar_t	wpath[MAX_PATH];
	DWORD attr;
	UTF8ToWideString (path, wpath, countof (wpath));
	attr = GetFileAttributesW (wpath);
	return attr != INVALID_FILE_ATTRIBUTES && !(attr & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_DEVICE));
}

qboolean Sys_GetFileTime (const char *path, time_t *out)
{
	wchar_t		wpath[MAX_PATH];
	HANDLE		handle;
	FILETIME	filetime;
	qboolean	ret;

	UTF8ToWideString (path, wpath, countof (wpath));
	handle = CreateFileW (wpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return false;

	ret = (GetFileTime (handle, NULL, NULL, &filetime) != FALSE);
	CloseHandle (handle);

	if (ret)
	{
		LARGE_INTEGER li;
		li.LowPart = filetime.dwLowDateTime;
		li.HighPart = filetime.dwHighDateTime;
		*out = li.QuadPart / 10000000LL - 11644473600LL;
	}

	return ret;
}

static qboolean Sys_GetRegistryString (HKEY root, const wchar_t *dir, const wchar_t *keyname, char *out, size_t maxchars)
{
	LSTATUS		err;
	HKEY		key;
	WCHAR		wpath[MAX_PATH + 1];
	DWORD		size, type;

	if (!maxchars)
		return false;
	*out = 0;

	err = RegOpenKeyExW (root, dir, 0, KEY_READ, &key);
	if (err != ERROR_SUCCESS)
		return false;

	// Note: string might not contain a terminating null character
	// https://docs.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regqueryvalueexw#remarks

	err = RegQueryValueExW (key, keyname, NULL, &type, NULL, &size);
	if (err != ERROR_SUCCESS || type != REG_SZ || size > sizeof (wpath) - sizeof (wpath[0]))
	{
		RegCloseKey (key);
		return false;
	}

	err = RegQueryValueExW (key, keyname, NULL, &type, (BYTE *)wpath, &size);
	RegCloseKey (key);
	if (err != ERROR_SUCCESS || type != REG_SZ)
		return false;

	wpath[size / sizeof (wpath[0])] = 0;

	if (WideCharToMultiByte (CP_UTF8, 0, wpath, -1, out, maxchars, NULL, NULL) != 0)
		return true;
	*out = 0;
	return false;
}

qboolean Sys_GetSteamDir (char *path, size_t pathsize)
{
	return Sys_GetRegistryString (HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath", path, pathsize);
}

static qboolean Sys_StripTrailingSlashes (char *path)
{
	size_t i = strlen (path);
	while (i > 0 && (path[i-1] == '\\' || path[i-1] == '/'))
		path[--i] = 0;
	return i > 0;
}

qboolean Sys_GetGOGQuakeDir (char *path, size_t pathsize)
{
	if (!Sys_GetRegistryString (HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Wow6432Node\\GOG.com\\Games\\1435828198", L"path",
		path, pathsize))
		return false;

	return Sys_StripTrailingSlashes (path);
}

qboolean Sys_GetGOGQuakeEnhancedDir (char *path, size_t pathsize)
{
	if (!Sys_GetRegistryString (HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Wow6432Node\\GOG.com\\Games\\1739637082", L"path",
		path, pathsize))
		return false;

	return Sys_StripTrailingSlashes (path);
}

// https://github.com/libsdl-org/SDL/blob/120c76c84bbce4c1bfed4e9eb74e10678bd83120/src/core/windows/SDL_windows.c#L88-L99
static HRESULT Sys_InitCOM (void)
{
	HRESULT hr = CoInitializeEx (NULL, COINIT_APARTMENTTHREADED);
	if (hr == RPC_E_CHANGED_MODE)
		hr = CoInitializeEx (NULL, COINIT_MULTITHREADED);

	/* S_FALSE means success, but someone else already initialized. */
	/* You still need to call CoUninitialize in this case! */
	if (hr == S_FALSE)
		return S_OK;

	return hr;
}

static qboolean Sys_GetKnownFolder (const KNOWNFOLDERID *base, const char *subdir, char *path, size_t pathsize)
{
	PWSTR wpath;
	HRESULT hr;
	qboolean ret;

	hr = Sys_InitCOM ();
	if (FAILED (hr))
		return false;

	hr = SHGetKnownFolderPath (base, 0, NULL, &wpath);
	if (FAILED (hr))
	{
		CoUninitialize ();
		return false;
	}

	ret = WideCharToMultiByte (CP_UTF8, 0, wpath, -1, path, pathsize, NULL, NULL) != 0;
	CoTaskMemFree (wpath);
	CoUninitialize ();

	return ret && (size_t) q_strlcat (path, subdir, pathsize) < pathsize;
}

static qboolean Sys_GetNightdiveDir (char *path, size_t pathsize)
{
	return Sys_GetKnownFolder (&FOLDERID_SavedGames, "\\Nightdive Studios\\Quake", path, pathsize);
}

qboolean Sys_GetSteamQuakeUserDir (char *path, size_t pathsize, const char *library)
{
	return Sys_GetNightdiveDir (path, pathsize);
}

qboolean Sys_GetSteamAPILibraryPath (char *path, size_t pathsize, const steamgame_t *game)
{
#ifdef _WIN64
	char installdir[MAX_OSPATH];
	if (!Steam_ResolvePath (installdir, sizeof (installdir), game))
		return false;
	return (size_t) q_snprintf (path, pathsize, "%s/rerelease/steam_api64.dll", installdir) < pathsize;
#endif
	return false;
}

qboolean Sys_GetGOGQuakeEnhancedUserDir (char *path, size_t pathsize)
{
	return Sys_GetNightdiveDir (path, pathsize);
}

qboolean Sys_GetEGSManifestDir (char *path, size_t pathsize)
{
	return Sys_GetKnownFolder (
		&FOLDERID_ProgramData, "\\Epic\\EpicGamesLauncher\\Data\\Manifests",
		path, pathsize
	);
}

const char *Sys_GetEGSLauncherData (void)
{
	char		path[MAX_OSPATH];
	char		*buf;
	FILE		*file;
	int64_t		filesize;
	int			size;

	if (!Sys_GetKnownFolder (
		&FOLDERID_ProgramData, "\\Epic\\UnrealEngineLauncher\\LauncherInstalled.dat",
		path, sizeof (path)))
		return NULL;

	file = Sys_fopen (path, "rb");
	if (!file)
		return NULL;

	_fseeki64 (file, 0, SEEK_END);
	filesize = _ftelli64 (file);
	_fseeki64 (file, 0, SEEK_SET);

	if (filesize < 128 || filesize > (1 << 30))
	{
		fclose (file);
		return NULL;
	}

	size = (int) filesize;
	buf = (char *) malloc (size + 1);
	if (!buf)
	{
		fclose (file);
		return NULL;
	}

	if (fread (buf, size, 1, file) != 1)
	{
		free (buf);
		fclose (file);
		return NULL;
	}
	buf[size] = '\0';

	fclose (file);

	// Convert to UTF-8 if needed
	if ((byte)buf[0] == 0xff && (byte)buf[1] == 0xfe) // UTF-16 little-endian byte order mark
	{
		int		size8;
		char	*buf8;

		size8 = WideCharToMultiByte (CP_UTF8, 0, (WCHAR *)(buf + 2), size / 2 - 1, NULL, 0, NULL, NULL);
		if (!size8)
		{
			free (buf);
			return NULL;
		}

		buf8 = (char *) malloc (size8 + 1);
		if (!buf8)
		{
			free (buf);
			return NULL;
		}

		size8 = WideCharToMultiByte (CP_UTF8, 0, (WCHAR *)(buf + 2), size / 2 - 1, buf8, size8, NULL, NULL);
		if (!size8)
		{
			free (buf8);
			free (buf);
			return NULL;
		}
		buf8[size8] = '\0';

		free (buf);
		buf = buf8;
	}

	return buf;
}

qboolean Sys_GetAltUserPrefDir (qboolean remastered, char *path, size_t pathsize)
{
	const char *subdir = remastered ?
		"\\" ENGINE_USERDIR_WIN "\\rerelease" :
		"\\" ENGINE_USERDIR_WIN "\\original"
	;
	return Sys_GetKnownFolder (&FOLDERID_SavedGames, subdir, path, pathsize);
}

qboolean Sys_Explore (const char *path)
{
	wchar_t			wpath[MAX_PATH];
	LPITEMIDLIST	file, folder;
	HRESULT			hr;
	SFGAOF			sfgaof;
	int				src, dst, slash;
	qboolean		result = false;

	if (Sys_FileType (path) == FS_ENT_NONE)
	{
		Sys_Printf ("Sys_Explore: '%s' not found.\n", path);
		return false;
	}

	UTF8ToWideString (path, wpath, countof (wpath));

	// Canonicalize path (replace forward slashes with backslashes, handle "." and "..")
	for (src = dst = 0, slash = -1; wpath[src]; src++)
	{
		if (wpath[src] == L'/')
			wpath[src] = L'\\';

		if (wpath[src] == L'\\')
		{
			if (slash != -1)
			{
				// Handle "\..\" by going up a level
				if (src == slash + 3 && wpath[slash + 1] == L'.' && wpath[slash + 2] == L'.')
				{
					// We've already written "\..", and dst is now pointing one character past that.
					// Rewind dst by 4 (the character before the '\') and look for the previous '\'.
					for (dst -= 4; dst >= 0; dst--)
						if (wpath[dst] == L'\\')
							break;
					if (dst < 0)
					{
						Sys_Printf ("Sys_Explore: malformed path '%s'.\n", path);
						return false;
					}
				}
				// Ignore "\.\"
				else if (src == slash + 2 && wpath[slash + 1] == L'.')
				{
					dst -= 2;
				}
			}
			slash = src;
		}

		wpath[dst++] = wpath[src];
	}
	wpath[dst] = L'\0';

	// If the cleaned up path is of a different length, we need to find the new index of the last slash character.
	if (src != dst)
		for (src = 0, slash = -1; wpath[src]; src++)
			if (wpath[src] == L'\\')
				slash = src;

	if (slash == -1)
	{
		Sys_Printf ("Sys_Explore: no slash in '%s'.\n", path);
		return false;
	}

	hr = Sys_InitCOM ();
	if (FAILED (hr))
	{
		Sys_Printf ("Sys_Explore: failed to initialize COM (0x%08lx).\n", hr);
		return false;
	}

	wpath[slash] = L'\0';
	hr = SHParseDisplayName (wpath, NULL, &folder, 0, &sfgaof);
	if (FAILED (hr))
	{
		Sys_Printf ("Sys_Explore: SHParseDisplayName failed (0x%08lx) for '%ls'.\n", hr, wpath);
		goto cleanup_com;
	}

	wpath[slash] = L'\\';
	hr = SHParseDisplayName (wpath, NULL, &file, 0, &sfgaof);
	if (FAILED (hr))
	{
		Sys_Printf ("Sys_Explore: SHParseDisplayName failed (0x%08lx) for '%ls'.\n", hr, wpath);
		goto cleanup_folder;
	}

	hr = SHOpenFolderAndSelectItems (folder, 1, (LPCITEMIDLIST *) &file, 0);
	if (SUCCEEDED (hr))
		result = true;
	else
		Sys_Printf ("Sys_Explore: SHOpenFolderAndSelectItems failed (0x%08lx) for '%ls'.\n", hr, wpath);

	CoTaskMemFree (file);
cleanup_folder:
	CoTaskMemFree (folder);
cleanup_com:
	CoUninitialize ();

	return result;
}

static char	cwd[1024];

static void Sys_GetBasedir (char *argv0, char *dst, size_t dstsize)
{
	char *tmp;
	size_t rc;
	wchar_t wpath[MAX_PATH];

	rc = GetCurrentDirectoryW (0, NULL);
	if (rc == 0)
		Sys_Error ("Couldn't determine current directory name length (error %lu)", GetLastError ());
	if (rc >= countof (wpath))
		Sys_Error ("Current directory name too long (%lu)", (DWORD)rc);
	if (!GetCurrentDirectoryW (rc, wpath))
		Sys_Error ("Couldn't determine current directory (error %lu)", GetLastError ());

	WideStringToUTF8 (wpath, dst, dstsize);

	tmp = dst;
	while (*tmp != 0)
		tmp++;
	while (*tmp == 0 && tmp != dst)
	{
		--tmp;
		if (tmp != dst && (*tmp == '/' || *tmp == '\\'))
			*tmp = 0;
	}
}

static char exedir[1024];

static const char *Sys_GetExeDir (void)
{
	wchar_t wpath[MAX_PATH];
	char *p, *slash;

	if (!GetModuleFileNameW (NULL, wpath, countof (wpath)))
		return NULL;

	if (!WideCharToMultiByte (CP_UTF8, 0, wpath, -1, exedir, sizeof (exedir), NULL, NULL))
		return NULL;

	for (p = exedir, slash = NULL; *p; p++)
		if (*p == '/' || *p == '\\')
			slash = p;
	if (slash)
		*slash = 0;

	return exedir;
}

typedef struct winfindfile_s {
	findfile_t			base;
	WIN32_FIND_DATAW	data;
	HANDLE				handle;
} winfindfile_t;

static void Sys_FillFindData (winfindfile_t *find)
{
	WideStringToUTF8 (find->data.cFileName, find->base.name, countof (find->base.name));
	find->base.attribs = 0;
	if (find->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		find->base.attribs |= FA_DIRECTORY;
}

findfile_t *Sys_FindFirst (const char *dir, const char *ext)
{
	winfindfile_t		*ret;
	char				pattern[MAX_OSPATH];
	wchar_t				wpattern[MAX_PATH];
	HANDLE				handle;
	WIN32_FIND_DATAW	data;

	if (!ext)
		ext = "*";
	else if (*ext == '.')
		++ext;
	q_snprintf (pattern, sizeof (pattern), "%s/*.%s", dir, ext);

	UTF8ToWideString (pattern, wpattern, countof (wpattern));
	handle = FindFirstFileW (wpattern, &data);

	if (handle == INVALID_HANDLE_VALUE)
		return NULL;

	ret = (winfindfile_t *) calloc (1, sizeof (winfindfile_t));
	if (!ret)
		Sys_Error ("Sys_FindFirst: out of memory");
	ret->handle = handle;
	ret->data = data;
	Sys_FillFindData (ret);

	return (findfile_t *) ret;
}

findfile_t *Sys_FindNext (findfile_t *find)
{
	winfindfile_t *wfind = (winfindfile_t *) find;
	if (!FindNextFileW (wfind->handle, &wfind->data))
	{
		Sys_FindClose (find);
		return NULL;
	}
	Sys_FillFindData (wfind);
	return find;
}

void Sys_FindClose (findfile_t *find)
{
	if (find)
	{
		winfindfile_t *wfind = (winfindfile_t *) find;
		FindClose (wfind->handle);
		free (wfind);
	}
}

typedef enum { dpi_unaware = 0, dpi_system_aware = 1, dpi_monitor_aware = 2 } dpi_awareness;
typedef BOOL (WINAPI *SetProcessDPIAwareFunc)();
typedef HRESULT (WINAPI *SetProcessDPIAwarenessFunc)(dpi_awareness value);

static void Sys_SetDPIAware (void)
{
	HMODULE hUser32, hShcore;
	SetProcessDPIAwarenessFunc setDPIAwareness;
	SetProcessDPIAwareFunc setDPIAware;

	/* Neither SDL 1.2 nor SDL 2.0.3 can handle the OS scaling our window.
	  (e.g. https://bugzilla.libsdl.org/show_bug.cgi?id=2713)
	  Call SetProcessDpiAwareness/SetProcessDPIAware to opt out of scaling.
	*/

	hShcore = LoadLibraryA ("Shcore.dll");
	hUser32 = LoadLibraryA ("user32.dll");
	setDPIAwareness = (SetProcessDPIAwarenessFunc) (hShcore ? GetProcAddress (hShcore, "SetProcessDpiAwareness") : NULL);
	setDPIAware = (SetProcessDPIAwareFunc) (hUser32 ? GetProcAddress (hUser32, "SetProcessDPIAware") : NULL);

	if (setDPIAwareness) /* Windows 8.1+ */
		setDPIAwareness (dpi_monitor_aware);
	else if (setDPIAware) /* Windows Vista-8.0 */
		setDPIAware ();

	if (hShcore)
		FreeLibrary (hShcore);
	if (hUser32)
		FreeLibrary (hUser32);
}

static void Sys_SetTimerResolution(void)
{
	/* Set OS timer resolution to 1ms.
	   Works around buffer underruns with directsound and SDL2, but also
	   will make Sleep()/SDL_Dleay() accurate to 1ms which should help framerate
	   stability.
	*/
	timeBeginPeriod (1);
}

static HHOOK key_hook = NULL;

#define HOOKED_KEYS			\
	HOOK_KEY (PRINTSCREEN)	\
	HOOK_KEY (CAPSLOCK)		\
	HOOK_KEY (SCROLLLOCK)	\
	HOOK_KEY (NUMLOCKCLEAR)	\

#define SC_CAPSLOCK			0x3A
#define SC_SCROLLLOCK		0x46
#define SC_NUMLOCKCLEAR		0xE045
#define SC_PRINTSCREEN		0xE037

enum
{
	#define HOOK_KEY(k)		HK_##k,
	HOOKED_KEYS
	#undef HOOK_KEY

	HK_COUNT,
};

static const SDL_Scancode hk_sdl_scancodes[HK_COUNT] =
{
	#define HOOK_KEY(k)		SDL_SCANCODE_##k,
	HOOKED_KEYS
	#undef HOOK_KEY
};

static int GetFilteredKeyIndex (int scancode)
{
	switch (scancode)
	{
		#define HOOK_KEY(k)	case SC_##k: return HK_##k;
		HOOKED_KEYS
		#undef HOOK_KEY
	default:
		return -1;
	}
}

LRESULT CALLBACK KeyFilter (int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0 && VID_HasMouseOrInputFocus ())
	{
		PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT) lParam;
		int scancode = p->scanCode | (p->flags & 1 ? 0xE000 : 0);
		int key = GetFilteredKeyIndex (scancode);
		if (key != -1)
		{
			// Note: if we intercept a key down message,
			// we also need to intercept the corresponding key up.
			static uint32_t pending_mask = 0;

			qboolean force_intercept = (pending_mask >> key) & 1;
			qboolean down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
			qboolean intercept =
				force_intercept ||
				key == HK_PRINTSCREEN ||
				(key_dest == key_game || M_WaitingForKeyBinding ())
			;

			if (intercept)
			{
				SDL_Event ev;
				if (down)
					pending_mask |= (1 << key);
				else
					pending_mask &= ~(1 << key);
				memset (&ev, 0, sizeof (ev));
				ev.type = down ? SDL_KEYDOWN : SDL_KEYUP;
				ev.key.state = down ? SDL_PRESSED : SDL_RELEASED;
				ev.key.keysym.scancode = hk_sdl_scancodes[key];
				SDL_PushEvent (&ev);
				return 1;
			}
		}
	}

	return CallNextHookEx (NULL, nCode, wParam, lParam);
}

typedef struct
{
	WCHAR name[MAX_PATH];
	DWORD id;
	DWORD parent_id;
} procinfo_t;

/*
=================
Sys_GetProcList
=================
*/
static procinfo_t *Sys_GetProcList (void)
{
	HANDLE				snapshot;
	PROCESSENTRY32W		proc_entry;
	procinfo_t			*result = NULL;

	snapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
		return NULL;

	proc_entry.dwSize = sizeof (proc_entry);

	if (Process32FirstW (snapshot, &proc_entry))
	{
		do
		{
			procinfo_t info;
			wcscpy_s (info.name, Q_COUNTOF (info.name), proc_entry.szExeFile);
			info.id = proc_entry.th32ProcessID;
			info.parent_id = proc_entry.th32ParentProcessID;
			VEC_PUSH (result, info);
		} while (Process32NextW (snapshot, &proc_entry));
	}

	CloseHandle (snapshot);

	return result;
}

/*
=================
Sys_FindProc
=================
*/
static procinfo_t *Sys_FindProc (DWORD id, procinfo_t *procs)
{
	size_t i;
	if (id == 0)
		return NULL;
	for (i = 0; i < VEC_SIZE (procs); i++)
		if (procs[i].id == id)
			return &procs[i];
	return NULL;
}

/*
=================
Sys_IsStartedFromMapEditor

Returns true if the process was started from TrenchBroom, JACK, ne_q1spCompilingGui, or q1compile
=================
*/
qboolean Sys_IsStartedFromMapEditor (void)
{
	qboolean from_editor = false;
	procinfo_t *proclist = Sys_GetProcList ();

	if (proclist)
	{
		procinfo_t *current = Sys_FindProc (GetCurrentProcessId (), proclist);
		procinfo_t *parent = current ? Sys_FindProc (current->parent_id, proclist) : NULL;

		while (parent && _wcsicmp (parent->name, L"cmd.exe") == 0)
			parent = Sys_FindProc (parent->parent_id, proclist);

		if (parent)
		{
			#define PARENT_STARTS_WITH(prefix)	(_wcsnicmp (parent->name, L##prefix, wcslen (L##prefix)) == 0)
			if (PARENT_STARTS_WITH ("TrenchBroom") ||
				PARENT_STARTS_WITH ("jack") ||
				PARENT_STARTS_WITH ("ne_q1spCompilingGui") ||
				PARENT_STARTS_WITH ("q1compile"))
				from_editor = true;
			#undef PARENT_STARTS_WITH
		}

		VEC_FREE (proclist);
	}

	return from_editor;
}

void Sys_Init (void)
{
	SYSTEM_INFO info;

	Sys_SetTimerResolution ();
	Sys_SetDPIAware ();

	memset (cwd, 0, sizeof(cwd));
	Sys_GetBasedir(NULL, cwd, sizeof(cwd));
	host_parms->basedir = cwd;

	host_parms->exedir = Sys_GetExeDir ();

	/* userdirs not really necessary for windows guys.
	 * can be done if necessary, though... */
	host_parms->userdir = host_parms->basedir; /* code elsewhere relies on this ! */

	if (!IsWindowsXPOrGreater ())
		Sys_Error ("This engine requires Windows XP or newer");

	GetSystemInfo(&info);
	host_parms->numcpus = info.dwNumberOfProcessors;
	if (host_parms->numcpus < 1)
		host_parms->numcpus = 1;
	Sys_Printf("Detected %d CPUs.\n", host_parms->numcpus);

	if (isDedicated)
	{
		if (!AllocConsole ())
		{
			isDedicated = false;	/* so that we have a graphical error dialog */
			Sys_Error ("Couldn't create dedicated server console");
		}

		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);
	}

	rcp_counter_freq = 1.0 / SDL_GetPerformanceFrequency();
}

void Sys_mkdir (const char *path)
{
	wchar_t wpath[MAX_PATH];
	BOOL result;
	DWORD err;

	UTF8ToWideString (path, wpath, countof (wpath));
	result = CreateDirectoryW (wpath, NULL);
	if (result)
		return;

	err = GetLastError ();
	if (err != ERROR_ALREADY_EXISTS)
		Sys_Error ("Unable to create directory %s (error %lu)", path, err);
}

static const wchar_t errortxt1[] = L"\nERROR-OUT BEGIN\n\n";
static const wchar_t errortxt2[] = L"\nQUAKE ERROR: ";

qboolean Sys_IsDebuggerPresent (void)
{
	return IsDebuggerPresent ();
}

void Sys_ReportError (const char *error, ...)
{
	va_list		argptr;
	char		text[1024];
	wchar_t		wtext[1024];

	host_parms->errstate++;

	va_start (argptr, error);
	q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	PR_SwitchQCVM(NULL);

	if (!MultiByteToWideChar (CP_UTF8, 0, text, -1, wtext, countof (wtext)))
		wcscpy (wtext, L"An unknown error has occurred");

	if (isDedicated)
		WriteConsoleW (houtput, errortxt1, wcslen(errortxt1), NULL, NULL);
	/* SDL will put these into its own stderr log,
	   so print to stderr even in graphical mode. */
	fputws (errortxt1, stderr);
	Host_Shutdown ();
	fputws (errortxt2, stderr);
	fputws (wtext, stderr);
	fputws (L"\n\n", stderr);
	if (!isDedicated)
		PL_ErrorDialog (text);
	else
	{
		WriteConsoleW (houtput, errortxt2, wcslen(errortxt2), NULL, NULL);
		WriteConsoleW (houtput, wtext,     wcslen(wtext),     NULL, NULL);
		WriteConsoleW (houtput, L"\r\n",   2,		          NULL, NULL);
		SDL_Delay (3000);	/* show the console 3 more seconds */
	}

	exit (1);
}

void Sys_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		qtext[1024];
	char		u8text[4096];
	wchar_t		wtext[2048];
	int			len;

	va_start (argptr,fmt);
	q_vsnprintf (qtext, sizeof (qtext), fmt, argptr);
	va_end (argptr);

	UTF8_FromQuake (u8text, sizeof (u8text), qtext);

	// log all messages to file as well if -condebug was specified
	Con_DebugLog (u8text);

	len = MultiByteToWideChar (CP_UTF8, 0, u8text, -1, wtext, countof (wtext));
	if (!len)
		return;

	if (isDedicated)
	{
		WriteConsoleW (houtput, wtext, len, NULL, NULL);
	}
	else
	{
	/* SDL will put these into its own stdout log,
	   so print to stdout even in graphical mode. */
		fputws (wtext, stdout);
	}

	OutputDebugStringW (wtext);
}

void Sys_Quit (void)
{
	Host_Shutdown();

	if (isDedicated)
		FreeConsole ();

	exit (0);
}

double Sys_DoubleTime (void)
{
	return (double)SDL_GetPerformanceCounter() * rcp_counter_freq;
}

const char *Sys_ConsoleInput (void)
{
	static char	con_text[256];
	static int	textlen;
	INPUT_RECORD	recs[1024];
	int		ch;
	DWORD		dummy, numread, numevents;

	for ( ;; )
	{
		if (GetNumberOfConsoleInputEvents(hinput, &numevents) == 0)
			Sys_Error ("Error getting # of console events");

		if (! numevents)
			break;

		if (ReadConsoleInput(hinput, recs, 1, &numread) == 0)
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
		    if (recs[0].Event.KeyEvent.bKeyDown == FALSE)
		    {
			ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

			switch (ch)
			{
			case '\r':
				WriteFile(houtput, "\r\n", 2, &dummy, NULL);

				if (textlen != 0)
				{
					con_text[textlen] = 0;
					textlen = 0;
					return con_text;
				}

				break;

			case '\b':
				WriteFile(houtput, "\b \b", 3, &dummy, NULL);
				if (textlen != 0)
					textlen--;

				break;

			default:
				if (ch >= ' ')
				{
					WriteFile(houtput, &ch, 1, &dummy, NULL);
					con_text[textlen] = ch;
					textlen = (textlen + 1) & 0xff;
				}

				break;
			}
		    }
		}
	}

	return NULL;
}

void Sys_Sleep (unsigned long msecs)
{
/*	Sleep (msecs);*/
	SDL_Delay (msecs);
}

void Sys_SendKeyEvents (void)
{
	IN_Commands();		//ericw -- allow joysticks to add keys so they can be used to confirm SCR_ModalMessage
	IN_SendKeyEvents();
}

void Sys_ActivateKeyFilter (qboolean active)
{
	if (isDedicated || !!active == (key_hook!=NULL))
		return;

	if (key_hook)
	{
		UnhookWindowsHookEx (key_hook);
		key_hook = NULL;
	}
	else
	{
		key_hook = SetWindowsHookExW (WH_KEYBOARD_LL, KeyFilter, GetModuleHandleW (NULL), 0);
		if (!key_hook)
			Sys_Printf ("Warning: SetWindowsHookExW failed (%lu)\n", GetLastError ());
	}
}

void *Sys_LoadLibrary (const char *path)
{
	wchar_t wpath[MAX_PATH];
	UTF8ToWideString (path, wpath, countof (wpath));
	return (void *) LoadLibraryW (wpath);
}

void *Sys_GetLibraryFunction (void *lib, const char *func)
{
	return GetProcAddress ((HMODULE) lib, func);
}

void Sys_CloseLibrary (void *lib)
{
	FreeLibrary ((HMODULE) lib);
}
