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
// r_main.c

#include "quakedef.h"

qboolean	r_cache_thrash;		// compatability

gpuframedata_t r_framedata;

vec3_t		*r_pointfile;

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

mplane_t	frustum[4];
float		r_matview[16];
float		r_matproj[16];
float		r_matviewproj[16];

//johnfitz -- rendering statistics
int rs_brushpolys, rs_aliaspolys, rs_skypolys;
int rs_dynamiclightmaps, rs_brushpasses, rs_aliaspasses, rs_skypasses;

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float r_fovx, r_fovy; //johnfitz -- rendering fov may be different becuase of r_waterwarp and r_stereo
qboolean water_warp;

extern byte *SV_FatPVS (vec3_t org, qmodel_t *worldmodel);
extern qboolean SV_EdictInPVS (edict_t *test, byte *pvs);
extern qboolean SV_BoxInPVS (vec3_t mins, vec3_t maxs, byte *pvs, mnode_t *node);

//
// screen size info
//
refdef_t	r_refdef;

mleaf_t		*r_viewleaf, *r_oldviewleaf;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value


cvar_t	r_norefresh = {"r_norefresh","0",CVAR_NONE};
cvar_t	r_drawentities = {"r_drawentities","1",CVAR_NONE};
cvar_t	r_shadows = {"r_shadows", "0", CVAR_ARCHIVE};
cvar_t	r_shadow_map_size = {"r_shadow_map_size", "2048", CVAR_ARCHIVE};
cvar_t	r_shadow_bias = {"r_shadow_bias", "0.0005", CVAR_ARCHIVE};
cvar_t	r_shadow_slope_bias = {"r_shadow_slope_bias", "2.0", CVAR_ARCHIVE};
cvar_t	r_drawviewmodel = {"r_drawviewmodel","1",CVAR_NONE};
cvar_t	r_speeds = {"r_speeds","0",CVAR_NONE};
cvar_t	r_pos = {"r_pos","0",CVAR_NONE};
cvar_t	r_fullbright = {"r_fullbright","0",CVAR_NONE};
cvar_t	r_lightmap = {"r_lightmap","0",CVAR_NONE};
cvar_t	r_wateralpha = {"r_wateralpha","1",CVAR_ARCHIVE};
cvar_t	r_litwater = {"r_litwater","1",CVAR_NONE};
cvar_t	r_dynamic = {"r_dynamic","1",CVAR_ARCHIVE};
cvar_t	r_novis = {"r_novis","0",CVAR_ARCHIVE};
#if defined(USE_SIMD)
cvar_t	r_simd = {"r_simd","1",CVAR_ARCHIVE};
#endif
cvar_t	r_alphasort = {"r_alphasort","1",CVAR_ARCHIVE};
cvar_t	r_oit = {"r_oit","1",CVAR_ARCHIVE};
cvar_t	r_dither = {"r_dither", "1.0", CVAR_ARCHIVE};

cvar_t	r_overbrightbits = {"r_overbrightbits", "1", CVAR_ARCHIVE};

cvar_t	gl_finish = {"gl_finish","0",CVAR_NONE};
cvar_t	gl_clear = {"gl_clear","1",CVAR_NONE};
cvar_t	gl_polyblend = {"gl_polyblend","1",CVAR_NONE};
cvar_t	gl_playermip = {"gl_playermip","0",CVAR_NONE};
cvar_t	gl_nocolors = {"gl_nocolors","0",CVAR_NONE};

//johnfitz -- new cvars
cvar_t	r_clearcolor = {"r_clearcolor","2",CVAR_ARCHIVE};
cvar_t	r_flatlightstyles = {"r_flatlightstyles", "0", CVAR_NONE};
cvar_t	r_lerplightstyles = {"r_lerplightstyles", "1", CVAR_ARCHIVE}; // 0=off; 1=skip abrupt transitions; 2=always lerp
cvar_t	gl_fullbrights = {"gl_fullbrights", "1", CVAR_ARCHIVE};
cvar_t	gl_farclip = {"gl_farclip", "65536", CVAR_ARCHIVE};
cvar_t	gl_overbright_models = {"gl_overbright_models", "1", CVAR_ARCHIVE};
cvar_t	r_oldskyleaf = {"r_oldskyleaf", "0", CVAR_NONE};
cvar_t	r_drawworld = {"r_drawworld", "1", CVAR_NONE};
cvar_t	r_showtris = {"r_showtris", "0", CVAR_NONE};
cvar_t	r_showbboxes = {"r_showbboxes", "0", CVAR_NONE};
cvar_t	r_showbboxes_think = {"r_showbboxes_think", "0", CVAR_NONE}; // 0=show all; 1=thinkers only; -1=non-thinkers only
cvar_t	r_showbboxes_health = {"r_showbboxes_health", "0", CVAR_NONE}; // 0=show all; 1=healthy only; -1=non-healthy only
cvar_t	r_showbboxes_links = {"r_showbboxes_links", "3", CVAR_NONE}; // 0=off; 1=outgoing only; 2=incoming only; 3=incoming+outgoing
cvar_t	r_showbboxes_targets = {"r_showbboxes_targets", "1", CVAR_NONE};
cvar_t	r_showfields = {"r_showfields", "0", CVAR_NONE};
cvar_t	r_showfields_align = {"r_showfields_align", "1", CVAR_ARCHIVE}; // 0=entity pos; 1=bottom-right
cvar_t	r_lerpmodels = {"r_lerpmodels", "1", CVAR_ARCHIVE};
cvar_t	r_lerpmove = {"r_lerpmove", "1", CVAR_ARCHIVE};
cvar_t	r_nolerp_list = {"r_nolerp_list", "progs/flame.mdl,progs/flame2.mdl,progs/braztall.mdl,progs/brazshrt.mdl,progs/longtrch.mdl,progs/flame_pyre.mdl,progs/v_saw.mdl,progs/v_xfist.mdl,progs/h2stuff/newfire.mdl", CVAR_NONE};
cvar_t	r_noshadow_list = {"r_noshadow_list", "progs/flame2.mdl,progs/flame.mdl,progs/bolt1.mdl,progs/bolt2.mdl,progs/bolt3.mdl,progs/laser.mdl", CVAR_NONE};

extern cvar_t	r_vfog;
extern cvar_t	vid_fsaa;
//johnfitz
extern cvar_t	r_softemu_dither_screen;
extern cvar_t	r_softemu_dither_texture;

cvar_t	gl_zfix = {"gl_zfix", "1", CVAR_ARCHIVE}; // QuakeSpasm z-fighting fix

cvar_t	r_lavaalpha = {"r_lavaalpha","0",CVAR_NONE};
cvar_t	r_telealpha = {"r_telealpha","0",CVAR_NONE};
cvar_t	r_slimealpha = {"r_slimealpha","0",CVAR_NONE};

float	map_wateralpha, map_lavaalpha, map_telealpha, map_slimealpha;
float	map_fallbackalpha;

qboolean r_fullbright_cheatsafe, r_lightmap_cheatsafe, r_drawworld_cheatsafe; //johnfitz

cvar_t	r_scale = {"r_scale", "1", CVAR_ARCHIVE};

static const vec3_t shadow_default_direction = {-0.57735027f, -0.57735027f, -0.57735027f};

typedef struct shadow_state_s {
	GLuint	fbo;
	GLuint	depth_texture;
	int		size;
	qboolean	ready;
	qboolean	enabled;
	vec3_t	direction;
	vec3_t	color;
	float	intensity;
	float	viewproj[16];
} shadow_state_t;

static shadow_state_t shadow_state;
static vec3_t shadow_pending_angles;
static qboolean shadow_has_angles;
static vec3_t shadow_pending_color;
static qboolean shadow_has_color;
static float shadow_pending_intensity;
static qboolean shadow_has_intensity;
static float shadow_view_znear;
static float shadow_view_zfar;

//==============================================================================
//
// FRAMEBUFFERS
//
//==============================================================================

glframebufs_t framebufs;

/*
=============
GL_CreateFBOAttachment
=============
*/
static GLuint GL_CreateFBOAttachment (GLenum format, int samples, GLenum filter, const char *name)
{
	GLenum target = samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
	GLuint texnum;

	glGenTextures (1, &texnum);
	GL_BindNative (GL_TEXTURE0, target, texnum);
	GL_ObjectLabelFunc (GL_TEXTURE, texnum, -1, name);
	if (samples > 1)
	{
		GL_TexStorage2DMultisampleFunc (target, samples, format, vid.width, vid.height, GL_FALSE);
	}
	else
	{
		GL_TexStorage2DFunc (target, 1, format, vid.width, vid.height);
		glTexParameteri (target, GL_TEXTURE_MAG_FILTER, filter);
		glTexParameteri (target, GL_TEXTURE_MIN_FILTER, filter);
	}
	glTexParameteri (target, GL_TEXTURE_MAX_LEVEL, 0);

	return texnum;
}

