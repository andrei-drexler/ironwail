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
// sbar.c -- status bar code

#include "quakedef.h"

static int		sb_updates;		// if >= vid.numpages, no update needed

#define STAT_MINUS		10	// num frame for '-' stats digit

#define SBAR2_MARGIN_X	16
#define SBAR2_MARGIN_Y	10

static qpic_t		*sb_nums[2][11];
static qpic_t		*sb_colon, *sb_slash;
static qpic_t		*sb_ibar;
static qpic_t		*sb_sbar;
static qpic_t		*sb_scorebar;

static qpic_t		*sb_weapons[7][8];   // 0 is active, 1 is owned, 2-5 are flashes
static qpic_t		*sb_ammo[4];
static qpic_t		*sb_sigil[4];
static qpic_t		*sb_armor[3];
static qpic_t		*sb_items[32];

static qpic_t		*sb_faces[7][2];	// 0 is gibbed, 1 is dead, 2-6 are alive
							// 0 is static, 1 is temporary animation
static qpic_t		*sb_face_invis;
static qpic_t		*sb_face_quad;
static qpic_t		*sb_face_invuln;
static qpic_t		*sb_face_invis_invuln;

static qboolean	sb_showscores;

int		sb_lines;			// scan lines to draw

static qpic_t		*rsb_invbar[2];
static qpic_t		*rsb_weapons[5];
static qpic_t		*rsb_items[2];
static qpic_t		*rsb_ammo[3];
static qpic_t		*rsb_teambord;		// PGM 01/19/97 - team color border

//MED 01/04/97 added two more weapons + 3 alternates for grenade launcher
static qpic_t		*hsb_weapons[7][5];   // 0 is active, 1 is owned, 2-5 are flashes
//MED 01/04/97 added array to simplify weapon parsing
static int		hipweapons[4] = {HIT_LASER_CANNON_BIT,HIT_MJOLNIR_BIT,4,HIT_PROXIMITY_GUN_BIT};
//MED 01/04/97 added hipnotic items array
static qpic_t		*hsb_items[2];

void Sbar_MiniDeathmatchOverlay (void);
void Sbar_DeathmatchOverlay (void);
void M_DrawPic (int x, int y, qpic_t *pic);

/*
===============
Sbar_ShowScores

Tab key down
===============
*/
void Sbar_ShowScores (void)
{
	if (sb_showscores)
		return;
	sb_showscores = true;
	sb_updates = 0;
}

/*
===============
Sbar_DontShowScores

Tab key up
===============
*/
void Sbar_DontShowScores (void)
{
	sb_showscores = false;
	sb_updates = 0;
}

/*
===============
Sbar_Changed
===============
*/
void Sbar_Changed (void)
{
	sb_updates = 0;	// update next frame
}

/*
===============
Sbar_LoadPics -- johnfitz -- load all the sbar pics
===============
*/
void Sbar_LoadPics (void)
{
	int		i;

	for (i = 0; i < 10; i++)
	{
		sb_nums[0][i] = Draw_PicFromWad (va("num_%i",i));
		sb_nums[1][i] = Draw_PicFromWad (va("anum_%i",i));
	}

	sb_nums[0][10] = Draw_PicFromWad ("num_minus");
	sb_nums[1][10] = Draw_PicFromWad ("anum_minus");

	sb_colon = Draw_PicFromWad ("num_colon");
	sb_slash = Draw_PicFromWad ("num_slash");

	sb_weapons[0][0] = Draw_PicFromWad ("inv_shotgun");
	sb_weapons[0][1] = Draw_PicFromWad ("inv_sshotgun");
	sb_weapons[0][2] = Draw_PicFromWad ("inv_nailgun");
	sb_weapons[0][3] = Draw_PicFromWad ("inv_snailgun");
	sb_weapons[0][4] = Draw_PicFromWad ("inv_rlaunch");
	sb_weapons[0][5] = Draw_PicFromWad ("inv_srlaunch");
	sb_weapons[0][6] = Draw_PicFromWad ("inv_lightng");

	sb_weapons[1][0] = Draw_PicFromWad ("inv2_shotgun");
	sb_weapons[1][1] = Draw_PicFromWad ("inv2_sshotgun");
	sb_weapons[1][2] = Draw_PicFromWad ("inv2_nailgun");
	sb_weapons[1][3] = Draw_PicFromWad ("inv2_snailgun");
	sb_weapons[1][4] = Draw_PicFromWad ("inv2_rlaunch");
	sb_weapons[1][5] = Draw_PicFromWad ("inv2_srlaunch");
	sb_weapons[1][6] = Draw_PicFromWad ("inv2_lightng");

	for (i = 0; i < 5; i++)
	{
		sb_weapons[2+i][0] = Draw_PicFromWad (va("inva%i_shotgun",i+1));
		sb_weapons[2+i][1] = Draw_PicFromWad (va("inva%i_sshotgun",i+1));
		sb_weapons[2+i][2] = Draw_PicFromWad (va("inva%i_nailgun",i+1));
		sb_weapons[2+i][3] = Draw_PicFromWad (va("inva%i_snailgun",i+1));
		sb_weapons[2+i][4] = Draw_PicFromWad (va("inva%i_rlaunch",i+1));
		sb_weapons[2+i][5] = Draw_PicFromWad (va("inva%i_srlaunch",i+1));
		sb_weapons[2+i][6] = Draw_PicFromWad (va("inva%i_lightng",i+1));
	}

	sb_ammo[0] = Draw_PicFromWad ("sb_shells");
	sb_ammo[1] = Draw_PicFromWad ("sb_nails");
	sb_ammo[2] = Draw_PicFromWad ("sb_rocket");
	sb_ammo[3] = Draw_PicFromWad ("sb_cells");

	sb_armor[0] = Draw_PicFromWad ("sb_armor1");
	sb_armor[1] = Draw_PicFromWad ("sb_armor2");
	sb_armor[2] = Draw_PicFromWad ("sb_armor3");

	sb_items[0] = Draw_PicFromWad ("sb_key1");
	sb_items[1] = Draw_PicFromWad ("sb_key2");
	sb_items[2] = Draw_PicFromWad ("sb_invis");
	sb_items[3] = Draw_PicFromWad ("sb_invuln");
	sb_items[4] = Draw_PicFromWad ("sb_suit");
	sb_items[5] = Draw_PicFromWad ("sb_quad");

	sb_sigil[0] = Draw_PicFromWad ("sb_sigil1");
	sb_sigil[1] = Draw_PicFromWad ("sb_sigil2");
	sb_sigil[2] = Draw_PicFromWad ("sb_sigil3");
	sb_sigil[3] = Draw_PicFromWad ("sb_sigil4");

	sb_faces[4][0] = Draw_PicFromWad ("face1");
	sb_faces[4][1] = Draw_PicFromWad ("face_p1");
	sb_faces[3][0] = Draw_PicFromWad ("face2");
	sb_faces[3][1] = Draw_PicFromWad ("face_p2");
	sb_faces[2][0] = Draw_PicFromWad ("face3");
	sb_faces[2][1] = Draw_PicFromWad ("face_p3");
	sb_faces[1][0] = Draw_PicFromWad ("face4");
	sb_faces[1][1] = Draw_PicFromWad ("face_p4");
	sb_faces[0][0] = Draw_PicFromWad ("face5");
	sb_faces[0][1] = Draw_PicFromWad ("face_p5");

	sb_face_invis = Draw_PicFromWad ("face_invis");
	sb_face_invuln = Draw_PicFromWad ("face_invul2");
	sb_face_invis_invuln = Draw_PicFromWad ("face_inv2");
	sb_face_quad = Draw_PicFromWad ("face_quad");

	sb_sbar = Draw_PicFromWad ("sbar");
	sb_ibar = Draw_PicFromWad ("ibar");
	sb_scorebar = Draw_PicFromWad ("scorebar");

//MED 01/04/97 added new hipnotic weapons
	if (hipnotic)
	{
		hsb_weapons[0][0] = Draw_PicFromWad ("inv_laser");
		hsb_weapons[0][1] = Draw_PicFromWad ("inv_mjolnir");
		hsb_weapons[0][2] = Draw_PicFromWad ("inv_gren_prox");
		hsb_weapons[0][3] = Draw_PicFromWad ("inv_prox_gren");
		hsb_weapons[0][4] = Draw_PicFromWad ("inv_prox");

		hsb_weapons[1][0] = Draw_PicFromWad ("inv2_laser");
		hsb_weapons[1][1] = Draw_PicFromWad ("inv2_mjolnir");
		hsb_weapons[1][2] = Draw_PicFromWad ("inv2_gren_prox");
		hsb_weapons[1][3] = Draw_PicFromWad ("inv2_prox_gren");
		hsb_weapons[1][4] = Draw_PicFromWad ("inv2_prox");

		for (i = 0; i < 5; i++)
		{
			hsb_weapons[2+i][0] = Draw_PicFromWad (va("inva%i_laser",i+1));
			hsb_weapons[2+i][1] = Draw_PicFromWad (va("inva%i_mjolnir",i+1));
			hsb_weapons[2+i][2] = Draw_PicFromWad (va("inva%i_gren_prox",i+1));
			hsb_weapons[2+i][3] = Draw_PicFromWad (va("inva%i_prox_gren",i+1));
			hsb_weapons[2+i][4] = Draw_PicFromWad (va("inva%i_prox",i+1));
		}

		hsb_items[0] = Draw_PicFromWad ("sb_wsuit");
		hsb_items[1] = Draw_PicFromWad ("sb_eshld");
	}

	if (rogue)
	{
		rsb_invbar[0] = Draw_PicFromWad ("r_invbar1");
		rsb_invbar[1] = Draw_PicFromWad ("r_invbar2");

		rsb_weapons[0] = Draw_PicFromWad ("r_lava");
		rsb_weapons[1] = Draw_PicFromWad ("r_superlava");
		rsb_weapons[2] = Draw_PicFromWad ("r_gren");
		rsb_weapons[3] = Draw_PicFromWad ("r_multirock");
		rsb_weapons[4] = Draw_PicFromWad ("r_plasma");

		rsb_items[0] = Draw_PicFromWad ("r_shield1");
		rsb_items[1] = Draw_PicFromWad ("r_agrav1");

// PGM 01/19/97 - team color border
		rsb_teambord = Draw_PicFromWad ("r_teambord");
// PGM 01/19/97 - team color border

		rsb_ammo[0] = Draw_PicFromWad ("r_ammolava");
		rsb_ammo[1] = Draw_PicFromWad ("r_ammomulti");
		rsb_ammo[2] = Draw_PicFromWad ("r_ammoplasma");
	}
}

