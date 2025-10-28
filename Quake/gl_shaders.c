/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
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
#include "gl_shaders.h"
#include "q_ctype.h"

glprogs_t glprogs;
static GLuint gl_programs[128];
static GLuint gl_current_program;
static int gl_num_programs;

/*
=============
GL_InitError
=============
*/
static void GL_InitError (const char *message, ...)
{
	const char *fmt;
	char buf[4096];
	size_t len;
	va_list argptr;

	va_start (argptr, message);
	q_vsnprintf (buf, sizeof (buf), message, argptr);
	va_end (argptr);

	len = strlen (buf);
	while (len && q_isspace (buf[len - 1]))
		buf[--len] = '\0';

	fmt = 
		"Your system appears to meet the minimum requirements,\n"
		"however an error was encountered during OpenGL initialization.\n"
		"This could be caused by a driver or an engine bug.\n"
		"Please report this issue, including the following details:\n"
		"\n"
		"%s\n"
		"\n"
		"Engine:	Ironwail " IRONWAIL_VER_STRING " (%d-bit)\n"
		"OpenGL:	%s\n"
		"GPU:   	%s\n"
		"Vendor:	%s\n"
#if defined(_WIN32)
		"\n"
		"(Note: you can press Ctrl+C to copy this text to clipboard)"
#endif
	;

	Sys_Error (
		fmt,
		buf,
		(int) sizeof (void *) * 8,
		gl_version,
		gl_renderer,
		gl_vendor
	);
}

/*
=============
AppendString
=============
*/
static qboolean AppendString (char **dst, const char *dstend, const char *str, int len)
{
	int avail = dstend - *dst;
	if (len < 0)
		len = Q_strlen (str);
	if (len + 1 > avail)
		return false;
	memcpy (*dst, str, len);
	(*dst)[len] = 0;
	*dst += len;
	return true;
}

/*
=============
GL_CreateShader
=============
*/
static GLuint GL_CreateShader (GLenum type, const char *source, const char *extradefs, const char *name)
{
	const char *strings[16];
	const char *typestr = NULL;
	char header[256];
	int numstrings = 0;
	GLint status;
	GLuint shader;

	switch (type)
	{
		case GL_VERTEX_SHADER:
			typestr = "vertex";
			break;
		case GL_FRAGMENT_SHADER:
			typestr = "fragment";
			break;
		case GL_COMPUTE_SHADER:
			typestr = "compute";
			break;
		default:
			Sys_Error ("GL_CreateShader: unknown type 0x%X for %s", type, name);
			break;
	}

	q_snprintf (header, sizeof (header),
		"#version 430\n"
		"\n"
		"#define BINDLESS %d\n"
		"#define REVERSED_Z %d\n",
		gl_bindless_able,
		gl_clipcontrol_able
	);
	strings[numstrings++] = header;

	if (extradefs && *extradefs)
		strings[numstrings++] = extradefs;
	strings[numstrings++] = source;

	shader = GL_CreateShaderFunc (type);
	GL_ObjectLabelFunc (GL_SHADER, shader, -1, name);
	GL_ShaderSourceFunc (shader, numstrings, strings, NULL);
	GL_CompileShaderFunc (shader);
	GL_GetShaderivFunc (shader, GL_COMPILE_STATUS, &status);

	if (status != GL_TRUE)
	{
		char infolog[1024];
		memset(infolog, 0, sizeof(infolog));
		GL_GetShaderInfoLogFunc (shader, sizeof(infolog), NULL, infolog);
		GL_InitError ("Error compiling %s %s shader:\n\n%s", name, typestr, infolog);
	}

	return shader;
}

/*
=============
GL_CreateProgramFromShaders
=============
*/
static GLuint GL_CreateProgramFromShaders (const GLuint *shaders, int numshaders, const char *name)
{
	GLuint program;
	GLint status;

	program = GL_CreateProgramFunc ();
	GL_ObjectLabelFunc (GL_PROGRAM, program, -1, name);

	while (numshaders-- > 0)
	{
		GL_AttachShaderFunc (program, *shaders);
		GL_DeleteShaderFunc (*shaders);
		++shaders;
	}

	GL_LinkProgramFunc (program);
	GL_GetProgramivFunc (program, GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		char infolog[1024];
		memset(infolog, 0, sizeof(infolog));
		GL_GetProgramInfoLogFunc (program, sizeof(infolog), NULL, infolog);
		GL_InitError ("Error linking %s program:\n\n%s", name, infolog);
	}

	if (gl_num_programs == countof(gl_programs))
		Sys_Error ("gl_programs overflow");
	gl_programs[gl_num_programs] = program;
	gl_num_programs++;

	return program;
}