/*
=============
GL_CreateFBO
=============
*/
static GLuint GL_CreateFBO (GLenum target, const GLuint *colors, int numcolors, GLuint depth, GLuint stencil, const char *name)
{
	GLenum status;
	GLuint fbo;
	GLenum buffers[8];
	int i;

	if (numcolors > (int)countof (buffers))
		Sys_Error ("GL_CreateFBO: too many color buffers (%d)", numcolors);

	GL_GenFramebuffersFunc (1, &fbo);
	GL_BindFramebufferFunc (GL_FRAMEBUFFER, fbo);
	GL_ObjectLabelFunc (GL_FRAMEBUFFER, fbo, -1, name);

	for (i = 0; i < numcolors; i++)
	{
		GL_FramebufferTexture2DFunc (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, target, colors[i], 0);
		buffers[i] = GL_COLOR_ATTACHMENT0 + i;
	}
	GL_DrawBuffersFunc (numcolors, buffers);

	if (depth)
		GL_FramebufferTexture2DFunc (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, depth, 0);
	if (stencil)
		GL_FramebufferTexture2DFunc (GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, target, stencil, 0);

	status = GL_CheckFramebufferStatusFunc (GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		Sys_Error ("Failed to create %s (status code 0x%X)", name, status);

	return fbo;
}

/*
=============
GL_CreateSimpleFBO
=============
*/
static GLuint GL_CreateSimpleFBO (GLenum target, GLuint colors, GLuint depth, GLuint stencil, const char *name)
{
	return GL_CreateFBO (target, colors ? &colors : NULL, colors ? 1 : 0, depth, stencil, name);
}

/*
=============
GL_CreateFrameBuffers
=============
*/
void GL_CreateFrameBuffers (void)
{
	GLenum color_format = GL_RGB10_A2;
	GLenum depth_format = GL_DEPTH24_STENCIL8;

	/* query MSAA limits */
	glGetIntegerv (GL_MAX_COLOR_TEXTURE_SAMPLES, &framebufs.max_color_tex_samples);
	glGetIntegerv (GL_MAX_DEPTH_TEXTURE_SAMPLES, &framebufs.max_depth_tex_samples);
	framebufs.max_samples = q_min (framebufs.max_color_tex_samples, framebufs.max_depth_tex_samples);

	/* main framebuffer (color + depth + stencil) */
	framebufs.composite.color_tex = GL_CreateFBOAttachment (color_format, 1, GL_NEAREST, "composite colors");
	framebufs.composite.depth_stencil_tex = GL_CreateFBOAttachment (depth_format, 1, GL_NEAREST, "composite depth/stencil");
	framebufs.composite.fbo = GL_CreateSimpleFBO (GL_TEXTURE_2D,
		framebufs.composite.color_tex,
		framebufs.composite.depth_stencil_tex,
		framebufs.composite.depth_stencil_tex,
		"composite fbo"
	);

	/* scene framebuffer (color + depth + stencil, potentially multisampled) */
	framebufs.scene.samples = Q_nextPow2 ((int) q_max (1.f, vid_fsaa.value));
	framebufs.scene.samples = CLAMP (1, framebufs.scene.samples, framebufs.max_samples);

	framebufs.scene.color_tex = GL_CreateFBOAttachment (color_format, framebufs.scene.samples, GL_NEAREST, "scene colors");
	framebufs.scene.depth_stencil_tex = GL_CreateFBOAttachment (depth_format, framebufs.scene.samples, GL_NEAREST, "scene depth/stencil");
	framebufs.scene.fbo = GL_CreateSimpleFBO (framebufs.scene.samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,
		framebufs.scene.color_tex,
		framebufs.scene.depth_stencil_tex,
		framebufs.scene.depth_stencil_tex,
		"scene fbo"
	);

	/* weighted blended order-independent transparency (accum + revealage, potentially multisampled */
	framebufs.oit.accum_tex = GL_CreateFBOAttachment (GL_RGBA16F, framebufs.scene.samples, GL_NEAREST, "oit accum");
	framebufs.oit.revealage_tex = GL_CreateFBOAttachment (GL_R8, framebufs.scene.samples, GL_NEAREST, "oit revealage");
	framebufs.oit.fbo_scene = GL_CreateFBO (framebufs.scene.samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,
		framebufs.oit.mrt, 2,
		framebufs.scene.depth_stencil_tex,
		framebufs.scene.depth_stencil_tex,
		"oit scene fbo"
	);

	/* resolved scene framebuffer (color only) */
	if (framebufs.scene.samples > 1)
	{
		framebufs.resolved_scene.color_tex = GL_CreateFBOAttachment (color_format, 1, GL_NEAREST, "resolved scene colors");
		framebufs.resolved_scene.fbo = GL_CreateSimpleFBO (GL_TEXTURE_2D, framebufs.resolved_scene.color_tex, 0, 0, "resolved scene fbo");
	}
	else
	{
		framebufs.resolved_scene.color_tex = 0;
		framebufs.resolved_scene.fbo = 0;

		framebufs.oit.fbo_composite = GL_CreateFBO (GL_TEXTURE_2D,
			framebufs.oit.mrt, 2,
			framebufs.composite.depth_stencil_tex,
			framebufs.composite.depth_stencil_tex,
			"oit composite fbo"
		);
	}

	GL_BindFramebufferFunc (GL_FRAMEBUFFER, 0);
	GL_BindNative (GL_TEXTURE0, GL_TEXTURE_2D, 0);
}

/*
=============
GL_DeleteFrameBuffers
=============
*/
void GL_DeleteFrameBuffers (void)
{
        GL_DeleteFramebuffersFunc (1, &framebufs.resolved_scene.fbo);
        GL_DeleteFramebuffersFunc (1, &framebufs.oit.fbo_composite);
        GL_DeleteFramebuffersFunc (1, &framebufs.oit.fbo_scene);
        GL_DeleteFramebuffersFunc (1, &framebufs.scene.fbo);
        GL_DeleteFramebuffersFunc (1, &framebufs.composite.fbo);
        GL_BindFramebufferFunc (GL_FRAMEBUFFER, 0);

        GL_DeleteNativeTexture (framebufs.resolved_scene.color_tex);
        GL_DeleteNativeTexture (framebufs.oit.revealage_tex);
        GL_DeleteNativeTexture (framebufs.oit.accum_tex);
        GL_DeleteNativeTexture (framebufs.scene.depth_stencil_tex);
        GL_DeleteNativeTexture (framebufs.scene.color_tex);
        GL_DeleteNativeTexture (framebufs.composite.depth_stencil_tex);
        GL_DeleteNativeTexture (framebufs.composite.color_tex);

        memset (&framebufs, 0, sizeof (framebufs));
}

static void GL_OrthoMatrix (float matrix[16], float left, float right, float bottom, float top, float n, float f)
{
        float rl = right - left;
        float tb = top - bottom;
        float fn = f - n;

        memset (matrix, 0, 16 * sizeof (float));

        if (rl == 0.f || tb == 0.f || fn == 0.f)
        {
                IdentityMatrix (matrix);
                return;
        }

        matrix[0*4 + 0] = 2.f / rl;
        matrix[1*4 + 1] = 2.f / tb;
        if (gl_clipcontrol_able)
        {
                matrix[2*4 + 2] = 1.f / (n - f);
                matrix[3*4 + 2] = n / (n - f);
        }
        else
        {
                matrix[2*4 + 2] = -2.f / fn;
                matrix[3*4 + 2] = -(f + n) / fn;
        }
        matrix[3*4 + 0] = -(right + left) / rl;
        matrix[3*4 + 1] = -(top + bottom) / tb;
        matrix[3*4 + 3] = 1.f;
}

static int R_ShadowClampSize (int size)
{
        if (size <= 1024)
                return 1024;
        if (size <= 2048)
                return 2048;
        return 4096;
}

static void R_ShadowDestroyResources (void)
{
        if (shadow_state.fbo)
        {
                GL_DeleteFramebuffersFunc (1, &shadow_state.fbo);
                shadow_state.fbo = 0;
        }
        if (shadow_state.depth_texture)
        {
                GL_DeleteNativeTexture (shadow_state.depth_texture);
                shadow_state.depth_texture = 0;
        }
        shadow_state.ready = false;
        shadow_state.enabled = false;
        shadow_state.size = 0;
}

void R_InitShadow (void)
{
        memset (&shadow_state, 0, sizeof (shadow_state));
        VectorCopy (shadow_default_direction, shadow_state.direction);
        shadow_state.color[0] = shadow_state.color[1] = shadow_state.color[2] = 1.f;
        shadow_state.intensity = 1.f;
        R_ShadowNewMap ();
        R_ResizeShadowMapIfNeeded ();
}

void R_ShutdownShadow (void)
{
        R_ShadowDestroyResources ();
}

void R_ResizeShadowMapIfNeeded (void)
{
        int desired;

        desired = R_ShadowClampSize ((int) Q_rint (r_shadow_map_size.value));
        if (desired != (int) r_shadow_map_size.value)
                Cvar_SetValueQuick (&r_shadow_map_size, (float) desired);

        if (shadow_state.ready && shadow_state.size == desired)
                return;

        R_ShadowDestroyResources ();
        shadow_state.size = desired;

        if (!desired)
                return;

        glGenTextures (1, &shadow_state.depth_texture);
        if (!shadow_state.depth_texture)
                return;

        GL_BindNative (GL_TEXTURE0, GL_TEXTURE_2D, shadow_state.depth_texture);
        GL_ObjectLabelFunc (GL_TEXTURE, shadow_state.depth_texture, -1, "shadow depth");
        glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, desired, desired, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, gl_clipcontrol_able ? GL_GEQUAL : GL_LEQUAL);
        {
                const float border[4] = {1.f, 1.f, 1.f, 1.f};
                glTexParameterfv (GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
        }
        GL_BindNative (GL_TEXTURE0, GL_TEXTURE_2D, 0);

        GL_GenFramebuffersFunc (1, &shadow_state.fbo);
        GL_BindFramebufferFunc (GL_FRAMEBUFFER, shadow_state.fbo);
        GL_ObjectLabelFunc (GL_FRAMEBUFFER, shadow_state.fbo, -1, "shadow fbo");
        GL_FramebufferTexture2DFunc (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_state.depth_texture, 0);
        glDrawBuffer (GL_NONE);
        glReadBuffer (GL_NONE);

        if (GL_CheckFramebufferStatusFunc (GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
                Con_Printf ("Failed to create shadow framebuffer
");
                GL_BindFramebufferFunc (GL_FRAMEBUFFER, 0);
                glDrawBuffer (GL_BACK);
                glReadBuffer (GL_BACK);
                R_ShadowDestroyResources ();
                shadow_state.size = 0;
                return;
        }

        GL_BindFramebufferFunc (GL_FRAMEBUFFER, 0);
        glDrawBuffer (GL_BACK);
        glReadBuffer (GL_BACK);

        shadow_state.ready = true;
}

void R_ShadowNewMap (void)
{
        shadow_has_angles = false;
        shadow_has_color = false;
        shadow_has_intensity = false;
        VectorCopy (shadow_default_direction, shadow_state.direction);
        shadow_state.color[0] = shadow_state.color[1] = shadow_state.color[2] = 1.f;
        shadow_state.intensity = 1.f;
        VectorClear (shadow_pending_angles);
        VectorClear (shadow_pending_color);
        shadow_pending_intensity = 1.f;
        shadow_state.enabled = false;
}

void R_ShadowParseWorldspawnKey (const char *key, const char *value)
{
        if (!strcmp (key, "sun_mangle") || !strcmp (key, "_sun_mangle"))
        {
                float pitch, yaw, roll;
                if (sscanf (value, "%f %f %f", &pitch, &yaw, &roll) == 3)
                {
                        shadow_pending_angles[0] = pitch;
                        shadow_pending_angles[1] = yaw;
                        shadow_pending_angles[2] = roll;
                        shadow_has_angles = true;
                }
        }
        else if (!strcmp (key, "sunlight") || !strcmp (key, "_sunlight"))
        {
                shadow_pending_intensity = (float) atof (value);
                shadow_has_intensity = true;
        }
        else if (!strcmp (key, "sunlight_color") || !strcmp (key, "_sunlight_color"))
        {
                float r, g, b;
                if (sscanf (value, "%f %f %f", &r, &g, &b) == 3)
                {
                        shadow_pending_color[0] = r;
                        shadow_pending_color[1] = g;
                        shadow_pending_color[2] = b;
                        shadow_has_color = true;
                }
        }
}

void R_ShadowFinalizeWorldspawn (void)
{
        vec3_t dir;

        if (shadow_has_angles)
        {
                AngleVectors (shadow_pending_angles, dir, NULL, NULL);
                if (VectorNormalize (dir) == 0.f)
                        VectorCopy (shadow_default_direction, dir);
        }
        else
        {
                VectorCopy (shadow_default_direction, dir);
        }
        VectorCopy (dir, shadow_state.direction);

        if (shadow_has_color)
        {
                vec3_t color;
                VectorCopy (shadow_pending_color, color);
                if (color[0] > 1.f || color[1] > 1.f || color[2] > 1.f)
                {
                        color[0] *= (1.f / 255.f);
                        color[1] *= (1.f / 255.f);
                        color[2] *= (1.f / 255.f);
                }
                shadow_state.color[0] = CLAMP (0.f, color[0], 1.f);
                shadow_state.color[1] = CLAMP (0.f, color[1], 1.f);
                shadow_state.color[2] = CLAMP (0.f, color[2], 1.f);
        }
        else
        {
                shadow_state.color[0] = shadow_state.color[1] = shadow_state.color[2] = 1.f;
        }

        if (shadow_has_intensity)
                shadow_state.intensity = shadow_pending_intensity;
        else
                shadow_state.intensity = 1.f;
}

void R_ShadowCvarChanged (cvar_t *var)
{
        if (var == &r_shadow_map_size)
        {
                int desired = R_ShadowClampSize ((int) Q_rint (var->value));
                if (desired != (int) var->value)
                        Cvar_SetValueQuick (var, (float) desired);
                if (host_initialized)
                        R_ResizeShadowMapIfNeeded ();
        }
        else if (var == &r_shadow_bias)
        {
                if (var->value < 0.f)
                        Cvar_SetValueQuick (var, 0.f);
        }
        else if (var == &r_shadow_slope_bias)
        {
                if (var->value < 0.f)
                        Cvar_SetValueQuick (var, 0.f);
        }
}

GLuint R_ShadowTexture (void)
{
        return shadow_state.depth_texture;
}

static qboolean R_ShadowComputeMatrices (float out_vp[16])
{
        vec3_t forward, right, up;
        vec3_t up_candidate;
        vec3_t center_world;
        vec3_t corners[8];
        vec3_t center_light;
        vec3_t center_world_snapped;
        float radius;
        float min_z, max_z;
        float near_plane, far_plane;
        float texel_size;
        float rot[16];
        float trans[16];
        float view[16];
        float proj[16];
        int i;

        if (!shadow_state.size)
                return false;

        VectorCopy (shadow_state.direction, forward);
        if (VectorNormalize (forward) == 0.f)
                return false;

        VectorSet (up_candidate, 0.f, 0.f, 1.f);
        if (fabs (DotProduct (forward, up_candidate)) > 0.95f)
                VectorSet (up_candidate, 0.f, 1.f, 0.f);
        CrossProduct (forward, up_candidate, right);
        if (VectorNormalize (right) == 0.f)
                return false;
        CrossProduct (right, forward, up);

        if (shadow_view_znear <= 0.f || shadow_view_zfar <= shadow_view_znear)
                return false;

        {
                float near_dist = shadow_view_znear;
                float far_dist = shadow_view_zfar;
                float tan_fov_x = tanf (DEG2RAD (r_fovx) * 0.5f);
                float tan_fov_y = tanf (DEG2RAD (r_fovy) * 0.5f);
                vec3_t center_near, center_far;
                vec3_t up_near, right_near, up_far, right_far;

                VectorMA (r_refdef.vieworg, near_dist, vpn, center_near);
                VectorMA (r_refdef.vieworg, far_dist, vpn, center_far);
                VectorScale (vup, near_dist * tan_fov_y, up_near);
                VectorScale (vright, near_dist * tan_fov_x, right_near);
                VectorScale (vup, far_dist * tan_fov_y, up_far);
                VectorScale (vright, far_dist * tan_fov_x, right_far);

                VectorAdd (center_near, up_near, corners[0]);
                VectorSubtract (corners[0], right_near, corners[0]);
                VectorAdd (center_near, up_near, corners[1]);
                VectorAdd (corners[1], right_near, corners[1]);
                VectorSubtract (center_near, up_near, corners[2]);
                VectorAdd (corners[2], right_near, corners[2]);
                VectorSubtract (center_near, up_near, corners[3]);
                VectorSubtract (corners[3], right_near, corners[3]);

                VectorAdd (center_far, up_far, corners[4]);
                VectorSubtract (corners[4], right_far, corners[4]);
                VectorAdd (center_far, up_far, corners[5]);
                VectorAdd (corners[5], right_far, corners[5]);
                VectorSubtract (center_far, up_far, corners[6]);
                VectorAdd (corners[6], right_far, corners[6]);
                VectorSubtract (center_far, up_far, corners[7]);
                VectorSubtract (corners[7], right_far, corners[7]);
        }

        VectorClear (center_world);
        for (i = 0; i < 8; i++)
                VectorAdd (center_world, corners[i], center_world);
        VectorScale (center_world, 1.f / 8.f, center_world);

        center_light[0] = DotProduct (center_world, right);
        center_light[1] = DotProduct (center_world, up);
        center_light[2] = DotProduct (center_world, forward);

        radius = 0.f;
        min_z = 1e30f;
        max_z = -1e30f;
        for (i = 0; i < 8; i++)
        {
                float rx = DotProduct (corners[i], right);
                float ry = DotProduct (corners[i], up);
                float rz = DotProduct (corners[i], forward);
                float dx = rx - center_light[0];
                float dy = ry - center_light[1];
                float dist = sqrtf (dx * dx + dy * dy);
                if (dist > radius)
                        radius = dist;
                if (rz < min_z)
                        min_z = rz;
                if (rz > max_z)
                        max_z = rz;
        }

        if (radius <= 0.f)
                radius = 1.f;

        texel_size = (radius * 2.f) / (float) shadow_state.size;
        if (texel_size > 0.f)
        {
                center_light[0] = floorf (center_light[0] / texel_size + 0.5f) * texel_size;
                center_light[1] = floorf (center_light[1] / texel_size + 0.5f) * texel_size;
        }

        for (i = 0; i < 3; i++)
        {
                center_world_snapped[i] = right[i] * center_light[0] + up[i] * center_light[1] + forward[i] * center_light[2];
        }

        min_z -= center_light[2];
        max_z -= center_light[2];
        near_plane = min_z - 128.f;
        far_plane = max_z + 128.f;
        if (far_plane <= near_plane + 1.f)
                far_plane = near_plane + 1.f;

        IdentityMatrix (rot);
        rot[0] = right[0];
        rot[1] = right[1];
        rot[2] = right[2];
        rot[4] = up[0];
        rot[5] = up[1];
        rot[6] = up[2];
        rot[8] = -forward[0];
        rot[9] = -forward[1];
        rot[10] = -forward[2];
        rot[15] = 1.f;

        TranslationMatrix (trans, -center_world_snapped[0], -center_world_snapped[1], -center_world_snapped[2]);
        memcpy (view, rot, sizeof (rot));
        MatrixMultiply (view, trans);

        GL_OrthoMatrix (proj, -radius, radius, -radius, radius, near_plane, far_plane);
        memcpy (out_vp, proj, sizeof (proj));
        MatrixMultiply (out_vp, view);

        return true;
}

void R_BuildShadowMap (void)
{
        entity_t **entlist;
        int *ofs;
        float light_vp[16];
        vec3_t sun_dir;

        shadow_state.enabled = false;

        r_framedata.shadow_params[0] = q_max (0.f, r_shadow_bias.value);
        r_framedata.shadow_params[1] = q_max (0.f, r_shadow_slope_bias.value);
        r_framedata.shadow_params[2] = shadow_state.size ? 1.f / (float) shadow_state.size : 0.f;
        r_framedata.shadow_params[3] = 0.f;
        memset (r_framedata.shadowviewproj, 0, sizeof (r_framedata.shadowviewproj));
        memset (r_framedata.shadow_sundir, 0, sizeof (r_framedata.shadow_sundir));
        memset (r_framedata.shadow_suncolor, 0, sizeof (r_framedata.shadow_suncolor));

        if (!r_shadows.value || !shadow_state.ready || !cl.worldmodel)
                return;
        if (shadow_state.intensity <= 0.f)
                return;
        VectorCopy (shadow_state.direction, sun_dir);
        if (VectorNormalize (sun_dir) == 0.f)
                return;

        if (!glprogs.shadow_depth)
                return;
        if (!R_ShadowComputeMatrices (light_vp))
                return;

        memcpy (shadow_state.viewproj, light_vp, sizeof (light_vp));
        memcpy (r_framedata.shadowviewproj, light_vp, sizeof (light_vp));

        r_framedata.shadow_params[3] = 1.f;
        r_framedata.shadow_sundir[0] = -sun_dir[0];
        r_framedata.shadow_sundir[1] = -sun_dir[1];
        r_framedata.shadow_sundir[2] = -sun_dir[2];
        r_framedata.shadow_sundir[3] = shadow_state.intensity;
        r_framedata.shadow_suncolor[0] = shadow_state.color[0];
        r_framedata.shadow_suncolor[1] = shadow_state.color[1];
        r_framedata.shadow_suncolor[2] = shadow_state.color[2];
        r_framedata.shadow_suncolor[3] = 0.f;

        shadow_state.enabled = true;

        GL_BeginGroup ("Shadow map");
        GL_BindFramebufferFunc (GL_FRAMEBUFFER, shadow_state.fbo);
        glViewport (0, 0, shadow_state.size, shadow_state.size);
        glColorMask (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask (GL_TRUE);
        glClear (GL_DEPTH_BUFFER_BIT);
        glEnable (GL_POLYGON_OFFSET_FILL);
        glEnable (GL_POLYGON_OFFSET_LINE);
        glPolygonOffset (r_shadow_slope_bias.value, r_shadow_bias.value);

        GL_UseProgram (glprogs.shadow_depth);
        GL_SetState (GLS_BLEND_OPAQUE | GLS_CULL_FRONT | GLS_ATTRIBS(4));

        entlist = cl_sorted_visedicts;
        ofs = cl_modtype_ofs;
        if (ofs[2 * mod_brush + 1] - ofs[2 * mod_brush] > 0)
                R_DrawBrushModels_Shadow (entlist + ofs[2 * mod_brush], ofs[2 * mod_brush + 1] - ofs[2 * mod_brush], false);
        ofs = cl_modtype_ofs + 1;
        if (ofs[2 * mod_brush + 1] - ofs[2 * mod_brush] > 0)
                R_DrawBrushModels_Shadow (entlist + ofs[2 * mod_brush], ofs[2 * mod_brush + 1] - ofs[2 * mod_brush], true);

        GL_SetState (GLS_DEFAULT_STATE);
        glDisable (GL_POLYGON_OFFSET_FILL);
        glDisable (GL_POLYGON_OFFSET_LINE);
        glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        GL_BindFramebufferFunc (GL_FRAMEBUFFER, 0);
        glDrawBuffer (GL_BACK);
        glReadBuffer (GL_BACK);
        GL_EndGroup ();
}

//==============================================================================
//
// POSTPROCESSING
//
//==============================================================================

static const float NOISESCALE = 9.f / 255.f;

extern GLuint gl_palette_lut;
extern GLuint gl_palette_buffer[2];

/*
=============
GL_PostProcess
=============
*/
void GL_PostProcess (void)
{
	int palidx, variant;
	float dither;
	if (!GL_NeedsPostprocess ())
		return;

	GL_BeginGroup ("Postprocess");

	palidx =  GLPalette_Postprocess ();
	dither = (softemu == SOFTEMU_FINE) ? NOISESCALE * r_dither.value * r_softemu_dither_screen.value : 0.f;

	GL_BindFramebufferFunc (GL_FRAMEBUFFER, 0);
	glViewport (glx, gly, glwidth, glheight);

	variant = q_min ((int)softemu, 2);
	GL_UseProgram (glprogs.postprocess[variant]);
	GL_SetState (GLS_BLEND_OPAQUE | GLS_NO_ZTEST | GLS_NO_ZWRITE | GLS_CULL_NONE | GLS_ATTRIBS(0));
	GL_BindNative (GL_TEXTURE0, GL_TEXTURE_2D, framebufs.composite.color_tex);
	GL_BindNative (GL_TEXTURE1, GL_TEXTURE_3D, gl_palette_lut);
	GL_BindBufferRange (GL_SHADER_STORAGE_BUFFER, 0, gl_palette_buffer[palidx], 0, 256 * sizeof (GLuint));
	if (variant != 2) // some AMD drivers optimize out the uniform in variant #2
		GL_Uniform4fFunc (0, vid_gamma.value, q_min(2.0f, q_max(1.0f, vid_contrast.value)), 1.f/r_refdef.scale, dither);

	glDrawArrays (GL_TRIANGLES, 0, 3);

	GL_EndGroup ();
}

/*
=================
R_CullBox -- johnfitz -- replaced with new function from lordhavoc

Returns true if the box is completely outside the frustum
=================
*/
qboolean R_CullBox (vec3_t emins, vec3_t emaxs)
{
	int i;
	mplane_t *p;
	byte signbits;
	float vec[3];
	for (i = 0;i < 4;i++)
	{
		p = frustum + i;
		signbits = p->signbits;
		vec[0] = ((signbits & 1) ? emins : emaxs)[0];
		vec[1] = ((signbits & 2) ? emins : emaxs)[1];
		vec[2] = ((signbits & 4) ? emins : emaxs)[2];
		if (p->normal[0]*vec[0] + p->normal[1]*vec[1] + p->normal[2]*vec[2] < p->dist)
			return true;
	}
	return false;
}

/*
===============
R_GetEntityBounds -- johnfitz -- uses correct bounds based on rotation
===============
*/
void R_GetEntityBounds (const entity_t *e, vec3_t mins, vec3_t maxs)
{
	vec_t scalefactor, *minbounds, *maxbounds;

	if (e->angles[0] || e->angles[2]) //pitch or roll
	{
		minbounds = e->model->rmins;
		maxbounds = e->model->rmaxs;
	}
	else if (e->angles[1]) //yaw
	{
		minbounds = e->model->ymins;
		maxbounds = e->model->ymaxs;
	}
	else //no rotation
	{
		minbounds = e->model->mins;
		maxbounds = e->model->maxs;
	}

	scalefactor = ENTSCALE_DECODE(e->scale);
	if (scalefactor != 1.0f)
	{
		VectorMA (e->origin, scalefactor, minbounds, mins);
		VectorMA (e->origin, scalefactor, maxbounds, maxs);
	}
	else
	{
		VectorAdd (e->origin, minbounds, mins);
		VectorAdd (e->origin, maxbounds, maxs);
	}
}

/*
===============
R_CullModelForEntity -- johnfitz -- uses correct bounds based on rotation
===============
*/
qboolean R_CullModelForEntity (entity_t *e)
{
	vec3_t mins, maxs;

	R_GetEntityBounds (e, mins, maxs);

	return R_CullBox (mins, maxs);
}

/*
===============
R_EntityMatrix
===============
*/
void R_EntityMatrix (float matrix[16], vec3_t origin, vec3_t angles, unsigned char scale)
{
	float scalefactor	= ENTSCALE_DECODE(scale);
	float yaw			= DEG2RAD(angles[YAW]);
	float pitch			= angles[PITCH];
	float roll			= angles[ROLL];
	if (pitch == 0.f && roll == 0.f)
	{
		float sy = sin(yaw) * scalefactor;
		float cy = cos(yaw) * scalefactor;

		// First column
		matrix[ 0] = cy;
		matrix[ 1] = sy;
		matrix[ 2] = 0.f;
		matrix[ 3] = 0.f;

		// Second column
		matrix[ 4] = -sy;
		matrix[ 5] = cy;
		matrix[ 6] = 0.f;
		matrix[ 7] = 0.f;

		// Third column
		matrix[ 8] = 0.f;
		matrix[ 9] = 0.f;
		matrix[10] = scalefactor;
		matrix[11] = 0.f;
	}
	else
	{
		float sy, sp, sr, cy, cp, cr;
		pitch = DEG2RAD(pitch);
		roll = DEG2RAD(roll);
		sy = sin(yaw);
		sp = sin(pitch);
		sr = sin(roll);
		cy = cos(yaw);
		cp = cos(pitch);
		cr = cos(roll);

		// https://www.symbolab.com/solver/matrix-multiply-calculator FTW!

		// First column
		matrix[ 0] = scalefactor * cy*cp;
		matrix[ 1] = scalefactor * sy*cp;
		matrix[ 2] = scalefactor * sp;
		matrix[ 3] = 0.f;

		// Second column
		matrix[ 4] = scalefactor * (-cy*sp*sr - cr*sy);
		matrix[ 5] = scalefactor * (cr*cy - sy*sp*sr);
		matrix[ 6] = scalefactor * cp*sr;
		matrix[ 7] = 0.f;

		// Third column
		matrix[ 8] = scalefactor * (sy*sr - cr*cy*sp);
		matrix[ 9] = scalefactor * (-cy*sr - cr*sy*sp);
		matrix[10] = scalefactor * cr*cp;
		matrix[11] = 0.f;
	}

	// Fourth column
	matrix[12] = origin[0];
	matrix[13] = origin[1];
	matrix[14] = origin[2];
	matrix[15] = 1.f;
}

/*
=============
GL_PolygonOffset -- johnfitz

negative offset moves polygon closer to camera
=============
*/
void GL_PolygonOffset (int offset)
{
	if (gl_clipcontrol_able)
		offset = -offset;

	if (offset > 0)
	{
		glEnable (GL_POLYGON_OFFSET_FILL);
		glEnable (GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(1, offset);
	}
	else if (offset < 0)
	{
		glEnable (GL_POLYGON_OFFSET_FILL);
		glEnable (GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-1, offset);
	}
	else
	{
		glDisable (GL_POLYGON_OFFSET_FILL);
		glDisable (GL_POLYGON_OFFSET_LINE);
	}
}

/*
=============
GL_DepthRange

Wrapper around glDepthRange that handles clip control/reversed Z differences
=============
*/
void GL_DepthRange (zrange_t range)
{
	switch (range)
	{
	default:
	case ZRANGE_FULL:
		glDepthRange (0.f, 1.f);
		break;

	case ZRANGE_VIEWMODEL:
		if (gl_clipcontrol_able)
			glDepthRange (0.7f, 1.f);
		else
			glDepthRange (0.f, 0.3f);
		break;

	case ZRANGE_NEAR:
		if (gl_clipcontrol_able)
			glDepthRange (1.f, 1.f);
		else
			glDepthRange (0.f, 0.f);
		break;
	}
}

/*
=============
R_GetAlphaMode
=============
*/
alphamode_t R_GetAlphaMode (void)
{
	if (r_oit.value)
		return ALPHAMODE_OIT;
	return r_alphasort.value ? ALPHAMODE_SORTED : ALPHAMODE_BASIC;
}

/*
=============
R_GetEffectiveAlphaMode
=============
*/
alphamode_t R_GetEffectiveAlphaMode (void)
{
	if (map_checks.value)
		return ALPHAMODE_BASIC;
	return R_GetAlphaMode ();
}

/*
=============
R_SetAlphaMode
=============
*/
void R_SetAlphaMode (alphamode_t mode)
{
	Cvar_SetValueQuick (&r_oit, mode == ALPHAMODE_OIT);
	if (mode != ALPHAMODE_OIT)
		Cvar_SetValueQuick (&r_alphasort, mode == ALPHAMODE_SORTED);
}


//==============================================================================
//
// SETUP FRAME
//
//==============================================================================

static uint32_t visedict_keys[MAX_VISEDICTS];
static uint16_t visedict_order[2][MAX_VISEDICTS];
static entity_t *cl_sorted_visedicts[MAX_VISEDICTS + 1]; // +1 for worldspawn
static int cl_modtype_ofs[mod_numtypes*2 + 1]; // x2: opaque/translucent; +1: total in last slot

typedef struct framesetup_s
{
	GLuint		scene_fbo;
	GLuint		oit_fbo;
} framesetup_t;

static framesetup_t framesetup;

/*
=============
R_SortEntities
=============
*/
static void R_SortEntities (void)
{
	int i, j, pass;
	int bins[1 << (MODSORT_BITS/2)];
	int typebins[mod_numtypes*2];
	alphamode_t alphamode = R_GetEffectiveAlphaMode ();

	if (!r_drawentities.value)
		cl_numvisedicts = 0;

	// remove entities with no or invisible models
	for (i = 0, j = 0; i < cl_numvisedicts; i++)
	{
		entity_t *ent = cl_visedicts[i];
		if (!ent->model || ent->alpha == ENTALPHA_ZERO)
			continue;
		if (ent->model->type == mod_brush && R_CullModelForEntity (ent))
			continue;
		cl_visedicts[j++] = ent;
	}
	cl_numvisedicts = j;

	memset (typebins, 0, sizeof(typebins));
	if (r_drawworld.value)
		typebins[mod_brush * 2 + 0]++; // count worldspawn

	// fill entity sort key array, initial order, and per-type counts
	for (i = 0; i < cl_numvisedicts; i++)
	{
		entity_t *ent = cl_visedicts[i];
		qboolean translucent = !ENTALPHA_OPAQUE (ent->alpha);

		if (translucent && alphamode == ALPHAMODE_SORTED)
		{
			float dist, delta;
			vec3_t mins, maxs;

			R_GetEntityBounds (ent, mins, maxs);
			for (j = 0, dist = 0.f; j < 3; j++)
			{
				delta = CLAMP (mins[j], r_refdef.vieworg[j], maxs[j]) - r_refdef.vieworg[j];
				dist += delta * delta;
			}
			dist = sqrt (dist);
			visedict_keys[i] = ~CLAMP (0, (int)dist, MODSORT_MASK);
		}
		else if (translucent && alphamode != ALPHAMODE_OIT)
		{
			// Note: -1 (0xfffff) for non-static entities (firstleaf=0),
			// so they are sorted after static ones
			visedict_keys[i] = ent->firstleaf - 1;
		}
		else
		{
			if (ent->model->type == mod_alias)
				visedict_keys[i] = ent->model->sortkey | (ent->skinnum & MODSORT_FRAMEMASK);
			else
				visedict_keys[i] = ent->model->sortkey | (ent->frame & MODSORT_FRAMEMASK);
		}

		if ((unsigned)ent->model->type >= (unsigned)mod_numtypes)
			Sys_Error ("Model '%s' has invalid type %d", ent->model->name, ent->model->type);
		typebins[ent->model->type * 2 + translucent]++;

		visedict_order[0][i] = i;
	}

	// convert typebin counts into offsets
	for (i = 0, j = 0; i < countof(typebins); i++)
	{
		int tmp = typebins[i];
		cl_modtype_ofs[i] = typebins[i] = j;
		j += tmp;
	}
	cl_modtype_ofs[i] = j;

	// LSD-first radix sort: 2 passes x MODSORT_BITS/2 bits
	for (pass = 0; pass < 2; pass++)
	{
		uint16_t *src = visedict_order[pass];
		uint16_t *dst = visedict_order[pass ^ 1];
		const int mask = countof (bins) - 1;
		int shift = pass * (MODSORT_BITS/2);
		int sum;

		// count number of entries in each bin
		memset (bins, 0, sizeof(bins));
		for (i = 0; i < cl_numvisedicts; i++)
			bins[(visedict_keys[i] >> shift) & mask]++;

		// turn bin counts into offsets
		sum = 0;
		for (i = 0; i < countof (bins); i++)
		{
			int tmp = bins[i];
			bins[i] = sum;
			sum += tmp;
		}

		// reorder
		for (i = 0; i < cl_numvisedicts; i++)
			dst[bins[(visedict_keys[src[i]] >> shift) & mask]++] = src[i];
	}

	// write sorted list
	if (r_drawworld.value)
		cl_sorted_visedicts[typebins[mod_brush * 2 + 0]++] = &cl_entities[0]; // add the world as the first brush entity
	for (i = 0; i < cl_numvisedicts; i++)
	{
		entity_t *ent = cl_visedicts[visedict_order[0][i]];
		qboolean translucent = !ENTALPHA_OPAQUE (ent->alpha);
		cl_sorted_visedicts[typebins[ent->model->type * 2 + translucent]++] = ent;
	}
}

int SignbitsForPlane (mplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}

/*
=============
GL_FrustumMatrix
=============
*/
static void GL_FrustumMatrix(float matrix[16], float fovx, float fovy, float n, float f)
{
	const float w = 1.0f / tanf(fovx * 0.5f);
	const float h = 1.0f / tanf(fovy * 0.5f);

	memset(matrix, 0, 16 * sizeof(float));

	if (gl_clipcontrol_able)
	{
		// reversed Z projection matrix with the coordinate system conversion baked in
		matrix[0*4 + 2] = -n / (f - n);
		matrix[0*4 + 3] = 1.f;
		matrix[1*4 + 0] = -w;
		matrix[2*4 + 1] = h;
		matrix[3*4 + 2] = f * n / (f - n);
	}
	else
	{
		// standard projection matrix with the coordinate system conversion baked in
		matrix[0*4 + 2] = (f + n) / (f - n);
		matrix[0*4 + 3] = 1.f;
		matrix[1*4 + 0] = -w;
		matrix[2*4 + 1] = h;
		matrix[3*4 + 2] = -2.f * f * n / (f - n);
	}
}

/*
===============
ExtractFrustumPlane

Extracts the normalized frustum plane from the given view-projection matrix
that corresponds to a value of 'ndcval' on the 'axis' axis in NDC space.
===============
*/
void ExtractFrustumPlane (float mvp[16], int axis, float ndcval, qboolean flip, mplane_t *out)
{
	float scale;
	out->normal[0] =  (mvp[0*4 + axis] - ndcval * mvp[0*4 + 3]);
	out->normal[1] =  (mvp[1*4 + axis] - ndcval * mvp[1*4 + 3]);
	out->normal[2] =  (mvp[2*4 + axis] - ndcval * mvp[2*4 + 3]);
	out->dist      = -(mvp[3*4 + axis] - ndcval * mvp[3*4 + 3]);

	scale = (flip ? -1.f : 1.f) / sqrtf (DotProduct (out->normal, out->normal));
	out->normal[0] *= scale;
	out->normal[1] *= scale;
	out->normal[2] *= scale;
	out->dist      *= scale;

	out->type      = PLANE_ANYZ;
	out->signbits  = SignbitsForPlane (out);
}

/*
===============
R_SetFrustum
===============
*/
void R_SetFrustum (void)
{
        float w, h, d;
        float znear, zfar;
        float logznear, logzfar;
	float translation[16];
	float rotation[16];

	// reduce near clip distance at high FOV's to avoid seeing through walls
	w = 1.f / tanf (DEG2RAD (r_fovx) * 0.5f);
	h = 1.f / tanf (DEG2RAD (r_fovy) * 0.5f);
	d = 12.f * q_min (w, h);
        znear = CLAMP (0.5f, d, 4.f);
        zfar = gl_farclip.value;

        shadow_view_znear = znear;
        shadow_view_zfar = zfar;

        GL_FrustumMatrix(r_matproj, DEG2RAD(r_fovx), DEG2RAD(r_fovy), znear, zfar);

	// View matrix
	RotationMatrix(r_matview, DEG2RAD(-r_refdef.viewangles[ROLL]), 0);
	RotationMatrix(rotation, DEG2RAD(-r_refdef.viewangles[PITCH]), 1);
	MatrixMultiply(r_matview, rotation);
	RotationMatrix(rotation, DEG2RAD(-r_refdef.viewangles[YAW]), 2);
	MatrixMultiply(r_matview, rotation);

	TranslationMatrix(translation, -r_refdef.vieworg[0], -r_refdef.vieworg[1], -r_refdef.vieworg[2]);
	MatrixMultiply(r_matview, translation);

	// View projection matrix
	memcpy(r_matviewproj, r_matproj, 16 * sizeof(float));
	MatrixMultiply(r_matviewproj, r_matview);

	ExtractFrustumPlane (r_matviewproj, 0,  1.f, true,  &frustum[0]); // right
	ExtractFrustumPlane (r_matviewproj, 0, -1.f, false, &frustum[1]); // left
	ExtractFrustumPlane (r_matviewproj, 1, -1.f, false, &frustum[2]); // bottom
	ExtractFrustumPlane (r_matviewproj, 1,  1.f, true,  &frustum[3]); // top

	logznear = log2f (znear);
	logzfar = log2f (zfar);
	memcpy (r_framedata.viewproj, r_matviewproj, 16 * sizeof (float));
	r_framedata.zlogscale = LIGHT_TILES_Z / (logzfar - logznear);
	r_framedata.zlogbias = -r_framedata.zlogscale * logznear;
}

/*
=============
GL_NeedsSceneEffects
=============
*/
qboolean GL_NeedsSceneEffects (void)
{
	return framebufs.scene.samples > 1 || water_warp || r_refdef.scale != 1;
}

/*
=============
GL_NeedsPostprocess
=============
*/
qboolean GL_NeedsPostprocess (void)
{
	return vid_gamma.value != 1.f || vid_contrast.value != 1.f || softemu || R_GetEffectiveAlphaMode () == ALPHAMODE_OIT;
}

/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	if (!GL_NeedsSceneEffects ())
	{
		GL_BindFramebufferFunc (GL_FRAMEBUFFER, GL_NeedsPostprocess () ? framebufs.composite.fbo : 0u);
		framesetup.scene_fbo = framebufs.composite.fbo;
		framesetup.oit_fbo = framebufs.oit.fbo_composite;
		glViewport (glx + r_refdef.vrect.x, gly + glheight - r_refdef.vrect.y - r_refdef.vrect.height, r_refdef.vrect.width, r_refdef.vrect.height);
	}
	else
	{
		GL_BindFramebufferFunc (GL_FRAMEBUFFER, framebufs.scene.fbo);
		framesetup.scene_fbo = framebufs.scene.fbo;
		framesetup.oit_fbo = framebufs.oit.fbo_scene;
		glViewport (0, 0, r_refdef.vrect.width / r_refdef.scale, r_refdef.vrect.height / r_refdef.scale);
	}
}

/*
=============
R_Clear -- johnfitz -- rewritten and gutted
=============
*/
void R_Clear (void)
{
	GLbitfield clearbits = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	if (gl_clear.value)
		clearbits |= GL_COLOR_BUFFER_BIT;

	GL_SetState (glstate & ~GLS_NO_ZWRITE); // make sure depth writes are enabled
	glStencilMask (~0u);
	glClear (clearbits);
}

/*
===============
R_SetupScene -- johnfitz -- this is the stuff that needs to be done once per eye in stereo mode
===============
*/
void R_SetupScene (void)
{
	R_SetupGL ();
}

/*
===============
R_UploadFrameData
===============
*/
void R_UploadFrameData (void)
{
	GLuint	buf;
	GLbyte	*ofs;
	size_t	size;

	size = sizeof(r_lightbuffer.lightstyles) + sizeof(r_lightbuffer.lights[0]) * q_max (r_framedata.numlights, 1); // avoid zero-length array
	GL_Upload (GL_SHADER_STORAGE_BUFFER, &r_lightbuffer, size, &buf, &ofs);
	GL_BindBufferRange (GL_SHADER_STORAGE_BUFFER, 0, buf, (GLintptr)ofs, size);

	GL_Upload (GL_UNIFORM_BUFFER, &r_framedata, sizeof (r_framedata), &buf, &ofs);
	GL_BindBufferRange (GL_UNIFORM_BUFFER, 0, buf, (GLintptr)ofs, sizeof (r_framedata));
}

/*
===============
R_SetupView -- johnfitz -- this is the stuff that needs to be done once per frame, even in stereo mode
===============
*/
void R_SetupView (void)
{
	R_AnimateLight ();

	{
		int overbright_bits = CLAMP (0, (int)Q_rint (r_overbrightbits.value), 3);
		r_framedata.overbright = (float)(1 << overbright_bits);
		r_framedata._padding1 = 0.f;
	}

	r_framecount++;
	r_framedata.eyepos[0] = r_refdef.vieworg[0];
	r_framedata.eyepos[1] = r_refdef.vieworg[1];
	r_framedata.eyepos[2] = r_refdef.vieworg[2];
	r_framedata.time = cl.time;
	if (softemu == SOFTEMU_COARSE)
	{
		r_framedata.screendither = NOISESCALE * r_dither.value * r_softemu_dither_screen.value;
		r_framedata.texturedither = NOISESCALE * r_dither.value * r_softemu_dither_texture.value;

		// r_fullbright replaces the actual lightmap texture with a 2x2 50% grey one.
		// Since texture-space dithering is applied on a scale of 1/16 of a lightmap texel,
		// this would lead to massively overscaled dithering patterns, so we disable
		// texture-space dithering in this case.
		if (r_fullbright_cheatsafe)
			r_framedata.texturedither = 0.f;
	}
	else if (softemu == SOFTEMU_OFF)
	{
		r_framedata.screendither = r_dither.value * (1.f/255.f);
		r_framedata.texturedither = 0.f;
	}
	else // FINE (screen-space dithering applied during postprocessing), or BANDED (no dithering)
	{
		r_framedata.screendither = 0.f;
		r_framedata.texturedither = 0.f;
	}

	Fog_SetupFrame (); //johnfitz
	Sky_SetupFrame ();

// build the transformation matrix for the given view angles
	VectorCopy (r_refdef.vieworg, r_origin);
	AngleVectors (r_refdef.viewangles, vpn, vright, vup);

// current viewleaf
	r_oldviewleaf = r_viewleaf;
	r_viewleaf = Mod_PointInLeaf (r_origin, cl.worldmodel);

	V_SetContentsColor (r_viewleaf->contents);
	V_CalcBlend ();

	//johnfitz -- calculate r_fovx and r_fovy here
	r_fovx = r_refdef.fov_x;
	r_fovy = r_refdef.fov_y;
	water_warp = false;
	if (r_waterwarp.value)
	{
		int contents = Mod_PointInLeaf (r_origin, cl.worldmodel)->contents;
		qboolean forced = M_ForcedUnderwater ();
		if (contents == CONTENTS_WATER || contents == CONTENTS_SLIME || contents == CONTENTS_LAVA || cl.forceunderwater || forced)
		{
			double t = forced ? realtime : cl.time;
			if (r_waterwarp.value > 1.f)
			{
				//variance is a percentage of width, where width = 2 * tan(fov / 2) otherwise the effect is too dramatic at high FOV and too subtle at low FOV.  what a mess!
				r_fovx = atan(tan(DEG2RAD(r_refdef.fov_x) / 2) * (0.97 + sin(t * 1.5) * 0.03)) * 2 / M_PI_DIV_180;
				r_fovy = atan(tan(DEG2RAD(r_refdef.fov_y) / 2) * (1.03 - sin(t * 1.5) * 0.03)) * 2 / M_PI_DIV_180;
			}
			else
			{
				water_warp = true;
			}
		}
	}
	//johnfitz

	R_SetFrustum ();

        R_MarkSurfaces (); //johnfitz -- create texture chains from PVS

        R_SortEntities ();

        R_BuildShadowMap ();

        R_PushDlights ();

	//johnfitz -- cheat-protect some draw modes
	r_fullbright_cheatsafe = r_lightmap_cheatsafe = false;
	r_drawworld_cheatsafe = true;
	if (cl.maxclients == 1)
	{
		if (!r_drawworld.value) r_drawworld_cheatsafe = false;

		if (r_fullbright.value) r_fullbright_cheatsafe = true;
		else if (r_lightmap.value) r_lightmap_cheatsafe = true;
	}
	if (!cl.worldmodel->lightdata)
	{
		r_fullbright_cheatsafe = true;
		r_lightmap_cheatsafe = false;
	}
	//johnfitz
}

//==============================================================================
//
// RENDER VIEW
//
//==============================================================================

/*
=============
R_GetVisEntities
=============
*/
entity_t **R_GetVisEntities (modtype_t type, qboolean translucent, int *outcount)
{
	entity_t **entlist = cl_sorted_visedicts;
	int *ofs = cl_modtype_ofs + type * 2 + (translucent ? 1 : 0);
	*outcount = ofs[1] - ofs[0];
	return entlist + ofs[0];
}

/*
=============
R_DrawWater
=============
*/
static void R_DrawWater (qboolean translucent)
{
	entity_t **entlist = cl_sorted_visedicts;
	int *ofs = cl_modtype_ofs + 2 * mod_brush;

	if (translucent)
	{
		// all entities can have translucent water
		R_DrawBrushModels_Water (entlist + ofs[0], ofs[2] - ofs[0], true);
	}
	else
	{
		// only opaque entities can have opaque water
		R_DrawBrushModels_Water (entlist + ofs[0], ofs[1] - ofs[0], false);
	}

}

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (qboolean alphapass) //johnfitz -- added parameter
{
	int		*ofs;
	entity_t **entlist = cl_sorted_visedicts;

	GL_BeginGroup (alphapass ? "Translucent entities" : "Opaque entities");

	ofs = cl_modtype_ofs + (alphapass ? 1 : 0);
	R_DrawBrushModels  (entlist + ofs[2*mod_brush ], ofs[2*mod_brush +1] - ofs[2*mod_brush ]);
	R_DrawAliasModels  (entlist + ofs[2*mod_alias ], ofs[2*mod_alias +1] - ofs[2*mod_alias ]);
	if (!alphapass)
		R_DrawSpriteModels (entlist + cl_modtype_ofs[2*mod_sprite], cl_modtype_ofs[2*mod_sprite+2] - cl_modtype_ofs[2*mod_sprite]);

	GL_EndGroup ();
}

/*
=============
R_IsViewModelVisible
=============
*/
static qboolean R_IsViewModelVisible (void)
{
	entity_t *e = &cl.viewent;
	if (!r_drawviewmodel.value || !r_drawentities.value || chase_active.value || scr_viewsize.value >= 130)
		return false;

	if (cl.items & IT_INVISIBILITY || cl.stats[STAT_HEALTH] <= 0)
		return false;

	if (!e->model)
		return false;

	//johnfitz -- this fixes a crash
	if (e->model->type != mod_alias)
		return false;

	return true;
}

/*
=============
R_DrawViewModel -- johnfitz -- gutted
=============
*/
void R_DrawViewModel (void)
{
	entity_t *e = &cl.viewent;

	if (!R_IsViewModelVisible ())
		return;

	GL_BeginGroup ("View model");

	// hack the depth range to prevent view model from poking into walls
	GL_DepthRange (ZRANGE_VIEWMODEL);
	R_DrawAliasModels (&e, 1);
	GL_DepthRange (ZRANGE_FULL);

	GL_EndGroup ();
}

typedef struct debugvert_s
{
	vec3_t		pos;
	uint32_t	color;
} debugvert_t;

static debugvert_t	debugverts[4096];
static uint16_t		debugidx[8192];
static int			numdebugverts = 0;
static int			numdebugidx = 0;
static qboolean		debugztest = false;

/*
================
R_FlushDebugGeometry
================
*/
static void R_FlushDebugGeometry (void)
{
	if (numdebugverts && numdebugidx)
	{
		GLuint	buf;
		GLbyte	*ofs;
		unsigned int state;

		GL_UseProgram (glprogs.debug3d);
		state = GLS_BLEND_ALPHA | GLS_NO_ZWRITE | GLS_CULL_NONE | GLS_ATTRIBS(2);
		if (!debugztest)
			state |= GLS_NO_ZTEST;
		GL_SetState (state);

		GL_Upload (GL_ARRAY_BUFFER, debugverts, sizeof (debugverts[0]) * numdebugverts, &buf, &ofs);
		GL_BindBuffer (GL_ARRAY_BUFFER, buf);
		GL_VertexAttribPointerFunc (0, 3, GL_FLOAT, GL_FALSE, sizeof (debugverts[0]), ofs + offsetof (debugvert_t, pos));
		GL_VertexAttribPointerFunc (1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof (debugverts[0]), ofs + offsetof (debugvert_t, color));

		GL_Upload (GL_ELEMENT_ARRAY_BUFFER, debugidx, sizeof (debugidx[0]) * numdebugidx, &buf, &ofs);
		GL_BindBuffer (GL_ELEMENT_ARRAY_BUFFER, buf);
		glDrawElements (GL_LINES, numdebugidx, GL_UNSIGNED_SHORT, ofs);
	}

	numdebugverts = 0;
	numdebugidx = 0;
}

/*
================
R_SetDebugGeometryZTest
================
*/
static void R_SetDebugGeometryZTest (qboolean ztest)
{
	if (debugztest == ztest)
		return;
	R_FlushDebugGeometry ();
	debugztest = ztest;
}

/*
================
R_AddDebugGeometry
================
*/
static void R_AddDebugGeometry (const debugvert_t verts[], int numverts, const uint16_t idx[], int numidx)
{
	int i;

	if (numdebugverts + numverts > countof (debugverts) ||
		numdebugidx + numidx > countof (debugidx))
		R_FlushDebugGeometry ();

	for (i = 0; i < numidx; i++)
		debugidx[numdebugidx + i] = idx[i] + numdebugverts;
	numdebugidx += numidx;

	for (i = 0; i < numverts; i++)
		debugverts[numdebugverts + i] = verts[i];
	numdebugverts += numverts;
}

/*
================
R_EmitLine
================
*/
static void R_EmitLine (const vec3_t a, const vec3_t b, uint32_t color)
{
	debugvert_t verts[2];
	uint16_t idx[2];

	VectorCopy (a, verts[0].pos);
	VectorCopy (b, verts[1].pos);
	verts[0].color = color;
	verts[1].color = color;
	idx[0] = 0;
	idx[1] = 1;

	R_AddDebugGeometry (verts, 2, idx, 2);
}

/*
================
R_EmitWirePoint -- johnfitz -- draws a wireframe cross shape for point entities
================
*/
static void R_EmitWirePoint (const vec3_t origin, uint32_t color)
{
	const float Size = 8.f;
	int i;
	for (i = 0; i < 3; i++)
	{
		vec3_t a, b;
		VectorCopy (origin, a);
		VectorCopy (origin, b);
		a[i] -= Size;
		b[i] += Size;
		R_EmitLine (a, b, color);
	}
}

/*
================
R_EmitWireBox -- johnfitz -- draws one axis aligned bounding box
================
*/
static const uint16_t boxidx[12*2] = { 0,1, 0,2, 0,4, 1,3, 1,5, 2,3, 2,6, 3,7, 4,5, 4,6, 5,7, 6,7, };

static void R_EmitWireBox (const vec3_t mins, const vec3_t maxs, uint32_t color)
{
	int i;
	debugvert_t v[8];

	for (i = 0; i < 8; i++)
	{
		v[i].pos[0] = i & 1 ? mins[0] : maxs[0];
		v[i].pos[1] = i & 2 ? mins[1] : maxs[1];
		v[i].pos[2] = i & 4 ? mins[2] : maxs[2];
		v[i].color = color;
	}

	R_AddDebugGeometry (v, countof (v), boxidx, countof (boxidx));
}

/*
================
R_EmitArrow
================
*/
static void R_EmitArrow (const vec3_t from, const vec3_t to, uint32_t color)
{
	float	frac, len;
	vec3_t	center, dir, side, tmp;

	R_EmitLine (from, to, color);

	VectorSubtract (to, from, dir);
	len = VectorNormalize (dir);
	if (len < 1e-2f)
	{
		VectorCopy (vup, dir);
		VectorCopy (vright, side);
	}
	else
	{
		VectorSubtract (from, r_origin, tmp);
		CrossProduct (dir, tmp, side);
		VectorNormalize (side);
	}

	frac = realtime - floor (realtime);
	VectorLerp (from, to, frac, center);

	VectorMA (center, 8.f, side, tmp);
	VectorMA (tmp, -8.f, dir, tmp);
	R_EmitLine (tmp, center, color);

	VectorMA (tmp, -16.f, side, tmp);
	R_EmitLine (tmp, center, color);
}

/*
================
R_EmitEdictLink
================
*/
static void R_EmitEdictLink (const edict_t *from, const edict_t *to, showbboxflags_t flags)
{
	vec3_t vec_from, vec_to;

	if (!flags)
		return;

	VectorCopy (from->v.origin, vec_from);
	if (!VectorCompare (from->v.mins, from->v.maxs))
	{
		VectorMA (vec_from, 0.5f, from->v.mins, vec_from);
		VectorMA (vec_from, 0.5f, from->v.maxs, vec_from);
	}

	VectorCopy (to->v.origin, vec_to);
	if (!VectorCompare (to->v.mins, to->v.maxs))
	{
		VectorMA (vec_to, 0.5f, to->v.mins, vec_to);
		VectorMA (vec_to, 0.5f, to->v.maxs, vec_to);
	}

	if (flags == SHOWBBOX_LINK_BOTH)
		R_EmitLine (vec_from, vec_to, 0x7f7f3f7f);
	else if (flags == SHOWBBOX_LINK_OUTGOING)
		R_EmitArrow (vec_from, vec_to, 0x7f7f3f3f);
	else if (flags == SHOWBBOX_LINK_INCOMING)
		R_EmitArrow (vec_to, vec_from, 0x7f3f3f7f);
}

/*
================
R_ShowBoundingBoxesFilter

r_showbboxes_filter artifact =trigger_secret #42
================
*/
char r_showbboxes_filter_strings[MAXCMDLINE];
qboolean r_showbboxes_filter_byindex;

static qboolean R_ShowBoundingBoxesFilter (edict_t *ed)
{
	char entnum[16] = "";
	const char *classname = NULL;
	const char *filter_p = r_showbboxes_filter_strings;

	if (!r_showbboxes_filter_strings[0])
		return true;

	if (r_showbboxes_filter_byindex)
		q_snprintf (entnum, sizeof (entnum), "%d", NUM_FOR_EDICT (ed));

	if (ed->v.classname)
		classname = PR_GetString (ed->v.classname);

	for (filter_p = r_showbboxes_filter_strings; *filter_p; filter_p += strlen (filter_p) + 1)
	{
		if (*filter_p == '#')
		{
			if (!strcmp (entnum, filter_p + 1))
				return true;
			continue;
		}

		if (!classname)
			continue;

		if (*filter_p == '=')
		{
			if (!strcmp (classname, filter_p + 1))
				return true;
			continue;
		}

		if (strstr (classname, filter_p) != NULL)
			return true;
	}

	return false;
}

static edict_t **bbox_edicts = NULL;		// all edicts shown by r_showbboxes & co
edict_t **bbox_linked = NULL;				// focused edict, followed by edicts linked from/to it

/*
================
R_AddHighlightedEntity
================
*/
static void R_AddHighlightedEntity (edict_t *ed, showbboxflags_t flags)
{
	if (ed->showbboxframe != r_framecount)
	{
		ed->showbboxframe = r_framecount;
		ed->showbboxflags = SHOWBBOX_LINK_NONE;
		VEC_PUSH (bbox_edicts, ed);
	}

	if (!(ed->showbboxflags & flags) && (int)r_showbboxes_links.value & flags)
	{
		VEC_PUSH (bbox_linked, ed);
		ed->showbboxflags |= flags;
	}
}

/*
================
R_ClearBoundingBoxes
================
*/
void R_ClearBoundingBoxes (void)
{
	VEC_CLEAR (bbox_edicts);
	VEC_CLEAR (bbox_linked);
}

/*
================
R_ShowBoundingBoxes -- johnfitz

draw bounding boxes -- the server-side boxes, not the renderer cullboxes
================
*/
static void R_ShowBoundingBoxes (void)
{
	extern		edict_t *sv_player;
	byte		*pvs;
	vec3_t		mins,maxs;
	edict_t		*ed, *focused;
	int			i, j, mode;
	uint32_t	color;
	qcvm_t 		*oldvm;	//in case we ever draw a scene from within csqc.
	float		dist, bestdist, extend;
	vec3_t		rcpdelta;

	VEC_CLEAR (bbox_edicts);
	VEC_CLEAR (bbox_linked);
	focused = NULL;

	mode = abs ((int)r_showbboxes.value);
	if ((!mode && !r_showfields.value) || cl.maxclients > 1 || !r_drawentities.value || !sv.active)
		return;

	GL_BeginGroup ("Show bounding boxes");

	R_SetDebugGeometryZTest (false);

	oldvm = qcvm;
	PR_SwitchQCVM(NULL);
	PR_SwitchQCVM(&sv.qcvm);

	// Use PVS if r_showbboxes >= 2, or if r_showbboxes is 0 (which means r_showfields is active)
	if (mode >= 2 || mode == 0)
	{
		vec3_t org;
		VectorAdd (sv_player->v.origin, sv_player->v.view_ofs, org);
		pvs = SV_FatPVS (org, sv.worldmodel);
	}
	else
		pvs = NULL;

	// Compute ray reciprocal delta
	for (i = 0; i < 3; i++)
		rcpdelta[i] = 1.f / (gl_farclip.value * vpn[i]);

	// Iterate over all server entities
	bestdist = FLT_MAX;
	for (i=1, ed=NEXT_EDICT(qcvm->edicts) ; i<qcvm->num_edicts ; i++, ed=NEXT_EDICT(ed))
	{
		if (ed == sv_player || ed->free)
			continue; // don't draw player's own bbox or freed edicts

		if (r_showbboxes_think.value && (ed->v.nextthink <= 0) == (r_showbboxes_think.value > 0))
			continue;

		if (r_showbboxes_health.value && (ed->v.health <= 0) == (r_showbboxes_health.value > 0))
			continue;

		// Compute bounding box (16 units wide for point entities)
		extend = VectorCompare (ed->v.mins, ed->v.maxs) ? 8.f : 0.f;
		for (j = 0; j < 3; j++)
		{
			mins[j] = ed->v.origin[j] + ed->v.mins[j] - extend;
			maxs[j] = ed->v.origin[j] + ed->v.maxs[j] + extend;
		}

		// Frustum culling
		if (R_CullBox (mins, maxs))
			continue;

		// Classname or edict num filter
		if (!R_ShowBoundingBoxesFilter(ed))
			continue;

		// PVS filter
		if (pvs)
		{
			qboolean inpvs =
				ed->num_leafs ?
					SV_EdictInPVS (ed, pvs) :
					SV_BoxInPVS (ed->v.absmin, ed->v.absmax, pvs, sv.worldmodel->nodes)
			;
			if (!inpvs)
				continue;
		}

		// Keep track of the closest bounding box intersecting the center ray
		// Note: if we're inside the box (dist == 0), we ignore this entity
		if (RayVsBox (r_origin, rcpdelta, mins, maxs, &dist) && dist > 0.f && dist < bestdist)
		{
			bestdist = dist;
			focused = ed;
		}

		// Add edict to list
		R_AddHighlightedEntity (ed, SHOWBBOX_LINK_NONE);
	}

	if (focused)
		VEC_PUSH (bbox_linked, focused);

	if (focused && r_showbboxes_links.value)
	{
		// Find outgoing links (entity field references other than .chain)
		if ((int)r_showbboxes_links.value & SHOWBBOX_LINK_OUTGOING)
		{
			for (i = 0; i < qcvm->numentityfields; i++)
			{
				eval_t *val = (eval_t *)((char *)&focused->v + qcvm->entityfieldofs[i]);
				if (qcvm->entityfieldofs[i] == offsetof (entvars_t, chain) || !val->edict)
					continue;
				ed = PROG_TO_EDICT (val->edict);
				if (ed == focused || ed->free || ed == sv_player)
					continue;
				R_AddHighlightedEntity (ed, SHOWBBOX_LINK_OUTGOING);
			}
		}

		// Inspect all other edicts to find incoming links
		// (either entity field references or target/targetname matches)
		if ((int)r_showbboxes_links.value & SHOWBBOX_LINK_INCOMING || r_showbboxes_targets.value)
		{
			const char *focus_target = PR_GetString (focused->v.target);
			const char *focus_targetname = PR_GetString (focused->v.targetname);

			for (i=1, ed=NEXT_EDICT(qcvm->edicts) ; i<qcvm->num_edicts ; i++, ed=NEXT_EDICT(ed))
			{
				if (ed == sv_player || ed->free || ed == focused)
					continue;

				// Check target/targetname matches
				if (r_showbboxes_targets.value && (*focus_target || *focus_targetname))
				{
					const char *target = PR_GetString (ed->v.target);
					const char *targetname = PR_GetString (ed->v.targetname);

					if (*focus_targetname && !strcmp (focus_targetname, target))
						R_AddHighlightedEntity (ed, SHOWBBOX_LINK_INCOMING);
					if (*focus_target && !strcmp (focus_target, targetname))
						R_AddHighlightedEntity (ed, SHOWBBOX_LINK_OUTGOING);
				}

				// Check for entity field references (other than .chain)
				if ((int)r_showbboxes_links.value & SHOWBBOX_LINK_INCOMING)
				{
					for (j = 0; j < qcvm->numentityfields; j++)
					{
						eval_t *val = (eval_t *)((char *)&ed->v + qcvm->entityfieldofs[j]);
						if (qcvm->entityfieldofs[i] == offsetof (entvars_t, chain) || !val->edict)
							continue;
						if (PROG_TO_EDICT (val->edict) == focused)
							R_AddHighlightedEntity (ed, SHOWBBOX_LINK_INCOMING);
					}
				}
			}
		}

		// Draw all links
		for (j = 0; j < (int) VEC_SIZE (bbox_linked); j++)
			R_EmitEdictLink (focused, bbox_linked[j], bbox_linked[j]->showbboxflags);
	}

	// Draw all the matching edicts
	for (i = 0; i < (int) VEC_SIZE (bbox_edicts); i++)
	{
		ed = bbox_edicts[i];

		if (ed == focused)
			color = 0xffffffff;
		else if (ed->showbboxflags)
			color = 0xaaaaaaaa;
		else if (r_showbboxes.value > 0.f)
		{
			int modelindex = (int)ed->v.modelindex;
			color = 0x7f800080;
			if (modelindex >= 0 && modelindex < MAX_MODELS && sv.models[modelindex])
			{
				switch (sv.models[modelindex]->type)
				{
					case mod_brush:  color = 0x7fff8080; break;
					case mod_alias:  color = 0x7f408080; break;
					case mod_sprite: color = 0x7f4040ff; break;
					default:
						break;
				}
			}
			if (ed->v.health > 0)
				color = 0x7f0000ff;
		}
		else if (r_showbboxes.value < 0.f)
			color = 0x7fffffff;
		else
			color = 0x5f7f7f7f;

		if (VectorCompare (ed->v.mins, ed->v.maxs))
		{
			//point entity
			R_EmitWirePoint (ed->v.origin, color);
		}
		else
		{
			//box entity
			VectorAdd (ed->v.mins, ed->v.origin, mins);
			VectorAdd (ed->v.maxs, ed->v.origin, maxs);
			R_EmitWireBox (mins, maxs, color);
		}
	}

	VEC_CLEAR (bbox_edicts);

	PR_SwitchQCVM(NULL);
	PR_SwitchQCVM(oldvm);

	R_FlushDebugGeometry ();

	Sbar_Changed (); //so we don't get dots collecting on the statusbar

	GL_EndGroup ();
}

/*
===============
R_ShowPointFile
===============
*/
static void R_ShowPointFile (void)
{
	size_t i;

	if (VEC_SIZE (r_pointfile) == 0)
		return;

	GL_BeginGroup ("Point file");
	R_SetDebugGeometryZTest (true);
	for (i = 1; i < VEC_SIZE (r_pointfile); i++)
		R_EmitArrow (r_pointfile[i - 1], r_pointfile[i], 0xff3f3f7f);
	R_FlushDebugGeometry ();
	GL_EndGroup ();
}

/*
===============
Collinear
===============
*/
static qboolean Collinear (const vec3_t a, const vec3_t b, const vec3_t c)
{
	return Distance (a, b) + Distance (b, c) < Distance (a, c) * 1.00001f;
}

/*
===============
R_ReadPointFile_f
===============
*/
void R_ReadPointFile_f (void)
{
	FILE		*f;
	vec3_t		org;
	int			r, n;
	qboolean	leakmode;
	char		name[MAX_QPATH];

	VEC_CLEAR (r_pointfile);

	if (cls.state != ca_connected)
		return;			// need an active map.

	q_snprintf (name, sizeof(name), "maps/%s.pts", cl.mapname);
	leakmode = Cmd_Argc () >= 2 && !strcmp (Cmd_Argv (1), "leak");

	COM_FOpenFile (name, &f, NULL);
	if (!f)
	{
		Con_Printf ("couldn't open %s\n", name);
		return;
	}

	if (!leakmode)
		Con_Printf ("Reading %s...\n", name);
	org[0] = org[1] = org[2] = 0; // silence pesky compiler warnings

	for (r = 0; fscanf (f,"%f %f %f\n", &org[0], &org[1], &org[2]) == 3; r++)
	{
		Vec_Append ((void **) &r_pointfile, sizeof (r_pointfile[0]), &org, 1);
		n = (int) VEC_SIZE (r_pointfile);
		if (n >= 3 && Collinear (r_pointfile[n-3], r_pointfile[n-2], r_pointfile[n-1]))
		{
			VectorCopy (r_pointfile[n-1], r_pointfile[n-2]);
			VEC_POP (r_pointfile);
		}
	}

	fclose (f);

	if (leakmode)
		Con_Warning ("map appears to have leaks!\n");
	else
		Con_Printf ("%i points read (%i significant)\n", r, (int) VEC_SIZE (r_pointfile));
}

/*
================
R_ShowTris -- johnfitz
================
*/
void R_ShowTris (void)
{
	int		*ofs;
	entity_t **entlist = cl_sorted_visedicts;

	if (r_showtris.value < 1 || r_showtris.value > 2 || cl.maxclients > 1)
		return;

	GL_BeginGroup ("Show tris");

	Fog_DisableGFog (); //johnfitz
	R_UploadFrameData ();

	if (r_showtris.value == 1)
		GL_DepthRange (ZRANGE_NEAR);
	glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	GL_PolygonOffset (OFFSET_SHOWTRIS);

	ofs = cl_modtype_ofs;
	R_DrawBrushModels_ShowTris  (entlist + ofs[2*mod_brush ], ofs[2*mod_brush +2] - ofs[2*mod_brush ]);
	R_DrawAliasModels_ShowTris  (entlist + ofs[2*mod_alias ], ofs[2*mod_alias +2] - ofs[2*mod_alias ]);
	R_DrawSpriteModels_ShowTris (entlist + ofs[2*mod_sprite], ofs[2*mod_sprite+2] - ofs[2*mod_sprite]);

	// viewmodel
	if (R_IsViewModelVisible ())
	{
		entity_t *e = &cl.viewent;

		if (r_showtris.value != 1.f)
			GL_DepthRange (ZRANGE_VIEWMODEL);

		R_DrawAliasModels_ShowTris (&e, 1);

		GL_DepthRange (ZRANGE_FULL);
	}

	R_DrawParticles_ShowTris ();

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	GL_PolygonOffset (OFFSET_NONE);
	if (r_showtris.value == 1)
		GL_DepthRange (ZRANGE_FULL);

	Sbar_Changed (); //so we don't get dots collecting on the statusbar

	GL_EndGroup ();
}

/*
================
R_BeginTranslucency
================
*/
static void R_BeginTranslucency (void)
{
	static const float zeroes[4] = {0.f, 0.f, 0.f, 0.f};
	static const float ones[4] = {1.f, 1.f, 1.f, 1.f};

	GL_BeginGroup ("Translucent objects");

	if (R_GetEffectiveAlphaMode () == ALPHAMODE_OIT)
	{
		GL_BindFramebufferFunc (GL_FRAMEBUFFER, framesetup.oit_fbo);
		GL_ClearBufferfvFunc (GL_COLOR, 0, zeroes);
		GL_ClearBufferfvFunc (GL_COLOR, 1, ones);

		glEnable (GL_STENCIL_TEST);
		glStencilMask (2);
		glStencilFunc (GL_ALWAYS, 2, 2);
		glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);
	}
}

/*
================
R_EndTranslucency
================
*/
static void R_EndTranslucency (void)
{
	if (R_GetEffectiveAlphaMode () == ALPHAMODE_OIT)
	{
		GL_BeginGroup  ("OIT resolve");

		GL_BindFramebufferFunc (GL_FRAMEBUFFER, framesetup.scene_fbo);

		glStencilFunc (GL_EQUAL, 2, 2);
		glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);

		GL_UseProgram (glprogs.oit_resolve[framebufs.scene.samples > 1]);
		GL_SetState (GLS_BLEND_ALPHA | GLS_NO_ZTEST | GLS_NO_ZWRITE | GLS_CULL_NONE | GLS_ATTRIBS(0));
		GL_BindNative (GL_TEXTURE0, framebufs.scene.samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, framebufs.oit.accum_tex);
		GL_BindNative (GL_TEXTURE1, framebufs.scene.samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, framebufs.oit.revealage_tex);

		glDrawArrays (GL_TRIANGLES, 0, 3);

		glDisable (GL_STENCIL_TEST);

		GL_EndGroup ();
	}

	GL_EndGroup (); // translucent objects
}

/*
================
R_RenderScene
================
*/
void R_RenderScene (void)
{
	R_SetupScene (); //johnfitz -- this does everything that should be done once per call to RenderScene

	R_Clear ();

	Fog_EnableGFog (); //johnfitz

	R_DrawViewModel (); //johnfitz -- moved here from R_RenderView

	S_ExtraUpdate (); // don't let sound get messed up if going slow

	R_DrawEntitiesOnList (false); //johnfitz -- false means this is the pass for nonalpha entities

	R_DrawParticles (false);

	Sky_DrawSky (); //johnfitz

	R_DrawWater (false);

	R_BeginTranslucency ();

	R_DrawWater (true);

	R_DrawEntitiesOnList (true); //johnfitz -- true means this is the pass for alpha entities

	R_DrawParticles (true);

	R_EndTranslucency ();

	R_ShowTris (); //johnfitz

	R_ShowBoundingBoxes (); //johnfitz

	R_ShowPointFile ();
}

/*
================
R_WarpScaleView

The r_scale cvar allows rendering the 3D view at 1/2, 1/3, or 1/4 resolution.
This function scales the reduced resolution 3D view back up to fill 
r_refdef.vrect. This is for emulating a low-resolution pixellated look,
or possibly as a perforance boost on slow graphics cards.
================
*/
void R_WarpScaleView (void)
{
	int srcx, srcy, srcw, srch;
	float smax, tmax;
	qboolean msaa = framebufs.scene.samples > 1;
	qboolean needwarpscale;
	GLuint fbodest;
	double t;

	if (!GL_NeedsSceneEffects ())
		return;

	srcx = glx + r_refdef.vrect.x;
	srcy = gly + glheight - r_refdef.vrect.y - r_refdef.vrect.height;
	srcw = r_refdef.vrect.width / r_refdef.scale;
	srch = r_refdef.vrect.height / r_refdef.scale;

	needwarpscale = r_refdef.scale != 1 || water_warp || (v_blend[3] && gl_polyblend.value && !softemu);
	fbodest = GL_NeedsPostprocess () ? framebufs.composite.fbo : 0;

	if (msaa)
	{
		GL_BeginGroup ("MSAA resolve");

		GL_BindFramebufferFunc (GL_READ_FRAMEBUFFER, framebufs.scene.fbo);
		if (needwarpscale)
		{
			GL_BindFramebufferFunc (GL_DRAW_FRAMEBUFFER, framebufs.resolved_scene.fbo);
			GL_BlitFramebufferFunc (0, 0, srcw, srch, 0, 0, srcw, srch, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
		else
		{
			GL_BindFramebufferFunc (GL_DRAW_FRAMEBUFFER, fbodest);
			GL_BlitFramebufferFunc (0, 0, srcw, srch, srcx, srcy, srcx + srcw, srcy + srch, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}

		GL_EndGroup ();
	}

	GL_BindFramebufferFunc (GL_FRAMEBUFFER, fbodest);
	glViewport (srcx, srcy, r_refdef.vrect.width, r_refdef.vrect.height);

	if (!needwarpscale)
		return;

	GL_BeginGroup ("Warp/scale view");

	smax = srcw/(float)vid.width;
	tmax = srch/(float)vid.height;

	GL_UseProgram (glprogs.warpscale[water_warp]);
	GL_SetState (GLS_BLEND_OPAQUE | GLS_NO_ZTEST | GLS_NO_ZWRITE | GLS_CULL_NONE | GLS_ATTRIBS(0));

	t = M_ForcedUnderwater () ? realtime : cl.time;
	GL_Uniform4fFunc (0, smax, tmax, water_warp ? 1.f/256.f : 0.f, (float)t);
	if (v_blend[3] && gl_polyblend.value && !softemu)
		GL_Uniform4fvFunc (1, 1, v_blend);
	else
		GL_Uniform4fFunc (1, 0.f, 0.f, 0.f, 0.f);
	GL_BindNative (GL_TEXTURE0, GL_TEXTURE_2D, msaa ? framebufs.resolved_scene.color_tex : framebufs.scene.color_tex);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, water_warp && msaa ? GL_LINEAR : GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, water_warp && msaa ? GL_LINEAR : GL_NEAREST);

	glDrawArrays (GL_TRIANGLES, 0, 3);

	GL_EndGroup ();
}

/*
================
R_RenderView
================
*/
void R_RenderView (void)
{
	double	time1, time2;

	if (r_norefresh.value)
		return;

	if (!cl.worldmodel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	time1 = 0; /* avoid compiler warning */
	if (r_speeds.value)
	{
		glFinish ();
		time1 = Sys_DoubleTime ();

		//johnfitz -- rendering statistics
		rs_brushpolys = rs_aliaspolys = rs_skypolys =
		rs_dynamiclightmaps = rs_aliaspasses = rs_skypasses = rs_brushpasses = 0;
	}
	else if (gl_finish.value)
		glFinish ();

	R_SetupView (); //johnfitz -- this does everything that should be done once per frame
	R_RenderScene ();
	R_WarpScaleView ();

	//johnfitz -- modified r_speeds output
	time2 = Sys_DoubleTime ();
	if (r_pos.value)
		Con_Printf ("x %i y %i z %i (pitch %i yaw %i roll %i)\n",
					(int)cl_entities[cl.viewentity].origin[0],
					(int)cl_entities[cl.viewentity].origin[1],
					(int)cl_entities[cl.viewentity].origin[2],
					(int)cl.viewangles[PITCH],
					(int)cl.viewangles[YAW],
					(int)cl.viewangles[ROLL]);
	else if (r_speeds.value == 2)
		Con_Printf ("%3i ms  %4i/%4i wpoly %4i/%4i epoly %3i lmap %4i/%4i sky %1.1f mtex\n",
					(int)((time2-time1)*1000),
					rs_brushpolys,
					rs_brushpasses,
					rs_aliaspolys,
					rs_aliaspasses,
					rs_dynamiclightmaps,
					rs_skypolys,
					rs_skypasses,
					TexMgr_FrameUsage ());
	else if (r_speeds.value)
		Con_Printf ("%3i ms  %4i wpoly %4i epoly %3i lmap\n",
					(int)((time2-time1)*1000),
					rs_brushpolys,
					rs_aliaspolys,
					rs_dynamiclightmaps);
	//johnfitz
}