/*
===============
Sbar_Init -- johnfitz -- rewritten
===============
*/
void Sbar_Init (void)
{
	Cmd_AddCommand ("+showscores", Sbar_ShowScores);
	Cmd_AddCommand ("-showscores", Sbar_DontShowScores);

	Sbar_LoadPics ();
}


//=============================================================================

// drawing routines are relative to the status bar location

/*
=============
Sbar_DrawPic -- johnfitz -- rewritten now that GL_SetCanvas is doing the work
=============
*/
void Sbar_DrawPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x, y + 24, pic);
}

/*
=============
Sbar_DrawPicAlpha -- johnfitz
=============
*/
void Sbar_DrawPicAlpha (int x, int y, qpic_t *pic, float alpha)
{
	GL_SetCanvasColor(1,1,1,alpha);
	Draw_Pic (x, y + 24, pic);
	GL_SetCanvasColor(1,1,1,1); // ericw -- changed from glColor3f to work around intel 855 bug with "r_oldwater 0" and "scr_sbaralpha 0"
}

/*
================
Sbar_DrawCharacter -- johnfitz -- rewritten now that GL_SetCanvas is doing the work
================
*/
void Sbar_DrawCharacter (int x, int y, int num)
{
	Draw_Character (x, y + 24, num);
}

/*
================
Sbar_DrawString -- johnfitz -- rewritten now that GL_SetCanvas is doing the work
================
*/
void Sbar_DrawString (int x, int y, const char *str)
{
	Draw_String (x, y + 24, str);
}

/*
===============
Sbar_DrawScrollString -- johnfitz

scroll the string inside a glscissor region
===============
*/
void Sbar_DrawScrollString (int x, int y, int width, const char *str)
{
	int len, ofs;

	Draw_SetClipRect (x, glcanvas.top, width, glcanvas.bottom - glcanvas.top);

	len = strlen(str)*8 + 40;
	ofs = ((int)(realtime*30))%len;
	Sbar_DrawString (x - ofs, y, str);
	Sbar_DrawCharacter (x - ofs + len - 32, y, '/');
	Sbar_DrawCharacter (x - ofs + len - 24, y, '/');
	Sbar_DrawCharacter (x - ofs + len - 16, y, '/');
	Sbar_DrawString (x - ofs + len, y, str);

	Draw_ResetClipping ();
}

/*
=============
Sbar_itoa
=============
*/
int Sbar_itoa (int num, char *buf)
{
	char	*str;
	int	pow10;
	int	dig;

	str = buf;

	if (num < 0)
	{
		*str++ = '-';
		num = -num;
	}

	for (pow10 = 10 ; num >= pow10 ; pow10 *= 10)
		;

	do
	{
		pow10 /= 10;
		dig = num/pow10;
		*str++ = '0'+dig;
		num -= dig*pow10;
	} while (pow10 != 1);

	*str = 0;

	return str-buf;
}


/*
=============
Sbar_DrawNum
=============
*/
void Sbar_DrawNum (int x, int y, int num, int digits, int color)
{
	char	str[12];
	char	*ptr;
	int	l, frame;

	num = q_min(999,num); //johnfitz -- cap high values rather than truncating number

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);
	if (l < digits)
		x += (digits-l)*24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Sbar_DrawPic (x,y,sb_nums[color][frame]); //johnfitz -- DrawTransPic is obsolete
		x += 24;
		ptr++;
	}
}

/*
=============
Sbar_DrawSmallNum
=============
*/
void Sbar_DrawSmallNum (int x, int y, int num)
{
	char	str[12];
	int		i;

	sprintf (str, "%3i", CLAMP (0, num, 999));
	for (i = 0; i < 3; i++)
		if (str[i] != ' ')
			Sbar_DrawCharacter (x + i*8, y, 18 + str[i] - '0');
}

//=============================================================================

int		fragsort[MAX_SCOREBOARD];
int		scoreboardlines;

/*
===============
Sbar_SortFrags
===============
*/
void Sbar_SortFrags (void)
{
	int		i, j, k;

// sort by frags
	scoreboardlines = 0;
	for (i = 0; i < cl.maxclients; i++)
	{
		if (cl.scores[i].name[0])
		{
			fragsort[scoreboardlines] = i;
			scoreboardlines++;
		}
	}

	for (i = 0; i < scoreboardlines; i++)
	{
		for (j = 0; j < scoreboardlines - 1 - i; j++)
		{
			if (cl.scores[fragsort[j]].frags < cl.scores[fragsort[j+1]].frags)
			{
				k = fragsort[j];
				fragsort[j] = fragsort[j+1];
				fragsort[j+1] = k;
			}
		}
	}
}

int	Sbar_ColorForMap (int m)
{
	return m < 128 ? m + 8 : m + 8;
}

