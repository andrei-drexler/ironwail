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

#include "arch_def.h"
#include "quakedef.h"
#include "steam.h"

#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#if defined(PLATFORM_OSX) || defined(PLATFORM_HAIKU)
#include <libgen.h>	/* dirname() and basename() */
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <pwd.h>
#include <dlfcn.h>

#if defined(SDL_FRAMEWORK) || defined(NO_SDL_CONFIG)
#if defined(USE_SDL2)
#include <SDL2/SDL.h>
#else
#include <SDL/SDL.h>
#endif
#else
#include "SDL.h"
#endif


qboolean		isDedicated;

static qboolean		stdinIsATTY;	/* from ioquake3 source */

static double rcp_counter_freq;

FILE *Sys_fopen (const char *path, const char *mode)
{
	if (strchr (mode, 'w'))
	{
		char dir[MAX_OSPATH];
		int i, rc;
		q_strlcpy (dir, path, sizeof (dir));
		for (i = 1; dir[i]; i++)
		{
			if (dir[i] != '/')
				continue;
			dir[i] = '\0';
			rc = mkdir (dir, 0777);
			if (rc != 0 && errno == EEXIST)
			{
				struct stat st;
				if (stat (dir, &st) == 0 && S_ISDIR (st.st_mode))
					rc = 0;
			}
			if (rc != 0)
				return NULL;
			dir[i] = '/';
		}
	}

	return fopen (path, mode);
}

COMPILE_TIME_ASSERT (CHECK_LARGE_FILE_SUPPORT, sizeof (off_t) >= sizeof (qfileofs_t));

int Sys_fseek (FILE *file, qfileofs_t ofs, int origin)
{
	return fseeko (file, ofs, origin);
}

qfileofs_t Sys_ftell (FILE *file)
{
	return ftello (file);
}

int Sys_remove (const char *path)
{
	return remove (path);
}

