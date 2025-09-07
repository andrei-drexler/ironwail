/*
Copyright (C) 1996-2001 Id Software, Inc.
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

static void CL_FinishTimeDemo (void);

/*
==============================================================================

DEMO CODE

When a demo is playing back, all NET_SendMessages are skipped, and
NET_GetMessages are read from the demo file.

Whenever cl.time gets past the last received message, another message is
read from the demo file.
==============================================================================
*/

// from ProQuake: space to fill out the demo header for record at any time
static byte		*demo_head;
static int		*demo_head_sizes;

// Demo rewinding
typedef struct
{
	qfileofs_t		fileofs;
	unsigned short	datasize;
	byte			intermission;
	byte			forceunderwater;
} demoframe_t;

typedef struct
{
	sfx_t			*sfx;
	int				ent;
	unsigned short	channel;
	byte			volume;
	byte			attenuation;
	vec3_t			pos;
} soundevent_t;

typedef enum
{
	DFE_LIGHTSTYLE,
	DFE_CSHIFT,
	DFE_SOUND,
} framevent_t;

static struct
{
	demoframe_t		*frames;
	byte			*frame_events;
	soundevent_t	*pending_sounds;
	qboolean		backstop;

	struct
	{
		cshift_t	cshift;
		char		lightstyles[MAX_LIGHTSTYLES][MAX_STYLESTRING];
	}				prev;
}					demo_rewind;

/*
==============
CL_ClearSignons
==============
*/
void CL_ClearSignons (void)
{
	VEC_CLEAR (demo_head);
	VEC_CLEAR (demo_head_sizes);
	cls.signon = 0;
}

/*
==============
CL_StopPlayback

Called when a demo file runs out, or the user starts a game
==============
*/
void CL_StopPlayback (void)
{
	if (!cls.demoplayback)
		return;

	fclose (cls.demofile);
	cls.demoplayback = false;
	cls.demopaused = false;
	cls.demospeed = 1.f;
	cls.demofile = NULL;
	cls.demofilesize = 0;
	cls.demofilestart = 0;
	cls.demofilename[0] = '\0';
	cls.state = ca_disconnected;

	VEC_CLEAR (demo_rewind.frames);
	VEC_CLEAR (demo_rewind.frame_events);
	VEC_CLEAR (demo_rewind.pending_sounds);
	demo_rewind.backstop = false;

	if (cls.timedemo)
		CL_FinishTimeDemo ();
}

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length and view angles
====================
*/
static void CL_WriteDemoMessage (void)
{
	int	len;
	int	i;
	float	f;

	len = LittleLong (net_message.cursize);
	fwrite (&len, 4, 1, cls.demofile);
	for (i = 0; i < 3; i++)
	{
		f = LittleFloat (cl.viewangles[i]);
		fwrite (&f, 4, 1, cls.demofile);
	}
	fwrite (net_message.data, net_message.cursize, 1, cls.demofile);
	fflush (cls.demofile);
}

/*
===============
CL_AddDemoRewindSound
===============
*/
void CL_AddDemoRewindSound (int entnum, int channel, sfx_t *sfx, vec3_t pos, int vol, float atten)
{
	soundevent_t sound;

	if (entnum <= 0 || channel <= 0)
		return;

	sound.sfx			= sfx;
	sound.ent			= entnum;
	sound.channel		= channel;
	sound.volume		= vol;
	sound.attenuation	= (int) (atten + 0.5f) * 64.f;
	sound.pos[0]		= pos[0];
	sound.pos[1]		= pos[1];
	sound.pos[2]		= pos[2];

	VEC_PUSH (demo_rewind.pending_sounds, sound);
}