/*
===============
Sbar_SoloScoreboard -- johnfitz -- new layout
===============
*/
void Sbar_SoloScoreboard (void)
{
	char	str[256];
	int	minutes, seconds, tens, units;
	int	left, right, len;

	sprintf (str,"Kills: %i/%i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
	left = 8 + strlen (str) * 8;
	Sbar_DrawString (8, 12, str);

	sprintf (str,"Secrets: %i/%i", cl.stats[STAT_SECRETS], cl.stats[STAT_TOTALSECRETS]);
	right = 312 - strlen (str) * 8;
	Sbar_DrawString (right, 12, str);

	if (!fitzmode)
	{ /* QuakeSpasm customization: */
		q_snprintf (str, sizeof(str), "skill %i", (int)(skill.value + 0.5));
		Sbar_DrawString ((left + right) / 2 - strlen (str) * 4, 12, str);

		if (cl.levelname[0])
		{
			char cleanname[sizeof (cl.levelname)];
			Mod_SanitizeMapDescription (cleanname, sizeof (cleanname), cl.levelname);
			q_snprintf (str, sizeof (str), "%s (%s)", cleanname, cl.mapname);
		}
		else
			q_strlcpy (str, cl.mapname, sizeof(str));
		len = strlen (str);
		if (len > 40)
			Sbar_DrawScrollString (0, 4, 320, str);
		else
			Sbar_DrawString (160 - len*4, 4, str);
		return;
	}
	minutes = cl.time / 60;
	seconds = cl.time - 60*minutes;
	tens = seconds / 10;
	units = seconds - 10*tens;
	sprintf (str,"%i:%i%i", minutes, tens, units);
	Sbar_DrawString ((left + right)/2 - strlen(str)*4, 12, str);

	if (cl.levelname[0])
		len = Mod_SanitizeMapDescription (str, sizeof (str), cl.levelname);
	else
		len = q_strlcpy (str, cl.mapname, sizeof(str));
	if (len > 40)
		Sbar_DrawScrollString (0, 4, 320, str);
	else
		Sbar_DrawString (160 - len*4, 4, str);
}

/*
===============
Sbar_DrawScoreboard
===============
*/
void Sbar_DrawScoreboard (void)
{
	Sbar_SoloScoreboard ();
	if (cl.gametype == GAME_DEATHMATCH)
		Sbar_DeathmatchOverlay ();
}

/*
===============
Sbar_MiniScoreboardSizeCheck
===============
*/
int Sbar_MiniScoreboardSizeCheck(void)
{
	float scale;

	scale = CLAMP(1.0f, scr_sbarscale.value, (float)glwidth / 320.0f); //johnfitz

	//MAX_SCOREBOARDNAME = 32, so total width for this overlay plus sbar is 632, but we can cut off some i guess
	if (glwidth / scale < 512 || scr_viewsize.value >= 120) //johnfitz -- test should consider scr_sbarscale
		return 0;

	return 1;
}

//=============================================================================

/*
===============
Sbar_InventoryBarPic
===============
*/
qpic_t *Sbar_InventoryBarPic (void)
{
	if (rogue)
		return rsb_invbar[cl.stats[STAT_ACTIVEWEAPON] < RIT_LAVA_NAILGUN];
	return sb_ibar;
}

/*
===============
Sbar_DrawInventory
===============
*/
void Sbar_DrawInventory (void)
{
	int		i;
	float	time;
	int	flashon;

	Sbar_DrawPicAlpha (0, -24, Sbar_InventoryBarPic (), scr_sbaralpha.value); //johnfitz -- scr_sbaralpha

// weapons
	for (i = 0; i < 7; i++)
	{
		if (cl.items & (IT_SHOTGUN<<i) )
		{
			time = cl.item_gettime[i];
			flashon = (int)((cl.time - time)*10);
			if (flashon >= 10)
			{
				if ( cl.stats[STAT_ACTIVEWEAPON] == (IT_SHOTGUN<<i)  )
					flashon = 1;
				else
					flashon = 0;
			}
			else
				flashon = (flashon%5) + 2;

			Sbar_DrawPic (i*24, -16, sb_weapons[flashon][i]);

			if (flashon > 1)
				sb_updates = 0;		// force update to remove flash
		}
	}

// MED 01/04/97
// hipnotic weapons
	if (hipnotic)
	{
		int grenadeflashing = 0;
		for (i = 0; i < 4; i++)
		{
			if (cl.items & (1<<hipweapons[i]))
			{
				time = cl.item_gettime[hipweapons[i]];
				flashon = (int)((cl.time - time)*10);
				if (flashon >= 10)
				{
					if (cl.stats[STAT_ACTIVEWEAPON] == (1<<hipweapons[i]))
						flashon = 1;
					else
						flashon = 0;
				}
				else
					flashon = (flashon%5) + 2;

				// check grenade launcher
				if (i == 2)
				{
					if (cl.items & HIT_PROXIMITY_GUN)
					{
						if (flashon)
						{
							grenadeflashing = 1;
							Sbar_DrawPic (96, -16, hsb_weapons[flashon][2]);
						}
					}
				}
				else if (i == 3)
				{
					if (cl.items & (IT_SHOTGUN<<4))
					{
						if (flashon && !grenadeflashing)
						{
							Sbar_DrawPic (96, -16, hsb_weapons[flashon][3]);
						}
						else if (!grenadeflashing)
						{
							Sbar_DrawPic (96, -16, hsb_weapons[0][3]);
						}
					}
					else
						Sbar_DrawPic (96, -16, hsb_weapons[flashon][4]);
				}
				else
					Sbar_DrawPic (176 + (i*24), -16, hsb_weapons[flashon][i]);

				if (flashon > 1)
					sb_updates = 0;	// force update to remove flash
			}
		}
	}

	if (rogue)
	{
    // check for powered up weapon.
		if ( cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN )
		{
			for (i=0;i<5;i++)
			{
				if (cl.stats[STAT_ACTIVEWEAPON] == (RIT_LAVA_NAILGUN << i))
				{
					Sbar_DrawPic ((i+2)*24, -16, rsb_weapons[i]);
				}
			}
		}
	}

// ammo counts
	for (i = 0; i < 4; i++)
		Sbar_DrawSmallNum (48*i + 10, -24, cl.stats[STAT_SHELLS+i]);

	flashon = 0;
	// items
	for (i = 0; i < 6; i++)
	{
		if (cl.items & (1<<(17+i)))
		{
			time = cl.item_gettime[17+i];
			if (time && time > cl.time - 2 && flashon)
			{	// flash frame
				sb_updates = 0;
			}
			else
			{
				//MED 01/04/97 changed keys
				if (!hipnotic || (i > 1))
				{
					Sbar_DrawPic (192 + i*16, -16, sb_items[i]);
				}
			}
			if (time && time > cl.time - 2)
				sb_updates = 0;
		}
	}
	//MED 01/04/97 added hipnotic items
	// hipnotic items
	if (hipnotic)
	{
		for (i = 0; i < 2; i++)
		{
			if (cl.items & (1<<(24+i)))
			{
				time = cl.item_gettime[24+i];
				if (time && time > cl.time - 2 && flashon )
				{	// flash frame
					sb_updates = 0;
				}
				else
				{
					Sbar_DrawPic (288 + i*16, -16, hsb_items[i]);
				}
				if (time && time > cl.time - 2)
					sb_updates = 0;
			}
		}
	}

	if (rogue)
	{
	// new rogue items
		for (i = 0; i < 2; i++)
		{
			if (cl.items & (1<<(29+i)))
			{
				time = cl.item_gettime[29+i];
				if (time && time > cl.time - 2 && flashon)
				{	// flash frame
					sb_updates = 0;
				}
				else
				{
					Sbar_DrawPic (288 + i*16, -16, rsb_items[i]);
				}
				if (time && time > cl.time - 2)
					sb_updates = 0;
			}
		}
	}
	else
	{
	// sigils
		for (i = 0; i < 4; i++)
		{
			if (cl.items & (1<<(28+i)))
			{
				time = cl.item_gettime[28+i];
				if (time && time > cl.time - 2 && flashon)
				{	// flash frame
					sb_updates = 0;
				}
				else
					Sbar_DrawPic (320-32 + i*8, -16, sb_sigil[i]);
				if (time && time > cl.time - 2)
					sb_updates = 0;
			}
		}
	}
}

/*
====================
Sbar_DrawInventoryQW
====================
*/
void Sbar_DrawInventoryQW (void)
{
	int		i;
	float	time;
	int	flashon;
	int scoreboard_y_gap = 0;
	qpic_t	*pic;

	pic = Sbar_InventoryBarPic();

	// handle sigil overlap w/ side scoreboard
	if (cl.gametype == GAME_DEATHMATCH && Sbar_MiniScoreboardSizeCheck() )
	{
		if (rogue || hipnotic)
			scoreboard_y_gap = 0; // no sigils so can move weps down a bit
		else
			scoreboard_y_gap = 24;
	}

	// draw weapons, ammo, sigils on right side if full hud
	if(scr_viewsize.value < 110)
	{
		int extraguns = 2 * hipnotic;

		GL_SetCanvas (CANVAS_SBAR_QW_INV);

		// weapons
		for (i = 0; i < 7; i++)
		{
			if (cl.items & (IT_SHOTGUN<<i) )
			{
				time = cl.item_gettime[i];
				flashon = (int)((cl.time - time)*10);
				if (flashon >= 10)
				{
					if ( cl.stats[STAT_ACTIVEWEAPON] == (IT_SHOTGUN<<i)  )
						flashon = 1;
					else
						flashon = 0;
				}
				else
					flashon = (flashon%5) + 2;

				Sbar_DrawPic (24, -69 - scoreboard_y_gap - (16 * (7 + extraguns) ) + i*16, sb_weapons[flashon][i]);

				if (flashon > 1)
					sb_updates = 0;		// force update to remove flash
			}
		}

		// MED 01/04/97
		// hipnotic weapons
		if (hipnotic)
		{
			int grenadeflashing = 0;
			for (i = 0; i < 4; i++)
			{
				if (cl.items & (1<<hipweapons[i]))
				{
					time = cl.item_gettime[hipweapons[i]];
					flashon = (int)((cl.time - time)*10);
					if (flashon >= 10)
					{
						if (cl.stats[STAT_ACTIVEWEAPON] == (1<<hipweapons[i]))
							flashon = 1;
						else
							flashon = 0;
					}
					else
						flashon = (flashon%5) + 2;

					// check grenade launcher
					if (i == 2)
					{
						if (cl.items & HIT_PROXIMITY_GUN)
						{
							if (flashon)
							{
								grenadeflashing = 1;
								Sbar_DrawPic(24, -69 - scoreboard_y_gap - (16 * 9) + 4 * 16, hsb_weapons[flashon][2]);
							}
						}
					}
					else if (i == 3)
					{
						if (cl.items & (IT_SHOTGUN<<4))
						{
							if (flashon && !grenadeflashing)
							{
								Sbar_DrawPic(24, -69 - scoreboard_y_gap - (16 * 9) + 4 * 16, hsb_weapons[flashon][3]);
							}
							else if (!grenadeflashing)
							{
								Sbar_DrawPic(24, -69 - scoreboard_y_gap - (16 * 9) + 4 * 16, hsb_weapons[0][3]);
							}
						}
						else
							Sbar_DrawPic(24, -69 - scoreboard_y_gap - (16 * 9) + 4 * 16, hsb_weapons[flashon][4]);
					}
					else
						Sbar_DrawPic(24, -69 - scoreboard_y_gap - (16 * 9) + (7 + i) * 16, hsb_weapons[flashon][i]);

					if (flashon > 1)
						sb_updates = 0;	// force update to remove flash
				}
			}
		}

		if (rogue)
		{
		// check for powered up weapon.
			if ( cl.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN )
			{
				for (i=0;i<5;i++)
				{
					if (cl.stats[STAT_ACTIVEWEAPON] == (RIT_LAVA_NAILGUN << i))
					{
						Sbar_DrawPic (24, -69 - scoreboard_y_gap - (16 * 7) + (i+2) * 16, rsb_weapons[i]);
					}
				}
			}
		}

		// ammo counts bgs
		if (rogue)	// ammo bar bg is slightly wider
		{
			for (i = 0; i < 4; i++)
			{
				Draw_SubPic(4, -45 - scoreboard_y_gap + (i * 11), 44, 11, pic, 1 / 320.f + i * (48 / 320.f), 0.f, 44 / 320.f, 11 / 24.f, NULL, scr_sbaralpha.value);
			}
		}
		else
		{
			for (i = 0; i < 4; i++)
			{
				Draw_SubPic(6, -45 - scoreboard_y_gap + (i * 11), 42 , 11, pic, 3 / 320.f + i * (48 / 320.f), 0.f, 42 / 320.f, 11 / 24.f, NULL, scr_sbaralpha.value);
			}
		}

		// ammo counts
		for (i = 0; i < 4; i++)
		{
			Sbar_DrawSmallNum(13, -69 - scoreboard_y_gap + (i * 11), cl.stats[STAT_SHELLS + i]);
		}

		if (!rogue)
		{
			flashon = 0;

			// sigils bg - hide if none
			int has_a_sigil = 0;

			for (i = 0; i < 4; i++)
			{
				if (cl.items & (1<<(28+i)))
					has_a_sigil = 1;
			}

			if (has_a_sigil)
				Draw_SubPic (16, -16 - scoreboard_y_gap + 24, 32, 16, sb_ibar, 1.f-32/320.f, 8/24.f, 32/320.f, 16/24.f, NULL, 1.f);

			// sigils
			for (i = 0; i < 4; i++)
			{
				if (cl.items & (1<<(28+i)))
				{
					time = cl.item_gettime[28+i];
					if (time && time > cl.time - 2 && flashon)
					{	// flash frame
						sb_updates = 0;
					}
					else
						Sbar_DrawPic (16 + i*8, -16 - scoreboard_y_gap, sb_sigil[i]);
					if (time && time > cl.time - 2)
						sb_updates = 0;
				}
			}
		}
	}

	// draw items/powerups with main hud
	GL_SetCanvas (CANVAS_SBAR);

	flashon = 0;
	// items
	for (i = 0; i < 6; i++)
	{
		if (cl.items & (1<<(17+i)))
		{
			time = cl.item_gettime[17+i];
			if (time && time > cl.time - 2 && flashon)
			{	// flash frame
				sb_updates = 0;
			}
			else
			{
				//MED 01/04/97 changed keys
				if (!hipnotic || (i > 1))
				{
					Sbar_DrawPic (192 + i*16, -16, sb_items[i]);
				}
			}
			if (time && time > cl.time - 2)
				sb_updates = 0;
		}
	}
	//MED 01/04/97 added hipnotic items
	// hipnotic items
	if (hipnotic)
	{
		for (i = 0; i < 2; i++)
		{
			if (cl.items & (1<<(24+i)))
			{
				time = cl.item_gettime[24+i];
				if (time && time > cl.time - 2 && flashon )
				{	// flash frame
					sb_updates = 0;
				}
				else
				{
					Sbar_DrawPic (288 + i*16, -16, hsb_items[i]);
				}
				if (time && time > cl.time - 2)
					sb_updates = 0;
			}
		}
	}

	if (rogue)
	{
		// new rogue items
		for (i = 0; i < 2; i++)
		{
			if (cl.items & (1<<(29+i)))
			{
				time = cl.item_gettime[29+i];
				if (time && time > cl.time - 2 && flashon)
				{	// flash frame
					sb_updates = 0;
				}
				else
				{
					Sbar_DrawPic (288 + i*16, -16, rsb_items[i]);
				}
				if (time && time > cl.time - 2)
					sb_updates = 0;
			}
		}
	}
}

/*
===============
Sbar_DrawInventory2
===============
*/
void Sbar_DrawInventory2 (void)
{
	int		i;
	float	x, y, time;
	int		flashon;
	qpic_t	*pic;

	// weapons
	if (scr_viewsize.value < 110)
	{
		const int ROW_HEIGHT = 16;
		x = (int)(glcanvas.right + 1 + 0.5f);
		y = (int)(LERP (glcanvas.top, glcanvas.bottom - 148, 0.5f) + ROW_HEIGHT * 7 * 0.5f + 0.5f);

		if (hipnotic)
			y += 12; // move down a bit to accomodate the extra weapons

		for (i = 0; i < 7; i++)
		{
			if (hipnotic && i == IT_GRENADE_LAUNCHER)
				continue;

			if (cl.items & (IT_SHOTGUN<<i))
			{
				qboolean active = (cl.stats[STAT_ACTIVEWEAPON] == (IT_SHOTGUN<<i));
				time = cl.item_gettime[i];
				flashon = (int)((cl.time - time)*10);
				if (flashon >= 10)
					flashon = active;
				else
					flashon = (flashon%5) + 2;

				if (rogue && i >= 2 && cl.stats[STAT_ACTIVEWEAPON] == (RIT_LAVA_NAILGUN << (i - 2)))
				{
					// powered up weapon
					pic = rsb_weapons[i - 2];
					active = true;
				}
				else
					pic = sb_weapons[flashon][i];
				Sbar_DrawPic (x - (active ? 24 : 18), y - ROW_HEIGHT * i, pic);

				if (flashon > 1)
					sb_updates = 0;		// force update to remove flash
			}
		}

		if (hipnotic) // hipnotic weapons
		{
			int grenadeflashing = 0;
			for (i = 0; i < 4; i++)
			{
				if (cl.items & (1<<hipweapons[i]))
				{
					qboolean active = (cl.stats[STAT_ACTIVEWEAPON] == (1<<hipweapons[i]));
					time = cl.item_gettime[hipweapons[i]];
					flashon = (int)((cl.time - time)*10);
					if (flashon >= 10)
						flashon = active;
					else
						flashon = (flashon%5) + 2;

					// check grenade launcher
					if (i == 2)
					{
						if (cl.items & HIT_PROXIMITY_GUN)
						{
							if (flashon)
							{
								grenadeflashing = 1;
								Sbar_DrawPic (x - (active ? 24 : 18), y - ROW_HEIGHT * 4, hsb_weapons[flashon][2]);
							}
						}
					}
					else if (i == 3)
					{
						if (cl.items & (IT_SHOTGUN<<4))
						{
							if (flashon && !grenadeflashing)
							{
								Sbar_DrawPic (x - (active ? 24 : 18), y - ROW_HEIGHT * 4, hsb_weapons[flashon][3]);
							}
							else if (!grenadeflashing)
							{
								Sbar_DrawPic (x - (active ? 24 : 18), y - ROW_HEIGHT * 4, hsb_weapons[0][3]);
							}
						}
						else
							Sbar_DrawPic (x - (active ? 24 : 18), y - ROW_HEIGHT * 4, hsb_weapons[flashon][4]);
					}
					else
						Sbar_DrawPic (x - (active ? 24 : 18), y - ROW_HEIGHT * (i + 7), hsb_weapons[flashon][i]);

					if (flashon > 1)
						sb_updates = 0;	// force update to remove flash
				}
			}
		}
	}

	// ammo counts
	if (scr_viewsize.value < 110)
	{
		pic = Sbar_InventoryBarPic ();

		if (hudstyle == HUD_MODERN_SIDEAMMO || hudstyle == HUD_QUAKEWORLD) // right side, 2x2
		{
			const int ITEM_WIDTH = 52;
			x = (int)(glcanvas.right - SBAR2_MARGIN_X - ITEM_WIDTH * 2 + 0.5f);
			y = (int)(glcanvas.bottom - SBAR2_MARGIN_Y - 60 + 0.5f);

			for (i = 0; i < 2; i++)
				Draw_SubPic (x, y + 24 - 10 * i, ITEM_WIDTH*2, 10, pic, i * (2*48/320.f), 0.f, 2*48/320.f, 10/24.f, NULL, scr_sbaralpha.value);

			for (i = 0; i < 4; i++)
				Sbar_DrawSmallNum (x + 11 + ITEM_WIDTH * (i&1), y - 10 * (i>>1), cl.stats[STAT_SHELLS+i]);
		}
		else // bottom center, 4x1
		{
			x = (int)((glcanvas.right + glcanvas.left) * 0.5f + 0.5f) - 96;
			y = (int)(glcanvas.bottom - 9 + 0.5f);
			Draw_SubPic (x, y, 192, 10, pic, 0.f, 0.f, 192/320.f, 10/24.f, NULL, scr_sbaralpha.value);
			for (i = 0; i < 4; i++)
				Sbar_DrawSmallNum (x + 10 + 48*i, y - 24, cl.stats[STAT_SHELLS+i]);
		}
	}

	// items
	if (scr_viewsize.value < 110 && (hudstyle == HUD_MODERN_SIDEAMMO || hudstyle == HUD_QUAKEWORLD))
	{
		x = (int)(glcanvas.right - SBAR2_MARGIN_X - 16 + 0.5f);
		y = (int)(glcanvas.bottom - SBAR2_MARGIN_Y - 68 - 20 + 0.5f);
	}
	else
	{
		x = (int)(glcanvas.right - SBAR2_MARGIN_X - 20 + 0.5f);
		y = (int)(glcanvas.bottom - SBAR2_MARGIN_Y - 68 + 0.5f);
	}

	// hipnotic keys
	if (hipnotic)
	{
		for (i = 0; i < 2; i++)
		{
			if (cl.items & (IT_KEY1 << i))
			{
				Sbar_DrawPic (x, y + 6, sb_items[i]);
				y -= sb_items[i]->height;
			}
		}
	}

	for (i = 0; i < 6; i++)
	{
		if (i == 2)
		{
			if (scr_viewsize.value >= 110) // just the keys in the mini HUD
				break;
			x = (int)(glcanvas.left + SBAR2_MARGIN_X + 4 + 0.5f);
			y = (int)(glcanvas.bottom - SBAR2_MARGIN_Y - 66 + 0.5f);
			if (cl.items & IT_INVULNERABILITY || cl.stats[STAT_ARMOR] > 0)
				y -= 24; // armor row is visible, move starting position above it
		}

		if (cl.items & (1<<(17+i)))
		{
			time = cl.item_gettime[17+i];
			//MED 01/04/97 changed keys
			if (!hipnotic || (i > 1))
			{
				Sbar_DrawPic (x, y, sb_items[i]);
				y -= 16;
			}
			if (time && time > cl.time - 2)
				sb_updates = 0;
		}
	}

	if (scr_viewsize.value < 110)
	{
		if (hipnotic) // hipnotic items
		{
			for (i = 0; i < 2; i++)
			{
				if (cl.items & (1<<(24+i)))
				{
					time = cl.item_gettime[24+i];
					Sbar_DrawPic (x, y, hsb_items[i]);
					y -= 16;

					if (time && time > cl.time - 2)
						sb_updates = 0;
				}
			}
		}

		if (rogue) // new rogue items
		{
			for (i = 0; i < 2; i++)
			{
				if (cl.items & (1<<(29+i)))
				{
					time = cl.item_gettime[29+i];
					Sbar_DrawPic (x, y, rsb_items[i]);
					y -= 16;

					if (time && time > cl.time - 2)
						sb_updates = 0;
				}
			}
		}
	}
}

/*
===============
Sbar_DrawSigils
===============
*/
static void Sbar_DrawSigils (void)
{
	int		i, x, y;
	float	t;

	if (rogue || !(cl.items & (15<<28)) || cl.stats[STAT_HEALTH] <= 0)
		return;

	t = -FLT_MAX;
	for (i = 0; i < 4; i++)
		if (cl.items & (1<<(28+i)))
			t = q_max (t, cl.item_gettime[28+i]);
	t = q_max (t, cl.spawntime);

	if (!sb_showscores && (cl.time - t > 3.f || scr_viewsize.value >= 120))
		return;

	GL_SetCanvas (CANVAS_SBAR);

	x = 160 - 32/2;
	if (sb_showscores)
		y = -20;
	else if (hudstyle == HUD_CLASSIC || hudstyle == HUD_MODERN_CENTERAMMO)
		y = -8;
	else
		y = -4;
	Draw_SubPic (x, y + 24, 32, 16, sb_ibar, 1.f-32/320.f, 8/24.f, 32/320.f, 16/24.f, NULL, 1.f);

	for (i = 0; i < 4; i++)
	{
		if (cl.items & (1<<(28+i)))
		{
			t = (cl.time - q_max (cl.item_gettime[28+i], cl.spawntime));
			t = q_max (t, 0.f);
			if (t >= 1.f)
				t = 1.f;
			else
				t = 1.f - floor (fabs (fmod (t * 5.f, 2.f) - 1.f) * 3.f + 0.5f) / 3.f;
			Sbar_DrawPicAlpha (x + i*8, y, sb_sigil[i], t);
		}
	}
}


//=============================================================================

/*
===============
Sbar_DrawFrags -- johnfitz -- heavy revision
===============
*/
void Sbar_DrawFrags (void)
{
	int	numscores, i, x, color;
	char	num[12];
	scoreboard_t	*s;

	Sbar_SortFrags ();

// draw the text
	numscores = q_min(scoreboardlines, 4);

	for (i = 0, x = 184; i<numscores; i++, x += 32)
	{
		s = &cl.scores[fragsort[i]];
		if (!s->name[0])
			continue;

	// top color
		color = s->colors & 0xf0;
		color = Sbar_ColorForMap (color);
		Draw_Fill (x + 10, 1, 28, 4, color, 1);

	// bottom color
		color = (s->colors & 15)<<4;
		color = Sbar_ColorForMap (color);
		Draw_Fill (x + 10, 5, 28, 3, color, 1);

	// number
		sprintf (num, "%3i", s->frags);
		Sbar_DrawCharacter (x + 12, -24, num[0]);
		Sbar_DrawCharacter (x + 20, -24, num[1]);
		Sbar_DrawCharacter (x + 28, -24, num[2]);

	// brackets
		if (fragsort[i] == cl.viewentity - 1)
		{
			Sbar_DrawCharacter (x + 6, -24, 16);
			Sbar_DrawCharacter (x + 32, -24, 17);
		}
	}
}

/*
================
Sbar_DrawFragsQW
================
*/
void Sbar_DrawFragsQW(void)
{
	int	numscores, i, x, color;
	char	num[12];
	scoreboard_t	*s;

	Sbar_SortFrags();

	// draw the text
	numscores = q_min(scoreboardlines, 4);

	for (i = 0, x = -88 + ( (4 - numscores) * 32) ; i < numscores; i++, x += 32) // fill from the right
	{
		s = &cl.scores[fragsort[i]];
		if (!s->name[0])
			continue;

		// top color
		color = s->colors & 0xf0;
		color = Sbar_ColorForMap(color);
		Draw_Fill(x + 10, 0, 28, 4, color, 1);

		// bottom color
		color = (s->colors & 15) << 4;
		color = Sbar_ColorForMap(color);
		Draw_Fill(x + 10, 4, 28, 3, color, 1);

		// number
		sprintf(num, "%3i", s->frags);
		Sbar_DrawCharacter(x + 12, -25, num[0]);
		Sbar_DrawCharacter(x + 20, -25, num[1]);
		Sbar_DrawCharacter(x + 28, -25, num[2]);

		// brackets
		if (fragsort[i] == cl.viewentity - 1)
		{
			Sbar_DrawCharacter(x + 6, -25, 16);
			Sbar_DrawCharacter(x + 32, -25, 17);
		}
	}
}

/*
===============
Sbar_DrawFrags2
===============
*/
void Sbar_DrawFrags2 (void)
{
	int		i, x, y, color;
	char	num[12];

	Sbar_SortFrags ();

// draw the text
	x = (int) glcanvas.left;
	y = (int) LERP (glcanvas.top, glcanvas.bottom, 0.25f);
	y -= (scoreboardlines >> 2) << 3;
	y = q_max (40, y);

	for (i = 0; i < scoreboardlines; i++, y += 8)
	{
		scoreboard_t *s = &cl.scores[fragsort[i]];
		if (!s->name[0])
			continue;

	// top color
		color = s->colors & 0xf0;
		color = Sbar_ColorForMap (color);
		Draw_Fill (x + 6, y + 1, 28, 4, color, 1);

	// bottom color
		color = (s->colors & 15)<<4;
		color = Sbar_ColorForMap (color);
		Draw_Fill (x + 6, y + 5, 28, 3, color, 1);

	// number
		sprintf (num, "%3i", s->frags);
		Sbar_DrawCharacter (x + 8, y - 24, num[0]);
		Sbar_DrawCharacter (x + 16, y - 24, num[1]);
		Sbar_DrawCharacter (x + 24, y - 24, num[2]);

	// brackets
		if (fragsort[i] == cl.viewentity - 1)
		{
			Sbar_DrawCharacter (x + 2, y - 24, 16);
			Sbar_DrawCharacter (x + 28, y - 24, 17);
		}

	// name
		if (scr_viewsize.value < 110)
			Sbar_DrawString (x + 40, y - 24, s->name);
	}
}

//=============================================================================

/*
===============
Sbar_FacePic
===============
*/
static qpic_t *Sbar_FacePic (void)
{
	int f, anim;

	if ((cl.items & (IT_INVISIBILITY | IT_INVULNERABILITY))
			== (IT_INVISIBILITY | IT_INVULNERABILITY))
		return sb_face_invis_invuln;

	if (cl.items & IT_QUAD)
		return sb_face_quad;

	if (cl.items & IT_INVISIBILITY)
		return sb_face_invis;

	if (cl.items & IT_INVULNERABILITY)
		return sb_face_invuln;

	if (cl.stats[STAT_HEALTH] >= 100)
		f = 4;
	else
		f = cl.stats[STAT_HEALTH] / 20;
	if (f < 0)	// in case we ever decide to draw when health <= 0
		f = 0;

	if (cl.time <= cl.faceanimtime)
	{
		anim = 1;
		sb_updates = 0;		// make sure the anim gets drawn over
	}
	else
		anim = 0;

	return sb_faces[f][anim];
}

/*
===============
Sbar_AmmoPic
===============
*/
static qpic_t *Sbar_AmmoPic (void)
{
	if (rogue)
	{
		if (cl.items & RIT_SHELLS)
			return sb_ammo[0];
		if (cl.items & RIT_NAILS)
			return sb_ammo[1];
		if (cl.items & RIT_ROCKETS)
			return sb_ammo[2];
		if (cl.items & RIT_CELLS)
			return sb_ammo[3];
		if (cl.items & RIT_LAVA_NAILS)
			return rsb_ammo[0];
		if (cl.items & RIT_PLASMA_AMMO)
			return rsb_ammo[1];
		if (cl.items & RIT_MULTI_ROCKETS)
			return rsb_ammo[2];
	}
	else
	{
		if (cl.items & IT_SHELLS)
			return sb_ammo[0];
		if (cl.items & IT_NAILS)
			return sb_ammo[1];
		if (cl.items & IT_ROCKETS)
			return sb_ammo[2];
		if (cl.items & IT_CELLS)
			return sb_ammo[3];
	}
	return NULL;
}

/*
===============
Sbar_ArmorPic
===============
*/
static qpic_t *Sbar_ArmorPic (void)
{
	if (cl.items & IT_INVULNERABILITY)
		return draw_disc;
	if (rogue)
	{
		if (cl.items & RIT_ARMOR3)
			return sb_armor[2];
		if (cl.items & RIT_ARMOR2)
			return sb_armor[1];
		if (cl.items & RIT_ARMOR1)
			return sb_armor[0];
	}
	else
	{
		if (cl.items & IT_ARMOR3)
			return sb_armor[2];
		if (cl.items & IT_ARMOR2)
			return sb_armor[1];
		if (cl.items & IT_ARMOR1)
			return sb_armor[0];
	}
	return NULL;
}

/*
===============
Sbar_DrawFace
===============
*/
void Sbar_DrawFace (void)
{
	int f;

// PGM 01/19/97 - team color drawing
// PGM 03/02/97 - fixed so color swatch only appears in CTF modes
	if (rogue && (cl.maxclients != 1) && (teamplay.value>3) && (teamplay.value<7))
	{
		int	top, bottom;
		int	xofs;
		char	num[12];
		scoreboard_t	*s;

		s = &cl.scores[cl.viewentity - 1];
		// draw background
		top = s->colors & 0xf0;
		bottom = (s->colors & 15)<<4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

		if (cl.gametype == GAME_DEATHMATCH)
			xofs = 113;
		else
			xofs = ((vid.width - 320)>>1) + 113;

		Sbar_DrawPic (112, 0, rsb_teambord);
		Draw_Fill (xofs, /*vid.height-*/24+3, 22, 9, top, 1); //johnfitz -- sbar coords are now relative
		Draw_Fill (xofs, /*vid.height-*/24+12, 22, 9, bottom, 1); //johnfitz -- sbar coords are now relative

		// draw number
		f = s->frags;
		sprintf (num, "%3i",f);

		if (top == 8)
		{
			if (num[0] != ' ')
				Sbar_DrawCharacter(113, 3, 18 + num[0] - '0');
			if (num[1] != ' ')
				Sbar_DrawCharacter(120, 3, 18 + num[1] - '0');
			if (num[2] != ' ')
				Sbar_DrawCharacter(127, 3, 18 + num[2] - '0');
		}
		else
		{
			Sbar_DrawCharacter (113, 3, num[0]);
			Sbar_DrawCharacter (120, 3, num[1]);
			Sbar_DrawCharacter (127, 3, num[2]);
		}

		return;
	}
// PGM 01/19/97 - team color drawing

	Sbar_DrawPic (112, 0, Sbar_FacePic ());
}

/*
===============
Sbar_Draw
===============
*/
void Sbar_Draw (void)
{
	qboolean invuln;
	int armor;
	float x, y, w, h; //johnfitz
	qpic_t *pic;

	if (scr_con_current == vid.height)
		return;		// console is full screen

	if (cl.qcvm.extfuncs.CSQC_DrawHud && !qcvm)
	{
		qboolean deathmatchoverlay = false;
		sb_updates++;
		GL_SetCanvas (CANVAS_CSQC); //johnfitz
		PR_SwitchQCVM(&cl.qcvm);
		pr_global_struct->frametime = host_frametime;
		if (qcvm->extglobals.cltime)
			*qcvm->extglobals.cltime = realtime;
		if (qcvm->extglobals.clframetime)
			*qcvm->extglobals.clframetime = host_frametime;
		if (qcvm->extglobals.player_localentnum)
			*qcvm->extglobals.player_localentnum = cl.viewentity;
		pr_global_struct->time = cl.time;
		Sbar_SortFrags ();
		w = glcanvas.right - glcanvas.left;
		h = glcanvas.bottom - glcanvas.top;
		G_VECTORSET(OFS_PARM0, w, h, 0);
		G_FLOAT(OFS_PARM1) = sb_showscores;
		PR_ExecuteProgram(cl.qcvm.extfuncs.CSQC_DrawHud);
		if (cl.qcvm.extfuncs.CSQC_DrawScores)
		{
			G_VECTORSET(OFS_PARM0, w, h, 0);
			G_FLOAT(OFS_PARM1) = sb_showscores;
			if (key_dest != key_menu)
				PR_ExecuteProgram(cl.qcvm.extfuncs.CSQC_DrawScores);
		}
		else
			deathmatchoverlay = (sb_showscores || cl.stats[STAT_HEALTH] <= 0);
		PR_SwitchQCVM(NULL);

		if (deathmatchoverlay && cl.gametype == GAME_DEATHMATCH)
		{
			GL_SetCanvas (CANVAS_SBAR);
			Sbar_DeathmatchOverlay ();
		}
		return;
	}

	if (cl.intermission)
		return; //johnfitz -- never draw sbar during intermission

	if (sb_updates >= vid.numpages && !gl_clear.value && scr_sbaralpha.value >= 1 //johnfitz -- gl_clear, scr_sbaralpha
        && vid_gamma.value == 1)                         //ericw -- must draw sbar every frame if doing glsl gamma
		return;

	sb_updates++;

	GL_SetCanvas (CANVAS_DEFAULT); //johnfitz

	//johnfitz -- don't waste fillrate by clearing the area behind the sbar
	w = CLAMP (320.0f, scr_sbarscale.value * 320.0f, (float)glwidth);
	if (sb_lines && glwidth > w)
	{
		if (scr_sbaralpha.value < 1)
			Draw_TileClear (0, glheight - sb_lines, glwidth, sb_lines);
		if (cl.gametype == GAME_DEATHMATCH)
			Draw_TileClear (w, glheight - sb_lines, glwidth - w, sb_lines);
		else
		{
			Draw_TileClear (0, glheight - sb_lines, (glwidth - w) / 2.0f, sb_lines);
			Draw_TileClear ((glwidth - w) / 2.0f + w, glheight - sb_lines, (glwidth - w) / 2.0f, sb_lines);
		}
	}
	//johnfitz

	invuln = (cl.items & IT_INVULNERABILITY) != 0;
	armor = invuln ? 666 : cl.stats[STAT_ARMOR];

	if (hudstyle == HUD_CLASSIC || hudstyle == HUD_QUAKEWORLD)
	{
		GL_SetCanvas (CANVAS_SBAR); //johnfitz

		if (hudstyle == HUD_CLASSIC)	//classic hud
		{
			if (scr_viewsize.value < 110) //johnfitz -- check viewsize instead of sb_lines
			{
				Sbar_DrawInventory ();
				if (cl.maxclients != 1)
					Sbar_DrawFrags ();
			}
		}
		else	//qw hud
		{
			if (scr_viewsize.value < 120)
			{
				Sbar_DrawInventoryQW();
			}
			if (cl.maxclients != 1 && scr_viewsize.value < 110)	// qw hides frag count if MiniDeathmatchOverlay is showing
			{
				if (!Sbar_MiniScoreboardSizeCheck() || cl.gametype != GAME_DEATHMATCH)
				{
					GL_SetCanvas(CANVAS_SBAR_QW_INV);
					Sbar_DrawFragsQW();
					GL_SetCanvas(CANVAS_SBAR);
				}
			}
		}

		if (sb_showscores || cl.stats[STAT_HEALTH] <= 0)
		{
			Sbar_DrawPicAlpha (0, 0, sb_scorebar, scr_sbaralpha.value); //johnfitz -- scr_sbaralpha
			Sbar_DrawScoreboard ();
			sb_updates = 0;
		}
		else if (scr_viewsize.value < 120) //johnfitz -- check viewsize instead of sb_lines
		{
			if (hudstyle == HUD_CLASSIC)
				Sbar_DrawPicAlpha (0, 0, sb_sbar, scr_sbaralpha.value); //johnfitz -- scr_sbaralpha

	   // keys (hipnotic only)
			//MED 01/04/97 moved keys here so they would not be overwritten
			if (hipnotic)
			{
				if (cl.items & IT_KEY1)
					Sbar_DrawPic (209, 3, sb_items[0]);
				if (cl.items & IT_KEY2)
					Sbar_DrawPic (209, 12, sb_items[1]);
			}
		// armor
			if (cl.items & IT_INVULNERABILITY)
			{
				Sbar_DrawNum (24, 0, 666, 3, 1);
				Sbar_DrawPic (0, 0, draw_disc);
			}
			else
			{
				Sbar_DrawNum (24, 0, cl.stats[STAT_ARMOR], 3, cl.stats[STAT_ARMOR] <= 25);
				pic = Sbar_ArmorPic ();
				if (pic)
					Sbar_DrawPic (0, 0, pic);
			}

		// face
			Sbar_DrawFace ();

		// health
			Sbar_DrawNum (136, 0, cl.stats[STAT_HEALTH], 3
			, cl.stats[STAT_HEALTH] <= 25);

		// ammo icon
			pic = Sbar_AmmoPic ();
			if (pic)
				Sbar_DrawPic (224, 0, pic);

			Sbar_DrawNum (248, 0, cl.stats[STAT_AMMO], 3,
						  cl.stats[STAT_AMMO] <= 10);
		}

		//johnfitz -- removed the vid.width > 320 check here
		if (cl.gametype == GAME_DEATHMATCH)
				Sbar_MiniDeathmatchOverlay ();
	}
	else
	{
		if (sb_showscores || cl.stats[STAT_HEALTH] <= 0)
		{
			GL_SetCanvas (CANVAS_SBAR); //johnfitz
			Sbar_DrawPicAlpha (0, 0, sb_scorebar, scr_sbaralpha.value); //johnfitz -- scr_sbaralpha
			Sbar_DrawScoreboard ();
			sb_updates = 0;
		}
		else if (scr_viewsize.value < 120) //johnfitz -- check viewsize instead of sb_lines
		{
			GL_SetCanvas (CANVAS_SBAR2);

			x = (int)(glcanvas.left + SBAR2_MARGIN_X + 0.5f);
			y = (int)(glcanvas.bottom - SBAR2_MARGIN_Y - 48 + 0.5f);
			Sbar_DrawPic (x, y, Sbar_FacePic ());
			Sbar_DrawNum (x + 32, y, cl.stats[STAT_HEALTH], 3, cl.stats[STAT_HEALTH] <= 25);

			if (armor > 0)
			{
				Sbar_DrawNum (x + 32, y - 24, armor, 3, invuln || armor <= 25);
				Sbar_DrawPic (x, y - 24, Sbar_ArmorPic ());
			}

			x = (int)(glcanvas.right - SBAR2_MARGIN_X - 24 + 0.5f);
			pic = Sbar_AmmoPic ();
			if (pic)
			{
				Sbar_DrawPic (x, y, pic);
				x -= 32;
			}
			Sbar_DrawNum (x - 48, y, cl.stats[STAT_AMMO], 3, cl.stats[STAT_AMMO] <= 10);

			Sbar_DrawInventory2 ();
			if (cl.maxclients != 1)
				Sbar_DrawFrags2 ();
		}

		Sbar_DrawSigils ();
	}
}

//=============================================================================

/*
==================
Sbar_IntermissionNumber

==================
*/
void Sbar_IntermissionNumber (int x, int y, int num, int digits, int color)
{
	char	str[12];
	char	*ptr;
	int	l, frame;

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);
	if (l < digits)
		x += (digits-l)*24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Draw_Pic (x,y,sb_nums[color][frame]); //johnfitz -- stretched menus
		x += 24;
		ptr++;
	}
}