int Sys_rename (const char *oldname, const char *newname)
{
	return rename (oldname, newname);
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

qboolean Sys_FileExists (const char *path)
{
	return access (path, F_OK) == 0;
}

int Sys_FileType (const char *path)
{
	/*
	if (access(path, R_OK) == -1)
		return 0;
	*/
	struct stat	st;

	if (stat(path, &st) != 0)
		return FS_ENT_NONE;
	if (S_ISDIR(st.st_mode))
		return FS_ENT_DIRECTORY;
	if (S_ISREG(st.st_mode))
		return FS_ENT_FILE;

	return FS_ENT_NONE;
}

qboolean Sys_GetFileTime (const char *path, time_t *out)
{
	struct stat st;
	if (stat (path, &st) != 0)
		return false;
	*out = st.st_mtime;
	return true;
}

#if defined(__linux__) || defined(__sun) || defined(sun) || defined(_AIX)
static int Sys_NumCPUs (void)
{
	int numcpus = sysconf(_SC_NPROCESSORS_ONLN);
	return (numcpus < 1) ? 1 : numcpus;
}

#elif defined(PLATFORM_OSX)
#include <sys/sysctl.h>
#if !defined(HW_AVAILCPU)	/* using an ancient SDK? */
#define HW_AVAILCPU		25	/* needs >= 10.2 */
#endif
static int Sys_NumCPUs (void)
{
	int numcpus;
	int mib[2];
	size_t len;

#if defined(_SC_NPROCESSORS_ONLN)	/* needs >= 10.5 */
	numcpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (numcpus != -1)
		return (numcpus < 1) ? 1 : numcpus;
#endif
	len = sizeof(numcpus);
	mib[0] = CTL_HW;
	mib[1] = HW_AVAILCPU;
	sysctl(mib, 2, &numcpus, &len, NULL, 0);
	if (sysctl(mib, 2, &numcpus, &len, NULL, 0) == -1)
	{
		mib[1] = HW_NCPU;
		if (sysctl(mib, 2, &numcpus, &len, NULL, 0) == -1)
			return 1;
	}
	return (numcpus < 1) ? 1 : numcpus;
}

#elif defined(__sgi) || defined(sgi) || defined(__sgi__) /* IRIX */
static int Sys_NumCPUs (void)
{
	int numcpus = sysconf(_SC_NPROC_ONLN);
	if (numcpus < 1)
		numcpus = 1;
	return numcpus;
}

#elif defined(PLATFORM_BSD)
#include <sys/sysctl.h>
static int Sys_NumCPUs (void)
{
	int numcpus;
	int mib[2];
	size_t len;

#if defined(_SC_NPROCESSORS_ONLN)
	numcpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (numcpus != -1)
		return (numcpus < 1) ? 1 : numcpus;
#endif
	len = sizeof(numcpus);
	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	if (sysctl(mib, 2, &numcpus, &len, NULL, 0) == -1)
		return 1;
	return (numcpus < 1) ? 1 : numcpus;
}

#elif defined(__hpux) || defined(__hpux__) || defined(_hpux)
#include <sys/mpctl.h>
static int Sys_NumCPUs (void)
{
	int numcpus = mpctl(MPC_GETNUMSPUS, NULL, NULL);
	return numcpus;
}

#else /* unknown OS */
static int Sys_NumCPUs (void)
{
	return -2;
}
#endif

qboolean Sys_GetSteamDir (char *path, size_t pathsize)
{
	const char		*home_dir = NULL;
	struct passwd	*pwent;

	pwent = getpwuid( getuid() );
	if (pwent == NULL)
		perror("getpwuid");
	else
		home_dir = pwent->pw_dir;
	if (home_dir == NULL)
		home_dir = getenv("HOME");
	if (home_dir == NULL)
		return false;

	if ((size_t) q_snprintf (path, pathsize, "%s/.steam/steam", home_dir) < pathsize && Steam_IsValidPath (path))
		return true;
	if ((size_t) q_snprintf (path, pathsize, "%s/.local/share/Steam", home_dir) < pathsize && Steam_IsValidPath (path))
		return true;
	if ((size_t) q_snprintf (path, pathsize, "%s/.var/app/com.valvesoftware.Steam/.steam/steam", home_dir) < pathsize && Steam_IsValidPath (path))
		return true;
	if ((size_t) q_snprintf (path, pathsize, "%s/.var/app/com.valvesoftware.Steam/.local/share/Steam", home_dir) < pathsize && Steam_IsValidPath (path))
		return true;

	return false;
}

qboolean Sys_GetSteamQuakeUserDir (char *path, size_t pathsize, const char *library)
{
	const char *fmt = "%s/steamapps/compatdata/2310/pfx/drive_c/users/steamuser/Saved Games/Nightdive Studios/Quake";
	struct stat st;

	if ((size_t) q_snprintf (path, pathsize, fmt, library) >= pathsize)
		return false;

	return stat (path, &st) == 0 && S_ISDIR (st.st_mode);
}

qboolean Sys_GetSteamAPILibraryPath (char *path, size_t pathsize, const steamgame_t *game)
{
	char		config_info_path[MAX_OSPATH];
	char		*line = NULL;
	size_t		line_size = 0;
	ssize_t		bytes_read = 0;
	FILE		*config_info;
	int			read_lines;
	qboolean	result;

	if ((size_t) q_snprintf (config_info_path, sizeof (config_info_path), "%s/steamapps/compatdata/%d/config_info", game->library, game->appid) >= pathsize)
		return false;

	config_info = Sys_fopen (config_info_path, "r");
	if (!config_info)
		return false;

	// lib dir is on line 3, lib64 on line 4
	read_lines = sizeof (void *) == 4 ? 3 : 4;
	while (read_lines-- > 0)
	{
		if ((bytes_read = getline (&line, &line_size, config_info)) == -1)
		{
			fclose (config_info);
			free (line);
			return false;
		}
	}

	fclose (config_info);
	if (!line)
		return false;

	line_size = strlen (line);

	if (line_size > 0 && line[line_size - 1] == '\n')
		line[--line_size] = '\0';
	if (line_size > 0 && line[line_size - 1] == '/')
		line[--line_size] = '\0';

	result = (size_t) q_snprintf (path, pathsize, "%s/libsteam_api.so", line) < pathsize;

	free (line);

	return result;
}

qboolean Sys_GetGOGQuakeDir (char *path, size_t pathsize)
{
	return false;
}

qboolean Sys_GetGOGQuakeEnhancedDir (char *path, size_t pathsize)
{
	return false;
}

qboolean Sys_GetGOGQuakeEnhancedUserDir (char *path, size_t pathsize)
{
	return false;
}

qboolean Sys_GetEGSManifestDir (char *path, size_t pathsize)
{
	return false;
}

const char *Sys_GetEGSLauncherData (void)
{
	return NULL;
}

#ifdef PLATFORM_OSX
#define SYS_USERDIR	"Library/Application Support/" ENGINE_USERDIR_OSX
#else
#define SYS_USERDIR	ENGINE_USERDIR_UNIX
#endif

static char	cwd[MAX_OSPATH];
#ifdef DO_USERDIRS
static char	userdir[MAX_OSPATH];

#ifdef PLATFORM_HAIKU
#include <FindDirectory.h>
#include <fs_info.h>

static void Sys_GetUserdir (char *dst, size_t dstsize)
{
	dev_t volume = dev_for_path("/boot");
	char buffer[B_PATH_NAME_LENGTH];
	status_t result;

	result = find_directory(B_USER_NONPACKAGED_DATA_DIRECTORY, volume, false, buffer, sizeof(buffer));
	if (result != B_OK)
		Sys_Error ("Couldn't determine userspace directory");

	q_snprintf (dst, dstsize, "%s/%s", buffer, SYS_USERDIR);
}
#else
static void Sys_GetUserdir (char *dst, size_t dstsize)
{
	size_t		n;
	const char	*home_dir = NULL;
	struct passwd	*pwent;

	pwent = getpwuid( getuid() );
	if (pwent == NULL)
		perror("getpwuid");
	else
		home_dir = pwent->pw_dir;
	if (home_dir == NULL)
		home_dir = getenv("HOME");
	if (home_dir == NULL)
		Sys_Error ("Couldn't determine userspace directory");

/* what would be a maximum path for a file in the user's directory...
 * $HOME/SYS_USERDIR/game_dir/dirname1/dirname2/dirname3/filename.ext
 * still fits in the MAX_OSPATH == 256 definition, but just in case :
 */
	n = strlen(home_dir) + strlen(SYS_USERDIR) + 50;
	if (n >= dstsize)
		Sys_Error ("Insufficient array size for userspace directory");

	q_snprintf (dst, dstsize, "%s/%s", home_dir, SYS_USERDIR);
}
#endif	/* PLATFORM_HAIKU */
#endif	/* DO_USERDIRS */

qboolean Sys_GetAltUserPrefDir (qboolean remastered, char *dst, size_t dstsize)
{
	const char		*home_dir = NULL;
	struct passwd	*pwent;

	pwent = getpwuid( getuid() );
	if (pwent == NULL)
		perror("getpwuid");
	else
		home_dir = pwent->pw_dir;
	if (home_dir == NULL)
		home_dir = getenv("HOME");
	if (home_dir == NULL)
		return false;

	if ((size_t) q_snprintf (dst, dstsize, "%s/%s/%s",
		home_dir, SYS_USERDIR, remastered ? ".rerelease" : ".original") >= dstsize)
		return false;

	return true;
}

static qboolean Sys_Exec (const char *cmd, ...)
{
	pid_t p = fork ();
	if (p < 0) // fork failed
		return false;
	else if (p == 0) // child process
	{
		va_list		argptr;
		const char	*argv[1024];
		size_t		argc;
		FILE		*dummy;

		argv[0] = (char *)cmd;
		argc = 1;

		va_start (argptr, cmd);
		for (; argc + 1 < countof (argv); argc++)
		{
			const char *cur = va_arg (argptr, const char*);
			if (!cur)
				break;
			argv[argc] = cur;
		}
		va_end (argptr);
		argv[argc++] = NULL;

		// Disable stdout/stderr
		// Note: using a dummy variable to avoid triggering -Wunused-result
		dummy = freopen ("/dev/null", "w", stdout); (void)dummy;
		dummy = freopen ("/dev/null", "w", stderr); (void)dummy;

		execvp (cmd, (char **) argv);

		exit (EXIT_FAILURE);
	}
	else // original process
	{
		return true;
	}
}

qboolean Sys_Explore (const char *path)
{
	char buf[32768];
	char *s;

	if (Sys_FileType (path) == FS_ENT_NONE)
		return false;

	// Try to identify the current desktop so we can open the parent dir in the file manager *and* select the right file in it
	s = getenv ("XDG_CURRENT_DESKTOP");
	if (s)
	{
		char *cur = NULL;
		char *desktop = NULL;
		q_strlcpy (buf, s, sizeof (buf));
		for (desktop = strtok_r (buf, ":", &cur); desktop; desktop = strtok_r (NULL, ":", &cur))
		{
			if (q_strcasecmp (desktop, "gnome") == 0)
				return Sys_Exec ("nautilus", "--select", path);
			if (q_strcasecmp (desktop, "kde") == 0)
				return Sys_Exec ("dolphin", "--select", path);
		}
	}

	// Fall back to just opening the parent dir without selecting the file
	q_strlcpy (buf, path, sizeof (buf));
	s = Q_strrchr (buf, '/');
	if (!s)
		return false;
	s[1] = '\0'; // terminate after the slash
	return SDL_OpenURL (buf) == 0;
}

#ifdef PLATFORM_OSX
static char *OSX_StripAppBundle (char *dir)
{ /* based on the ioquake3 project at icculus.org. */
	static char	osx_path[MAX_OSPATH];

	q_strlcpy (osx_path, dir, sizeof(osx_path));
	if (strcmp(basename(osx_path), "MacOS"))
		return dir;
	q_strlcpy (osx_path, dirname(osx_path), sizeof(osx_path));
	if (strcmp(basename(osx_path), "Contents"))
		return dir;
	q_strlcpy (osx_path, dirname(osx_path), sizeof(osx_path));
	if (!strstr(basename(osx_path), ".app"))
		return dir;
	q_strlcpy (osx_path, dirname(osx_path), sizeof(osx_path));
	return osx_path;
}

static void Sys_GetBasedir (char *argv0, char *dst, size_t dstsize)
{
	char	*tmp;

	if (realpath(argv0, dst) == NULL)
	{
		perror("realpath");
		if (getcwd(dst, dstsize - 1) == NULL)
	_fail:		Sys_Error ("Couldn't determine current directory");
	}
	else
	{
		/* strip off the binary name */
		if (! (tmp = strdup (dst))) goto _fail;
		q_strlcpy (dst, dirname(tmp), dstsize);
		free (tmp);
	}

	tmp = OSX_StripAppBundle(dst);
	if (tmp != dst)
		q_strlcpy (dst, tmp, dstsize);
}
#else
static void Sys_GetBasedir (char *argv0, char *dst, size_t dstsize)
{
	char	*tmp;

	#ifdef PLATFORM_HAIKU
	if (realpath(argv0, dst) == NULL)
	{
		perror("realpath");
		if (getcwd(dst, dstsize - 1) == NULL)
	_fail:		Sys_Error ("Couldn't determine current directory");
	}
	else
	{
		/* strip off the binary name */
		if (! (tmp = strdup (dst))) goto _fail;
		q_strlcpy (dst, dirname(tmp), dstsize);
		free (tmp);
	}
	#else
	if (getcwd(dst, dstsize - 1) == NULL)
		Sys_Error ("Couldn't determine current directory");

	tmp = dst;
	while (*tmp != 0)
		tmp++;
	while (*tmp == 0 && tmp != dst)
	{
		--tmp;
		if (tmp != dst && *tmp == '/')
			*tmp = 0;
	}
	#endif
}
#endif

static char exedir[MAX_OSPATH];

static const char *Sys_GetExeDir (void)
{
	char *argv0 = host_parms->argv[0];
	char *slash = argv0 ? strrchr (argv0, '/') : NULL;
	if (!slash || slash - argv0 >= countof (exedir))
		return NULL;

	memcpy (exedir, argv0, slash - argv0);
	exedir[slash - argv0] = 0;

	return exedir;
}

typedef struct unixfindfile_s {
	findfile_t		base;
	DIR				*handle;
	struct dirent	*data;
	char			filter[8];
} unixfindfile_t;

static void Sys_FillFindData (unixfindfile_t *find)
{
	q_strlcpy (find->base.name, find->data->d_name, sizeof (find->base.name));
	find->base.attribs = 0;
	if (find->data->d_type & DT_DIR)
		find->base.attribs |= FA_DIRECTORY;
}

static struct dirent *readdir_filtered (DIR *handle, const char *ext)
{
	while (1)
	{
		struct dirent *data = readdir (handle);
		if (!data || ext[0] == '*' || !strcmp (ext, COM_FileGetExtension (data->d_name)))
			return data;
	}
	return NULL;
}

findfile_t *Sys_FindFirst (const char *dir, const char *ext)
{
	unixfindfile_t		*ret;
	DIR					*handle;
	struct dirent		*data;

	if (!ext)
		ext = "*";
	else if (*ext == '.')
		++ext;

	if (Q_strlen (ext) >= countof (ret->filter))
		Sys_Error ("Sys_FindFirst: extension too long '%s'", ext);

	handle = opendir (dir);
	if (!handle)
		return NULL;

	data = readdir_filtered (handle, ext);
	if (!data)
	{
		closedir (handle);
		return NULL;
	}

	ret = (unixfindfile_t *) calloc (1, sizeof (unixfindfile_t));
	if (!ret)
		Sys_Error ("Sys_FindFirst: out of memory");
	ret->handle = handle;
	ret->data = data;
	q_strlcpy (ret->filter, ext, sizeof (ret->filter));
	Sys_FillFindData (ret);

	return (findfile_t *) ret;
}

findfile_t *Sys_FindNext (findfile_t *find)
{
	unixfindfile_t *ufind = (unixfindfile_t *) find;
	ufind->data = readdir_filtered (ufind->handle, ufind->filter);
	if (!ufind->data)
	{
		Sys_FindClose (find);
		return NULL;
	}
	Sys_FillFindData (ufind);
	return find;
}

void Sys_FindClose (findfile_t *find)
{
	if (find)
	{
		unixfindfile_t *ufind = (unixfindfile_t *) find;
		closedir (ufind->handle);
		free (ufind);
	}
}

/*
=================
Sys_GetParentProcessName
=================
*/
static qboolean Sys_GetParentProcessName (char *dst, size_t dstsize)
{
#ifdef __linux__
	char link[MAX_OSPATH];
	q_snprintf (link, sizeof (link), "/proc/%d/exe", getppid ());
	if (readlink (link, dst, dstsize) < 0)
		return false;
	dst[dstsize - 1] = '\0';
	return true;
#else
	return false;
#endif
}

/*
=================
Sys_IsStartedFromMapEditor

Returns true if the process was started from TrenchBroom
=================
*/
qboolean Sys_IsStartedFromMapEditor (void)
{
	char path[MAX_OSPATH];
	const char *slash, *exe;

	if (!Sys_GetParentProcessName (path, sizeof (path)))
		return false;

	slash = strrchr (path, '/');
	exe = slash ? slash + 1 : path;

	return q_strcasestr (exe, "trenchbroom") != NULL || q_strcasestr (exe, "jack") != NULL;
}

void Sys_Init (void)
{
	const char* term = getenv("TERM");
	stdinIsATTY = isatty(STDIN_FILENO) &&
			!(term && (!strcmp(term, "raw") || !strcmp(term, "dumb")));
	if (!stdinIsATTY)
		Sys_Printf("Terminal input not available.\n");

	memset (cwd, 0, sizeof(cwd));
	Sys_GetBasedir(host_parms->argv[0], cwd, sizeof(cwd));
	host_parms->basedir = cwd;
	host_parms->exedir = Sys_GetExeDir ();
#ifndef DO_USERDIRS
	host_parms->userdir = host_parms->basedir; /* code elsewhere relies on this ! */
#else
	memset (userdir, 0, sizeof(userdir));
	Sys_GetUserdir(userdir, sizeof(userdir));
	Sys_mkdir (userdir);
	host_parms->userdir = userdir;
#endif
	host_parms->numcpus = Sys_NumCPUs ();
	Sys_Printf("Detected %d CPUs.\n", host_parms->numcpus);

	rcp_counter_freq = 1.0 / SDL_GetPerformanceFrequency();
}

void Sys_mkdir (const char *path)
{
	int rc = mkdir (path, 0777);
	if (rc != 0 && errno == EEXIST)
	{
		struct stat st;
		if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
			rc = 0;
	}
	if (rc != 0)
	{
		rc = errno;
		Sys_Error("Unable to create directory %s: %s", path, strerror(rc));
	}
}

qboolean Sys_IsDebuggerPresent (void)
{
	return false;
}

static const char errortxt1[] = "\nERROR-OUT BEGIN\n\n";
static const char errortxt2[] = "\nQUAKE ERROR: ";

void Sys_ReportError (const char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	host_parms->errstate++;

	va_start (argptr, error);
	q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	fputs (errortxt1, stderr);
	Host_Shutdown ();
	fputs (errortxt2, stderr);
	fputs (text, stderr);
	fputs ("\n\n", stderr);
	if (!isDedicated)
		PL_ErrorDialog(text);

	exit (1);
}

void Sys_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		qtext[1024];
	char		u8text[4096];

	va_start (argptr,fmt);
	q_vsnprintf (qtext, sizeof (qtext), fmt, argptr);
	va_end (argptr);

	UTF8_FromQuake (u8text, sizeof (u8text), qtext);
	printf ("%s", u8text);

	// log all messages to file as well if -condebug was specified
	Con_DebugLog (u8text);
}