/*
===============
CL_UpdateDemoSpeed
===============
*/
static void CL_UpdateDemoSpeed (void)
{
	extern qboolean keydown[256];
	int adjust;

	if (key_dest != key_game)
	{
		cls.demospeed = cls.basedemospeed * !cls.demopaused;
		return;
	}

	adjust = (keydown[K_RIGHTARROW] - keydown[K_LEFTARROW] +
			  keydown[K_DPAD_RIGHT] - keydown[K_DPAD_LEFT]);

	if (adjust)
	{
		cls.demospeed = adjust * 5.f;
		if (cls.basedemospeed)
			cls.demospeed *= cls.basedemospeed;
	}
	else
	{
		cls.demospeed = cls.basedemospeed * !cls.demopaused;
	}

	if (keydown[K_SHIFT] || keydown[K_CTRL])
		cls.demospeed *= 0.25f;

	if (cls.demospeed > 0.f)
		demo_rewind.backstop = false;
}


/*
====================
CL_AdvanceTime
====================
*/
void CL_AdvanceTime (void)
{
	cl.oldtime = cl.time;

	if (cls.demoplayback)
	{
		CL_UpdateDemoSpeed ();
		cl.time += cls.demospeed * host_frametime;
		if (demo_rewind.backstop)
			cl.time = cl.mtime[0];
	}
	else
	{
		cl.time += host_frametime;
	}
}


/*
====================
CL_NextDemoFrame
====================
*/
static qboolean CL_NextDemoFrame (void)
{
	size_t		i, framecount;
	demoframe_t	*lastframe;

	VEC_CLEAR (demo_rewind.pending_sounds);

	// Forward playback
	if (cls.demospeed > 0.f)
	{
		if (cls.signon < SIGNONS)
		{
			VEC_CLEAR (demo_rewind.frames);
			VEC_CLEAR (demo_rewind.frame_events);
		}
		else
		{
			demoframe_t newframe;

			memset (&newframe, 0, sizeof (newframe));
			newframe.fileofs = Sys_ftell (cls.demofile);
			newframe.intermission = cl.intermission;
			newframe.forceunderwater = cl.forceunderwater;
			VEC_PUSH (demo_rewind.frames, newframe);

			// Take a snapshot of the tracked data at the beginning of this frame
			for (i = 0; i < MAX_LIGHTSTYLES; i++)
				q_strlcpy (demo_rewind.prev.lightstyles[i], cl_lightstyle[i].map, MAX_STYLESTRING);
			memcpy (&demo_rewind.prev.cshift, &cshift_empty, sizeof (cshift_empty));
		}
		return true;
	}

	// If we're rewinding we should always have at least one frame to go back to
	framecount = VEC_SIZE (demo_rewind.frames);
	if (!framecount)
		return false;

	lastframe = &demo_rewind.frames[framecount - 1];
	Sys_fseek (cls.demofile, lastframe->fileofs, SEEK_SET);

	if (framecount == 1)
		demo_rewind.backstop = true;

	return true;
}