/*
==================
Sbar_IntermissionPicForChar
==================
*/
qpic_t *Sbar_IntermissionPicForChar (char c, int color)
{
	if ((unsigned)(c - '0') < 10)
		return sb_nums[color][c - '0'];
	if (c == '/')
		return sb_slash;
	if (c == ':')
		return sb_colon;
	if (c == '-')
		return sb_nums[color][STAT_MINUS];
	return NULL;
}

/*
==================
Sbar_IntermissionTextWidth
==================
*/
int Sbar_IntermissionTextWidth (const char *str, int color)
{
	int len = 0;
	while (*str)
	{
		qpic_t *pic = Sbar_IntermissionPicForChar (*str++, color);
		len += pic ? pic->width : 24;
	}
	return len;
}

/*
==================
Sbar_IntermissionText
==================
*/
void Sbar_IntermissionText (int x, int y, const char *str, int color)
{
	while (*str)
	{
		qpic_t *pic = Sbar_IntermissionPicForChar (*str++, color);
		if (!pic)
			continue;
		Draw_Pic (x, y, pic);
		x += pic ? pic->width : 24;
	}
}

/*
==================
Sbar_DeathmatchOverlay
==================
*/
void Sbar_DeathmatchOverlay (void)
{
	qpic_t	*pic;
	int	i, k, l;
	int	top, bottom;
	int	x, y, f;
	char	num[12];
	scoreboard_t	*s;

	GL_SetCanvas (CANVAS_MENU); //johnfitz

	pic = Draw_CachePic ("gfx/ranking.lmp");
	M_DrawPic ((320-pic->width)/2, 8, pic);

// scores
	Sbar_SortFrags ();

// draw the text
	l = scoreboardlines;

	x = 80; //johnfitz -- simplified becuase some positioning is handled elsewhere
	y = 40;
	for (i = 0; i < l; i++)
	{
		k = fragsort[i];
		s = &cl.scores[k];
		if (!s->name[0])
			continue;

	// draw background
		top = s->colors & 0xf0;
		bottom = (s->colors & 15)<<4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

		Draw_Fill ( x, y, 40, 4, top, 1); //johnfitz -- stretched overlays
		Draw_Fill ( x, y+4, 40, 4, bottom, 1); //johnfitz -- stretched overlays

	// draw number
		f = s->frags;
		sprintf (num, "%3i",f);

		Draw_Character ( x+8 , y, num[0]); //johnfitz -- stretched overlays
		Draw_Character ( x+16 , y, num[1]); //johnfitz -- stretched overlays
		Draw_Character ( x+24 , y, num[2]); //johnfitz -- stretched overlays

		if (k == cl.viewentity - 1)
			Draw_Character ( x - 8, y, 12); //johnfitz -- stretched overlays

#if 0
{
	int				total;
	int				n, minutes, tens, units;

	// draw time
		total = cl.completed_time - s->entertime;
		minutes = (int)total/60;
		n = total - minutes*60;
		tens = n/10;
		units = n%10;

		sprintf (num, "%3i:%i%i", minutes, tens, units);

		M_Print ( x+48 , y, num); //johnfitz -- was Draw_String, changed for stretched overlays
}
#endif

	// draw name
		M_Print (x+64, y, s->name); //johnfitz -- was Draw_String, changed for stretched overlays

		y += 10;
	}

	GL_SetCanvas (CANVAS_SBAR); //johnfitz
}