/*
====================
GL_CreateProgramFromSources
====================
*/
static GLuint GL_CreateProgramFromSources (int count, const GLchar **sources, const GLenum *types, const char *name, va_list argptr)
{
	char macros[1024];
	char eval[256];
	char *pipe;
	int i, realcount;
	GLuint shaders[2];

	if (count <= 0 || count > 2)
		Sys_Error ("GL_CreateProgramFromSources: invalid source count (%d)", count);

	q_vsnprintf (eval, sizeof (eval), name, argptr);
	macros[0] = 0;

	pipe = strchr (name, '|');
	if (pipe) // parse symbol list and generate #defines
	{
		char *dst = macros;
		char *dstend = macros + sizeof (macros);
		char *src = eval + 1 + (pipe - name);

		while (*src == ' ')
			src++;

		while (*src)
		{
			char *srcend = src + 1;
			while (*srcend && *srcend != ';')
				srcend++;

			if (!AppendString (&dst, dstend, "#define ", 8) ||
				!AppendString (&dst, dstend, src, srcend - src) ||
				!AppendString (&dst, dstend, "\n", 1))
				Sys_Error ("GL_CreateProgram: symbol overflow for %s", eval);

			src = srcend;
			while (*src == ';' || *src == ' ')
				src++;
		}

		AppendString (&dst, dstend, "\n", 1);
	}

	name = eval;

	realcount = 0;
	for (i = 0; i < count; i++)
		if (sources[i])
			shaders[realcount++] = GL_CreateShader (types[i], sources[i], macros, name);

	return GL_CreateProgramFromShaders (shaders, realcount, name);
}

/*
====================
GL_CreateProgram

Compiles and returns GLSL program.
====================
*/
static FUNC_PRINTF(3,4) GLuint GL_CreateProgram (const GLchar *vertSource, const GLchar *fragSource, const char *name, ...)
{
	const GLchar *sources[2] = {vertSource, fragSource};
	GLenum types[2] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
	va_list argptr;
	GLuint program;

	va_start (argptr, name);
	program = GL_CreateProgramFromSources (2, sources, types, name, argptr);
	va_end (argptr);

	return program;
}

/*
====================
GL_CreateComputeProgram

Compiles and returns GLSL program.
====================
*/
static FUNC_PRINTF(2,3) GLuint GL_CreateComputeProgram (const GLchar *source, const char *name, ...)
{
	GLenum type = GL_COMPUTE_SHADER;
	va_list argptr;
	GLuint program;

	va_start (argptr, name);
	program = GL_CreateProgramFromSources (1, &source, &type, name, argptr);
	va_end (argptr);

	return program;
}

/*
====================
GL_UseProgram
====================
*/
void GL_UseProgram (GLuint program)
{
	if (program == gl_current_program)
		return;
	gl_current_program = program;
	GL_UseProgramFunc (program);
}

/*
====================
GL_ClearCachedProgram

This must be called if you do anything that could make the cached program
invalid (e.g. manually binding, destroying the context).
====================
*/
void GL_ClearCachedProgram (void)
{
	gl_current_program = 0;
	GL_UseProgramFunc (0);
}