/*
===============
CL_FinishDemoFrame
===============
*/
void CL_FinishDemoFrame (void)
{
	size_t		i, len, numframes;
	demoframe_t	*lastframe;

	if (!cls.demoplayback || !cls.demospeed)
		return;

	// Flush any pending stuffcmds (such as v_chifts)
	// so that they take effect this frame, not the next
	Cbuf_Execute ();

	// We're not going to rewind before the first frame,
	// so we only track state changes from the second one onwards
	numframes = VEC_SIZE (demo_rewind.frames);
	if (numframes < 2)
		return;

	lastframe = &demo_rewind.frames[numframes - 1];

	if (cls.demospeed > 0.f) // forward playback
	{
		SDL_assert (lastframe->datasize == 0);

		// Save the previous cshift value if it changed this frame
		if (memcmp (&demo_rewind.prev.cshift, &cshift_empty, sizeof (cshift_t)) != 0)
		{
			VEC_PUSH (demo_rewind.frame_events, DFE_CSHIFT);
			Vec_Append ((void**)&demo_rewind.frame_events, 1, &demo_rewind.prev.cshift, sizeof (cshift_t));
			lastframe->datasize += 1 + sizeof (cshift_t);
		}

		// Save the previous value for any changed lightstyle
		for (i = 0; i < MAX_LIGHTSTYLES; i++)
		{
			if (strcmp (demo_rewind.prev.lightstyles[i], cl_lightstyle[i].map) == 0)
				continue;
			len = strlen (demo_rewind.prev.lightstyles[i]);
			VEC_PUSH (demo_rewind.frame_events, DFE_LIGHTSTYLE);
			VEC_PUSH (demo_rewind.frame_events, (byte) i);
			VEC_PUSH (demo_rewind.frame_events, (byte) len);
			Vec_Append ((void**)&demo_rewind.frame_events, 1, demo_rewind.prev.lightstyles[i], len);
			lastframe->datasize += 3 + len;
		}

		// Play back pending sounds in reverse order
		len = VEC_SIZE (demo_rewind.pending_sounds);
		while (len > 0)
		{
			soundevent_t *snd = &demo_rewind.pending_sounds[--len];
			VEC_PUSH (demo_rewind.frame_events, DFE_SOUND);
			Vec_Append ((void**)&demo_rewind.frame_events, 1, snd, sizeof (*snd));
			lastframe->datasize += 1 + sizeof (*snd);
		}
		VEC_CLEAR (demo_rewind.pending_sounds);
	}
	else // rewinding
	{
		// Revert tracked state changes in this frame
		if (lastframe->datasize > 0)
		{
			size_t end = VEC_SIZE (demo_rewind.frame_events);
			size_t begin = end - lastframe->datasize;

			while (begin < end)
			{
				byte	*data = &demo_rewind.frame_events[begin++];
				byte	datatype = *data++;

				switch (datatype)
				{
				case DFE_LIGHTSTYLE:
					{
						char	str[MAX_STYLESTRING];
						byte	style;

						style = *data++;
						len = *data++;
						memcpy (str, data, len);
						str[len] = '\0';
						CL_SetLightstyle (style, str);

						begin += 2 + len;
					}
					break;

				case DFE_CSHIFT:
					{
						memcpy (&cshift_empty, data, sizeof (cshift_empty));
						begin += sizeof (cshift_empty);
					}
					break;

				case DFE_SOUND:
					{
						soundevent_t snd;

						memcpy (&snd, data, sizeof (snd));
						if (snd.sfx)
							S_StartSound (snd.ent, snd.channel, snd.sfx, snd.pos, snd.volume/255.0, snd.attenuation / 64.f);
						else
							S_StopSound (snd.ent, snd.channel);

						begin += sizeof (snd);
					}
					break;

				default:
					Sys_Error ("CL_NextDemoFrame: bad event type %d", datatype);
					break;
				}
			}

			SDL_assert (begin == end);

			VEC_POP_N (demo_rewind.frame_events, lastframe->datasize);
			lastframe->datasize = 0;
		}

		if (cl.intermission != lastframe->intermission && !lastframe->intermission)
			cl.completed_time = 0;
		cl.intermission = lastframe->intermission;
		cl.forceunderwater = lastframe->forceunderwater;

		VEC_POP (demo_rewind.frames);
	}
}

static int CL_GetDemoMessage (void)
{
	int		i;
	float	f;

	if (!cls.demospeed || demo_rewind.backstop)
		return 0;

	// decide if it is time to grab the next message
	if (cls.signon == SIGNONS)	// always grab until fully connected
	{
		if (cls.timedemo)
		{
			if (host_framecount == cls.td_lastframe)
				return 0;	// already read this frame's message
			cls.td_lastframe = host_framecount;
		// if this is the second frame, grab the real td_starttime
		// so the bogus time on the first frame doesn't count
			if (host_framecount == cls.td_startframe + 1)
				cls.td_starttime = realtime;
		}
		else if (/* cl.time > 0 && */ cls.demospeed > 0.f ? cl.time <= cl.mtime[0] : cl.time >= cl.mtime[0])
		{
			return 0;	// don't need another message yet
		}
	}

// get the next message
	if (!CL_NextDemoFrame ())
		return 0;

	if (fread (&net_message.cursize, 4, 1, cls.demofile) != 1)
		goto readerror;
	VectorCopy (cl.mviewangles[0], cl.mviewangles[1]);
	for (i = 0 ; i < 3 ; i++)
	{
		if (fread (&f, 4, 1, cls.demofile) != 1)
			goto readerror;
		cl.mviewangles[0][i] = LittleFloat (f);
	}

	net_message.cursize = LittleLong (net_message.cursize);
	if (net_message.cursize > MAX_MSGLEN)
		Sys_Error ("Demo message > MAX_MSGLEN");
	if (fread (net_message.data, net_message.cursize, 1, cls.demofile) != 1)
	{
	readerror:
		CL_StopPlayback ();
		return 0;
	}

	return 1;
}