/*
==================
Sbar_MiniDeathmatchOverlay
==================
*/
void Sbar_MiniDeathmatchOverlay (void)
{
	int	i, k, top, bottom, x, y, f, numlines;
	char	num[12];
	scoreboard_t	*s;

	if (!Sbar_MiniScoreboardSizeCheck())
		return;

// scores
	Sbar_SortFrags ();

// draw the text
	numlines = (scr_viewsize.value >= 110) ? 3 : 6; //johnfitz

	//find us
	for (i = 0; i < scoreboardlines; i++)
		if (fragsort[i] == cl.viewentity - 1)
			break;
	if (i == scoreboardlines) // we're not there
		i = 0;
	else // figure out start
		i = i - numlines/2;
	if (i > scoreboardlines - numlines)
		i = scoreboardlines - numlines;
	if (i < 0)
		i = 0;

	x = 324;
	y = (scr_viewsize.value >= 110) ? 24 : 0; //johnfitz -- start at the right place
	for ( ; i < scoreboardlines && y <= 48; i++, y+=8) //johnfitz -- change y init, test, inc
	{
		k = fragsort[i];
		s = &cl.scores[k];
		if (!s->name[0])
			continue;

	// colors
		top = s->colors & 0xf0;
		bottom = (s->colors & 15)<<4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

		Draw_Fill (x, y+1, 40, 4, top, 1);
		Draw_Fill (x, y+5, 40, 3, bottom, 1);

	// number
		f = s->frags;
		sprintf (num, "%3i",f);
		Draw_Character (x+ 8, y, num[0]);
		Draw_Character (x+16, y, num[1]);
		Draw_Character (x+24, y, num[2]);

	// brackets
		if (k == cl.viewentity - 1)
		{
			Draw_Character (x, y, 16);
			Draw_Character (x+32, y, 17);
		}

	// name
		Draw_String (x+48, y, s->name);
	}
}