void Sys_Quit (void)
{
	Host_Shutdown();

	exit (0);
}

double Sys_DoubleTime (void)
{
	return (double)SDL_GetPerformanceCounter() * rcp_counter_freq;
}

const char *Sys_ConsoleInput (void)
{
	static qboolean	con_eof = false;
	static char	con_text[256];
	static int	textlen;
	char		c;
	fd_set		set;
	struct timeval	timeout;

	if (!stdinIsATTY || con_eof)
		return NULL;

	FD_ZERO (&set);
	FD_SET (0, &set);	// stdin
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	while (select (1, &set, NULL, NULL, &timeout))
	{
		if (read(0, &c, 1) <= 0)
		{
			// Finish processing whatever is already in the
			// buffer (if anything), then stop reading
			con_eof = true;
			c = '\n';
		}
		if (c == '\n' || c == '\r')
		{
			con_text[textlen] = '\0';
			textlen = 0;
			return con_text;
		}
		else if (c == 8)
		{
			if (textlen)
			{
				textlen--;
				con_text[textlen] = '\0';
			}
			continue;
		}
		con_text[textlen] = c;
		textlen++;
		if (textlen < (int) sizeof(con_text))
			con_text[textlen] = '\0';
		else
		{
		// buffer is full
			textlen = 0;
			con_text[0] = '\0';
			Sys_Printf("\nConsole input too long!\n");
			break;
		}
	}

	return NULL;
}

void Sys_Sleep (unsigned long msecs)
{
/*	usleep (msecs * 1000);*/
	SDL_Delay (msecs);
}

void Sys_SendKeyEvents (void)
{
	IN_Commands();		//ericw -- allow joysticks to add keys so they can be used to confirm SCR_ModalMessage
	IN_SendKeyEvents();
}

void Sys_ActivateKeyFilter (qboolean active)
{
}

void *Sys_LoadLibrary (const char *path)
{
	return dlopen (path, RTLD_LAZY);
}

void *Sys_GetLibraryFunction (void *lib, const char *func)
{
	return dlsym (lib, func);
}

void Sys_CloseLibrary (void *lib)
{
	dlclose (lib);
}