/*
====================
CL_GetMessage

Handles recording and playback of demos, on top of NET_ code
====================
*/
int CL_GetMessage (void)
{
	int	r;

	if (cls.demoplayback)
		return CL_GetDemoMessage ();

	while (1)
	{
		r = NET_GetMessage (cls.netcon);

		if (r != 1 && r != 2)
			return r;

	// discard nop keepalive message
		if (net_message.cursize == 1 && net_message.data[0] == svc_nop)
			Con_Printf ("<-- server to client keepalive\n");
		else
			break;
	}

	if (cls.demorecording)
		CL_WriteDemoMessage ();

	if (cls.signon < 2)
	{
	// record messages before full connection, so that a
	// demo record can happen after connection is done
		Vec_Append ((void**)&demo_head, 1, net_message.data, net_message.cursize);
		VEC_PUSH (demo_head_sizes, net_message.cursize);
	}

	return r;
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f (void)
{
	if (cmd_source != src_command)
		return;

	if (!cls.demorecording)
	{
		Con_Printf ("Not recording a demo.\n");
		return;
	}

// write a disconnect message to the demo file
	SZ_Clear (&net_message);
	MSG_WriteByte (&net_message, svc_disconnect);
	CL_WriteDemoMessage ();

// finish up
	fclose (cls.demofile);
	cls.demofile = NULL;
	cls.demorecording = false;
	Con_Printf ("Completed demo\n");
	
// ericw -- update demo tab-completion list
	DemoList_Rebuild ();
}

/*
====================
CL_Record_f

record <demoname> <map> [cd track]
====================
*/
void CL_Record_f (void)
{
	int		c;
	char	relname[MAX_OSPATH];
	char	name[MAX_OSPATH];
	int		track;

	if (cmd_source != src_command)
		return;

	if (cls.demoplayback)
	{
		Con_Printf ("Can't record during demo playback\n");
		return;
	}

	if (cls.demorecording)
		CL_Stop_f();

	c = Cmd_Argc();
	if (c != 2 && c != 3 && c != 4)
	{
		Con_Printf ("record <demoname> [<map> [cd track]]\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		Con_Printf ("Relative pathnames are not allowed.\n");
		return;
	}

	if (c == 2 && cls.state == ca_connected)
	{
#if 0
		Con_Printf("Can not record - already connected to server\nClient demo recording must be started before connecting\n");
		return;
#endif
		if (cls.signon < 2)
		{
			Con_Printf("Can't record - try again when connected\n");
			return;
		}
	}

// write the forced cd track number, or -1
	if (c == 4)
	{
		track = atoi(Cmd_Argv(3));
		Con_Printf ("Forcing CD track to %i\n", cls.forcetrack);
	}
	else
	{
		track = -1;
	}

	// save the demo name here, before potentially loading a new map (which would change argv[1])
	q_strlcpy (relname, Cmd_Argv(1), sizeof(relname));

	// start the map up
	if (c > 2)
	{
		Cmd_ExecuteString ( va("map %s", Cmd_Argv(2)), src_command);
		if (cls.state != ca_connected)
			return;
	}

// open the demo file
	COM_AddExtension (relname, ".dem", sizeof(relname));
	q_snprintf (name, sizeof(name), "%s/%s", com_gamedir, relname);

	Con_SafePrintf ("Recording to ");
	Con_LinkPrintf (name, "%s", relname);
	Con_SafePrintf (".\n");

	cls.demofile = Sys_fopen (name, "wb");
	if (!cls.demofile)
	{
		Con_Printf ("ERROR: couldn't create %s\n", relname);
		return;
	}

	cls.forcetrack = track;
	fprintf (cls.demofile, "%i\n", cls.forcetrack);
	q_strlcpy (cls.demofilename, name, sizeof (cls.demofilename));

	cls.demorecording = true;

	// from ProQuake: initialize the demo file if we're already connected
	if (c == 2 && cls.state == ca_connected)
	{
		static byte tmpbuf[NET_MAXMESSAGE];
		byte *data = net_message.data;
		int cursize = net_message.cursize;
		int maxsize = net_message.maxsize;
		int i, count;

		net_message.data = demo_head;
		for (i = 0, count = VEC_SIZE (demo_head_sizes); i < count; i++)
		{
			net_message.cursize = demo_head_sizes[i];
			CL_WriteDemoMessage ();
			net_message.data += net_message.cursize;
		}

		net_message.data = tmpbuf;
		net_message.maxsize = sizeof (tmpbuf);
		SZ_Clear (&net_message);

		// current names, colors, and frag counts
		for (i = 0; i < cl.maxclients; i++)
		{
			MSG_WriteByte (&net_message, svc_updatename);
			MSG_WriteByte (&net_message, i);
			MSG_WriteString (&net_message, cl.scores[i].name);
			MSG_WriteByte (&net_message, svc_updatefrags);
			MSG_WriteByte (&net_message, i);
			MSG_WriteShort (&net_message, cl.scores[i].frags);
			MSG_WriteByte (&net_message, svc_updatecolors);
			MSG_WriteByte (&net_message, i);
			MSG_WriteByte (&net_message, cl.scores[i].colors);
		}

		// send all current light styles
		for (i = 0; i < MAX_LIGHTSTYLES; i++)
		{
			MSG_WriteByte (&net_message, svc_lightstyle);
			MSG_WriteByte (&net_message, i);
			MSG_WriteString (&net_message, cl_lightstyle[i].map);
		}

		//stats
		for (i = 0; i < MAX_CL_STATS; i++)
		{
			if (!cl.stats[i] && !cl.statsf[i])
				continue;

			if (net_message.cursize > 4096)
			{	//periodically flush so that large maps don't need larger than vanilla limits
				CL_WriteDemoMessage();
				SZ_Clear (&net_message);
			}

			if ((double)cl.stats[i] != cl.statsf[i] && (unsigned int)cl.stats[i] <= 0x00ffffff)
			{	//if the float representation seems to have more precision then use that, unless its getting huge in which case we're probably getting fpu truncation, so go back to more compatible ints
				MSG_WriteByte (&net_message, svc_stufftext);
				MSG_WriteString (&net_message, va ("//st %i %g\n", i, cl.statsf[i]));
			}
			else if (i >= MAX_CL_BASE_STATS)
			{
				MSG_WriteByte (&net_message, svc_stufftext);
				MSG_WriteString (&net_message, va ("//st %i %i\n", i, cl.stats[i]));
			}
			else
			{
				MSG_WriteByte (&net_message, svc_updatestat);
				MSG_WriteByte (&net_message, i);
				MSG_WriteLong (&net_message, cl.stats[i]);
			}
		}

		// what about the CD track or SVC fog... future consideration.
		MSG_WriteByte (&net_message, svc_updatestat);
		MSG_WriteByte (&net_message, STAT_TOTALSECRETS);
		MSG_WriteLong (&net_message, cl.stats[STAT_TOTALSECRETS]);

		MSG_WriteByte (&net_message, svc_updatestat);
		MSG_WriteByte (&net_message, STAT_TOTALMONSTERS);
		MSG_WriteLong (&net_message, cl.stats[STAT_TOTALMONSTERS]);

		MSG_WriteByte (&net_message, svc_updatestat);
		MSG_WriteByte (&net_message, STAT_SECRETS);
		MSG_WriteLong (&net_message, cl.stats[STAT_SECRETS]);

		MSG_WriteByte (&net_message, svc_updatestat);
		MSG_WriteByte (&net_message, STAT_MONSTERS);
		MSG_WriteLong (&net_message, cl.stats[STAT_MONSTERS]);

		// view entity
		MSG_WriteByte (&net_message, svc_setview);
		MSG_WriteShort (&net_message, cl.viewentity);

		// signon
		MSG_WriteByte (&net_message, svc_signonnum);
		MSG_WriteByte (&net_message, 3);

		CL_WriteDemoMessage();

		// restore net_message
		net_message.data = data;
		net_message.cursize = cursize;
		net_message.maxsize = maxsize;
	}
}


/*
====================
CL_PlayDemo_f

play [demoname]
====================
*/
void CL_PlayDemo_f (void)
{
	char	name[MAX_OSPATH];

	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() < 2)
	{
		Con_Printf ("playdemo <demoname> : plays a demo\n");
		return;
	}

// disconnect from server
	CL_Disconnect ();

// open the demo file
	q_strlcpy (name, Cmd_Argv(1), sizeof(name));
	COM_AddExtension (name, ".dem", sizeof(name));

	Con_Printf ("Playing demo from %s.\n", name);

	COM_FOpenFile (name, &cls.demofile, NULL);
	if (!cls.demofile)
	{
		Con_Printf ("ERROR: couldn't open %s\n", name);
		cls.demonum = -1;	// stop demo loop
		return;
	}

// ZOID, fscanf is evil
// O.S.: if a space character e.g. 0x20 (' ') follows '\n',
// fscanf skips that byte too and screws up further reads.
//	fscanf (cls.demofile, "%i\n", &cls.forcetrack);
	if (fscanf (cls.demofile, "%i", &cls.forcetrack) != 1 || fgetc (cls.demofile) != '\n')
	{
		fclose (cls.demofile);
		cls.demofile = NULL;
		cls.demonum = -1;	// stop demo loop
		Con_Printf ("ERROR: demo \"%s\" is invalid\n", name);
		return;
	}

	cls.demoplayback = true;
	cls.demopaused = false;
	cls.demospeed = 1.f;
	// Only change basedemospeed if it hasn't been initialized,
	// otherwise preserve the existing value
	if (!cls.basedemospeed)
		cls.basedemospeed = 1.f;
	q_strlcpy (cls.demofilename, name, sizeof (cls.demofilename));
	cls.state = ca_connected;
	cls.demoloop = Cmd_Argc () >= 3 ? Q_atoi (Cmd_Argv (2)) != 0 : false;
	cls.demofilestart = Sys_ftell (cls.demofile);
	cls.demofilesize = com_filesize;

// if this is a player-initiated demo, get rid of the console
	if (cls.demonum == -1 && key_dest == key_console)
		key_dest = key_game;
}

/*
====================
CL_FinishTimeDemo

====================
*/
static void CL_FinishTimeDemo (void)
{
	int	frames;
	float	time;

	cls.timedemo = false;

// the first frame didn't count
	frames = (host_framecount - cls.td_startframe) - 1;
	time = realtime - cls.td_starttime;
	if (!time)
		time = 1;
	Con_Printf ("%i frames %5.1f seconds %5.1f fps\n", frames, time, frames/time);
}

/*
====================
CL_TimeDemo_f

timedemo [demoname]
====================
*/
void CL_TimeDemo_f (void)
{
	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("timedemo <demoname> : gets demo speeds\n");
		return;
	}

	CL_PlayDemo_f ();
	if (!cls.demofile)
		return;

// cls.td_starttime will be grabbed at the second frame of the demo, so
// all the loading time doesn't get counted

	cls.timedemo = true;
	cls.td_startframe = host_framecount;
	cls.td_lastframe = -1;	// get a new message this frame
}