/*
==================
Sbar_IntermissionOverlay
==================
*/
void Sbar_IntermissionOverlay (void)
{
	qpic_t	*pic;
	char	time[32];
	char	secrets[32];
	char	monsters[32];
	int		ltime, lsecrets, lmonsters;
	int		total;

	if (cl.qcvm.extfuncs.CSQC_DrawScores && !qcvm)
	{
		float w, h;
		GL_SetCanvas (CANVAS_CSQC);
		PR_SwitchQCVM(&cl.qcvm);
		if (qcvm->extglobals.cltime)
			*qcvm->extglobals.cltime = realtime;
		if (qcvm->extglobals.clframetime)
			*qcvm->extglobals.clframetime = host_frametime;
		if (qcvm->extglobals.player_localentnum)
			*qcvm->extglobals.player_localentnum = cl.viewentity;
		if (qcvm->extglobals.intermission)
			*qcvm->extglobals.intermission = cl.intermission;
		if (qcvm->extglobals.intermission_time)
			*qcvm->extglobals.intermission_time = cl.completed_time;
		pr_global_struct->time = cl.time;
		pr_global_struct->frametime = host_frametime;
		Sbar_SortFrags ();
		w = glcanvas.right - glcanvas.left;
		h = glcanvas.bottom - glcanvas.top;
		G_VECTORSET(OFS_PARM0, w, h, 0);
		G_FLOAT(OFS_PARM1) = sb_showscores;
		PR_ExecuteProgram(cl.qcvm.extfuncs.CSQC_DrawScores);
		PR_SwitchQCVM(NULL);
		return;
	}

	if (cl.gametype == GAME_DEATHMATCH)
	{
		Sbar_DeathmatchOverlay ();
		return;
	}

	GL_SetCanvas (CANVAS_MENU); //johnfitz

	q_snprintf (time, sizeof (time), "%d:%02d", cl.completed_time / 60, cl.completed_time % 60);
	q_snprintf (secrets, sizeof (secrets), "%d/%2d", cl.stats[STAT_SECRETS], cl.stats[STAT_TOTALSECRETS]);
	q_snprintf (monsters, sizeof (monsters), "%d/%2d", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);

	ltime = Sbar_IntermissionTextWidth (time, 0);
	lsecrets = Sbar_IntermissionTextWidth (secrets, 0);
	lmonsters = Sbar_IntermissionTextWidth (monsters, 0);

	total = q_max (ltime, lsecrets);
	total = q_max (lmonsters, total);

	pic = Draw_CachePic ("gfx/inter.lmp");
	total += pic->width + 24;
	total = q_min (320, total);
	Draw_Pic (160 - total / 2, 56, pic);

	pic = Draw_CachePic ("gfx/complete.lmp");
	Draw_Pic (160 - pic->width / 2, 24, pic);

	Sbar_IntermissionText (160 + total / 2 - ltime, 64, time, 0);
	Sbar_IntermissionText (160 + total / 2 - lsecrets, 104, secrets, 0);
	Sbar_IntermissionText (160 + total / 2 - lmonsters, 144, monsters, 0);
}


/*
==================
Sbar_FinaleOverlay
==================
*/
void Sbar_FinaleOverlay (void)
{
	qpic_t	*pic;

	GL_SetCanvas (CANVAS_MENU); //johnfitz

	pic = Draw_CachePic ("gfx/finale.lmp");
	Draw_Pic ( (320 - pic->width)/2, 16, pic); //johnfitz -- stretched menus
}