/*
=============
GL_CreateShaders
=============
*/
void GL_CreateShaders (void)
{
	int palettize, dither, mode, alphatest, warp, oit;

	glprogs.gui = GL_CreateProgram (gui_vertex_shader, gui_fragment_shader, "gui");
	glprogs.viewblend = GL_CreateProgram (viewblend_vertex_shader, viewblend_fragment_shader, "viewblend");
	for (warp = 0; warp < 2; warp++)
		glprogs.warpscale[warp] = GL_CreateProgram (warpscale_vertex_shader, warpscale_fragment_shader, "view warp/scale|WARP %d", warp);
	for (palettize = 0; palettize < 3; palettize++)
		glprogs.postprocess[palettize] = GL_CreateProgram (postprocess_vertex_shader, postprocess_fragment_shader, "postprocess|PALETTIZE %d", palettize);

	for (mode = 0; mode < 2; mode++)
		glprogs.oit_resolve[mode] = GL_CreateProgram (oit_resove_vertex_shader, oit_resove_fragment_shader, "oit resolve|MSAA %d", mode);

	for (oit = 0; oit < 2; oit++)
		for (dither = 0; dither < 3; dither++)
			for (mode = 0; mode < 3; mode++)
				glprogs.world[oit][dither][mode] = GL_CreateProgram (world_vertex_shader, world_fragment_shader, "world|OIT %d; DITHER %d; MODE %d", oit, dither, mode);

	for (dither = 0; dither < 2; dither++)
	{
		for (oit = 0; oit < 2; oit++)
		{
			glprogs.water[oit][dither] = GL_CreateProgram (water_vertex_shader, water_fragment_shader, "water|OIT %d; DITHER %d", oit, dither);
			glprogs.particles[oit][dither] = GL_CreateProgram (particles_vertex_shader, particles_fragment_shader, "particles|OIT %d; DITHER %d", oit, dither);
		}
		for (mode = 0; mode < 2; mode++)
			glprogs.skycubemap[mode][dither] = GL_CreateProgram (sky_cubemap_vertex_shader, sky_cubemap_fragment_shader, "sky cubemap|ANIM %d; DITHER %d", mode, dither);
		glprogs.skylayers[dither] = GL_CreateProgram (sky_layers_vertex_shader, sky_layers_fragment_shader, "sky layers|DITHER %d", dither);
		glprogs.skyboxside[dither] = GL_CreateProgram (sky_boxside_vertex_shader, sky_boxside_fragment_shader, "skybox side|DITHER %d", dither);
		glprogs.sprites[dither] = GL_CreateProgram (sprites_vertex_shader, sprites_fragment_shader, "sprites|DITHER %d", dither);
	}
	glprogs.skystencil = GL_CreateProgram (skystencil_vertex_shader, NULL, "sky stencil");

	int poseverttype;
	for (oit = 0; oit < 2; oit++)
		for (mode = 0; mode < 3; mode++)
			for (alphatest = 0; alphatest < 2; alphatest++)
				for (poseverttype = 0; poseverttype < 3; poseverttype++) 
					glprogs.alias[oit][mode][alphatest][poseverttype] =
					GL_CreateProgram (alias_vertex_shader, alias_fragment_shader, "alias|OIT %d; MODE %d; ALPHATEST %d; POSEVERTTYPE %d", oit, mode, alphatest, poseverttype);
	glprogs.alias_numverts_loc = GL_GetUniformLocationFunc (glprogs.alias[0][0][0][2], "u_numVerts");
	glprogs.alias_numposes_loc = GL_GetUniformLocationFunc (glprogs.alias[0][0][0][2], "u_numPoses");

	glprogs.debug3d = GL_CreateProgram (debug3d_vertex_shader, debug3d_fragment_shader, "debug3d");

	glprogs.clear_indirect = GL_CreateComputeProgram (clear_indirect_compute_shader, "clear indirect draw params");
	glprogs.gather_indirect = GL_CreateComputeProgram (gather_indirect_compute_shader, "indirect draw gather");
	glprogs.cull_mark = GL_CreateComputeProgram (cull_mark_compute_shader, "cull/mark");
	glprogs.cluster_lights = GL_CreateComputeProgram (cluster_lights_compute_shader, "light cluster");
	for (mode = 0; mode < 3; mode++)
		glprogs.palette_init[mode] = GL_CreateComputeProgram (palette_init_compute_shader, "palette init|MODE %d", mode);
	glprogs.palette_postprocess = GL_CreateComputeProgram (palette_postprocess_compute_shader, "palette postprocess");
}

/*
=============
GL_DeleteShaders
=============
*/
void GL_DeleteShaders (void)
{
	int i;
	for (i = 0; i < gl_num_programs; i++)
	{
		GL_DeleteProgramFunc (gl_programs[i]);
		gl_programs[i] = 0;
	}
	gl_num_programs = 0;

	GL_UseProgramFunc (0);
	gl_current_program = 0;

	memset (&glprogs, 0, sizeof(glprogs));
}
