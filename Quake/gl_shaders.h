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

////////////////////////////////////////////////////////////////
//
// Compile-time options
//
////////////////////////////////////////////////////////////////

#define LINEAR_SPACE_OIT			0
#define SHOW_ACTIVE_LIGHT_CLUSTERS	0
#define SHOW_WORLD_NORMALS			0

////////////////////////////////////////////////////////////////
//
// GUI
//
////////////////////////////////////////////////////////////////

static const char gui_vertex_shader[] =
"layout(location=0) in vec2 in_pos;\n"
"layout(location=1) in vec2 in_uv;\n"
"layout(location=2) in vec4 in_color;\n"
"\n"
"layout(location=0) centroid out vec2 out_uv;\n"
"layout(location=1) centroid out vec4 out_color;\n"
"\n"
"void main()\n"
"{\n"
"	gl_Position = vec4(in_pos, 0.0, 1.0);\n"
"	out_uv = in_uv;\n"
"	out_color = in_color;\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char gui_fragment_shader[] =
"layout(binding=0) uniform sampler2D Tex;\n"
"\n"
"layout(location=0) centroid in vec2 in_uv;\n"
"layout(location=1) centroid in vec4 in_color;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	out_fragcolor = texture(Tex, in_uv) * in_color;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// View blend
//
////////////////////////////////////////////////////////////////

static const char viewblend_vertex_shader[] =
"void main()\n"
"{\n"
"	ivec2 v = ivec2(gl_VertexID & 1, gl_VertexID >> 1);\n"
"	gl_Position = vec4(vec2(v) * 4.0 - 1.0, 0.0, 1.0);\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char viewblend_fragment_shader[] =
"layout(location=0) uniform vec4 Color;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	out_fragcolor = Color;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// View warp/scale
//
////////////////////////////////////////////////////////////////

static const char warpscale_vertex_shader[] =
"layout(location=0) out vec2 out_uv;\n"
"\n"
"void main()\n"
"{\n"
"	ivec2 v = ivec2(gl_VertexID & 1, gl_VertexID >> 1);\n"
"	out_uv = vec2(v) * 2.0;\n"
"	gl_Position = vec4(out_uv * 2.0 - 1.0, 0.0, 1.0);\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char warpscale_fragment_shader[] =
"layout(binding=0) uniform sampler2D Tex;\n"
"\n"
"layout(location=0) uniform vec4 UVScaleWarpTime; // xy=Scale z=Warp w=Time\n"
"layout(location=1) uniform vec4 BlendColor;\n"
"\n"
"layout(location=0) in vec2 in_uv;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	vec2 uv = in_uv;\n"
"	vec2 uv_scale = UVScaleWarpTime.xy;\n"
"\n"
"#if WARP\n"
"	float time = UVScaleWarpTime.w;\n"
"	float aspect = dFdy(uv.y) / dFdx(uv.x);\n"
"	vec2 warp_amp = UVScaleWarpTime.zz;\n"
"	warp_amp.y *= aspect;\n"
"	uv = warp_amp + uv * (1.0 - 2.0 * warp_amp); // remap to safe area\n"
"	uv += warp_amp * sin(vec2(uv.y / aspect, uv.x) * (3.14159265 * 8.0) + time);\n"
"#endif // WARP\n"
"\n"
"	out_fragcolor = texture(Tex, uv * uv_scale);\n"
"	out_fragcolor.rgb = mix(out_fragcolor.rgb, BlendColor.rgb, BlendColor.a);\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Postprocess (dithering, palettization, gamma/contrast)
//
////////////////////////////////////////////////////////////////

#define PALETTE_BUFFER \
"layout(std430, binding=0) restrict readonly buffer PaletteBuffer\n"\
"{\n"\
"	uint Palette[256];\n"\
"};\n"\
"\n"\
"uvec3 UnpackRGB8(uint c)\n"\
"{\n"\
"	return uvec3(c, c >> 8, c >> 16) & 255u;\n"\
"}\n"\
"\n"\

////////////////////////////////////////////////////////////////

#define NOISE_FUNCTIONS \
"// ALU-only 16x16 Bayer matrix\n"\
"float bayer01(ivec2 coord)\n"\
"{\n"\
"	coord &= 15;\n"\
"	coord.y ^= coord.x;\n"\
"	uint v = uint(coord.y | (coord.x << 8));	// 0  0  0  0 | x3 x2 x1 x0 |  0  0  0  0 | y3 y2 y1 y0\n"\
"	v = (v ^ (v << 2)) & 0x3333;				// 0  0 x3 x2 |  0  0 x1 x0 |  0  0 y3 y2 |  0  0 y1 y0\n"\
"	v = (v ^ (v << 1)) & 0x5555;				// 0 x3  0 x2 |  0 x1  0 x0 |  0 y3  0 y2 |  0 y1  0 y0\n"\
"	v |= v >> 7;								// 0 x3  0 x2 |  0 x1  0 x0 | x3 y3 x2 y2 | x1 y1 x0 y0\n"\
"	v = bitfieldReverse(v) >> 24;				// 0  0  0  0 |  0  0  0  0 | y0 x0 y1 x1 | y2 x2 y3 x3\n"\
"	return float(v) * (1.0/256.0);\n"\
"}\n"\
"\n"\
"float bayer(ivec2 coord)\n"\
"{\n"\
"	return bayer01(coord) - 0.5;\n"\
"}\n"\
"\n"\
"// Hash without Sine\n"\
"// https://www.shadertoy.com/view/4djSRW \n"\
/* Copyright (c)2014 David Hoskins.\
\
Permission is hereby granted, free of charge, to any person obtaining a copy\
of this software and associated documentation files (the "Software"), to deal\
in the Software without restriction, including without limitation the rights\
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\
copies of the Software, and to permit persons to whom the Software is\
furnished to do so, subject to the following conditions:\
\
The above copyright notice and this permission notice shall be included in all\
copies or substantial portions of the Software.\
\
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\
SOFTWARE.*/\
"float whitenoise01(vec2 p)\n"\
"{\n"\
"	vec3 p3 = fract(vec3(p.xyx) * .1031);\n"\
"	p3 += dot(p3, p3.yzx + 33.33);\n"\
"	return fract((p3.x + p3.y) * p3.z);\n"\
"}\n"\
"\n"\
"float whitenoise(vec2 p)\n"\
"{\n"\
"	return whitenoise01(p) - 0.5;\n"\
"}\n"\
"\n"\
"// Convert uniform distribution to triangle-shaped distribution\n"\
"// Input in [0..1], output in [-1..1]\n"\
"// Based on https://www.shadertoy.com/view/4t2SDh \n"\
"float tri(float x)\n"\
"{\n"\
"	float orig = x * 2.0 - 1.0;\n"\
"	uint signbit = floatBitsToUint(orig) & 0x80000000u;\n"\
"	x = sqrt(abs(orig)) - 1.;\n"\
"	x = uintBitsToFloat(floatBitsToUint(x) ^ signbit);\n"\
"	return x;\n"\
"}\n"\
"\n"\
"#define DITHER_NOISE(uv) tri(bayer01(ivec2(uv)))\n"\
"#define SCREEN_SPACE_NOISE() DITHER_NOISE(floor(gl_FragCoord.xy)+0.5)\n"\
"#define SUPPRESS_BANDING() bayer(ivec2(gl_FragCoord.xy))\n"\

////////////////////////////////////////////////////////////////

static const char postprocess_vertex_shader[] =
"void main()\n"
"{\n"
"	ivec2 v = ivec2(gl_VertexID & 1, gl_VertexID >> 1);\n"
"	gl_Position = vec4(vec2(v) * 4.0 - 1.0, 0.0, 1.0);\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char postprocess_fragment_shader[] =
"layout(binding=0) uniform sampler2D GammaTexture;\n"
"layout(binding=1) uniform usampler3D PaletteLUT;\n"
"\n"
PALETTE_BUFFER
NOISE_FUNCTIONS
"\n"
"layout(location=0) uniform vec4 Params;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	float gamma = Params.x;\n"
"	float contrast = Params.y;\n"
"	float scale = Params.z;\n"
"	float dither = Params.w;\n"
"	out_fragcolor = texelFetch(GammaTexture, ivec2(gl_FragCoord), 0);\n"
"#if PALETTIZE == 1\n"
"	vec2 noiseuv = floor(gl_FragCoord.xy * scale) + 0.5;\n"
"	out_fragcolor.rgb = sqrt(out_fragcolor.rgb);\n"
"	out_fragcolor.rgb += DITHER_NOISE(noiseuv) * dither;\n"
"	out_fragcolor.rgb *= out_fragcolor.rgb;\n"
"#endif // PALETTIZE == 1\n"
"#if PALETTIZE\n"
"	ivec3 clr = ivec3(clamp(out_fragcolor.rgb, 0., 1.) * 127. + 0.5);\n"
"	uint remap = Palette[texelFetch(PaletteLUT, clr, 0).x];\n"
"	out_fragcolor.rgb = vec3(UnpackRGB8(remap)) * (1./255.);\n"
"#else\n"
"	out_fragcolor.rgb *= contrast;\n"
"	out_fragcolor = vec4(pow(out_fragcolor.rgb, vec3(gamma)), 1.0);\n"
"#endif // PALETTIZE\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Common shader snippets
//
////////////////////////////////////////////////////////////////

#define FRAMEDATA_BUFFER \
"layout(std140, binding=0) uniform FrameDataUBO\n"\
"{\n"\
"	mat4	ViewProj;\n"\
"	vec4	Fog;\n"\
"	vec4	SkyFog;\n"\
"	vec3	WindDir;\n"\
"	float	WindPhase;\n"\
"	float	ScreenDither;\n"\
"	float	TextureDither;\n"\
"	vec3	EyePos;\n"\
"	float	Time;\n"\
"	float	ZLogScale;\n"\
"	float	ZLogBias;\n"\
"	uint	NumLights;\n"\
"};\n"\
"\n"\
"vec3 ApplyFog(vec3 clr, vec3 p)\n"\
"{\n"\
"	float fog = exp2(-Fog.w * dot(p, p));\n"\
"	fog = clamp(fog, 0.0, 1.0);\n"\
"	return mix(Fog.rgb, clr, fog);\n"\
"}\n"\
"\n"\

////////////////////////////////////////////////////////////////

#define LIGHT_BUFFER \
"#define LIGHT_TILES_X " QS_STRINGIFY (LIGHT_TILES_X) "\n"\
"#define LIGHT_TILES_Y " QS_STRINGIFY (LIGHT_TILES_Y) "\n"\
"#define LIGHT_TILES_Z " QS_STRINGIFY (LIGHT_TILES_Z) "\n"\
"#define MAX_LIGHTS    " QS_STRINGIFY (MAX_DLIGHTS)   "\n"\
"\n"\
"struct Light\n"\
"{\n"\
"	vec3	origin;\n"\
"	float	radius;\n"\
"	vec3	color;\n"\
"	float	minlight;\n"\
"};\n"\
"\n"\
"layout(std430, binding=0) restrict readonly buffer LightBuffer\n"\
"{\n"\
"	float	LightStyles[" QS_STRINGIFY (MAX_LIGHTSTYLES) "];\n"\
"	Light	Lights[];\n"\
"};\n"\
"\n"\
"float GetLightStyle(int index)\n"\
"{\n"\
"	float result;\n"\
"	if (index < " QS_STRINGIFY (MAX_LIGHTSTYLES) ")\n"\
"		result = LightStyles[index];\n"\
"	else\n"\
"		result = 1.0;\n"\
"	return result;\n"\
"}\n"\
"\n"\

////////////////////////////////////////////////////////////////

#define LIGHT_CLUSTER_IMAGE(mode) \
"layout(rg32ui, binding=0) uniform " mode " uimage3D LightClusters;\n"\

////////////////////////////////////////////////////////////////

#define DRAW_ELEMENTS_INDIRECT_COMMAND \
"struct DrawElementsIndirectCommand\n"\
"{\n"\
"	uint	count;\n"\
"	uint	instanceCount;\n"\
"	uint	firstIndex;\n"\
"	uint	baseVertex;\n"\
"	uint	baseInstance;\n"\
"};\n"\

////////////////////////////////////////////////////////////////

#define WORLD_DRAW_BUFFER \
DRAW_ELEMENTS_INDIRECT_COMMAND \
"\n"\
"layout(std430, binding=1) buffer DrawIndirectBuffer\n"\
"{\n"\
"	DrawElementsIndirectCommand cmds[];\n"\
"};\n"\

////////////////////////////////////////////////////////////////

#define WORLD_CALLDATA_BUFFER \
"struct Call\n"\
"{\n"\
"	uint	flags;\n"\
"	float	wateralpha;\n"\
"#if BINDLESS\n"\
"	uvec2	txhandle;\n"\
"	uvec2	fbhandle;\n"\
"#else\n"\
"	int		baseinstance;\n"\
"	int		padding;\n"\
"#endif // BINDLESS\n"\
"};\n"\
"const uint\n"\
"	CF_USE_POLYGON_OFFSET = 1u,\n"\
"	CF_USE_FULLBRIGHT = 2u,\n"\
"	CF_NOLIGHTMAP = 4u\n"\
";\n"\
"\n"\
"layout(std430, binding=1) restrict readonly buffer CallBuffer\n"\
"{\n"\
"	Call call_data[];\n"\
"};\n"\
"\n"\
"#if BINDLESS\n"\
"	#define GET_INSTANCE_ID(call) (gl_BaseInstanceARB + gl_InstanceID)\n"\
"#else\n"\
"	#define GET_INSTANCE_ID(call) (call.baseinstance + gl_InstanceID)\n"\
"#endif\n"\

////////////////////////////////////////////////////////////////

#define WORLD_INSTANCEDATA_BUFFER \
"struct Instance\n"\
"{\n"\
"	vec4	mat[3];\n"\
"	float	alpha;\n"\
"};\n"\
"\n"\
"layout(std430, binding=2) restrict readonly buffer InstanceBuffer\n"\
"{\n"\
"	Instance instance_data[];\n"\
"};\n"\
"\n"\
"vec3 Transform(vec3 p, Instance instance)\n"\
"{\n"\
"	mat4x3 world = transpose(mat3x4(instance.mat[0], instance.mat[1], instance.mat[2]));\n"\
"	return mat3(world[0], world[1], world[2]) * p + world[3];\n"\
"}\n"\
"\n"\

////////////////////////////////////////////////////////////////

#define WORLD_VERTEX_BUFFER \
"layout(location=0) in vec3 in_pos;\n"\
"layout(location=1) in vec4 in_uv;\n"\
"layout(location=2) in float in_lmofs;\n"\
"layout(location=3) in ivec4 in_styles;\n"\
"\n"\

////////////////////////////////////////////////////////////////

#define BINDLESS_VERTEX_HEADER \
"#if BINDLESS\n"\
"	#extension GL_ARB_shader_draw_parameters : require\n"\
"	#define DRAW_ID			gl_DrawIDARB\n"\
"#else\n"\
"	layout(location=0) uniform int DrawID;\n"\
"	#define DRAW_ID			DrawID\n"\
"#endif\n"\
"\n"\

////////////////////////////////////////////////////////////////

#define OIT_OUTPUT(output_name) \
"#define OUT_COLOR " QS_STRINGIFY (output_name) "\n"\
"#if OIT\n"\
"	vec4 OUT_COLOR;\n"\
"	layout(location=0) out vec4 out_accum;\n"\
"	layout(location=1) out float out_reveal;\n"\
"\n"\
"	vec3 GammaToLinear(vec3 v)\n"\
"	{\n"\
"#if " QS_STRINGIFY (LINEAR_SPACE_OIT) "\n"\
"		return v*v;\n"\
"#else\n"\
"		return v;\n"\
"#endif\n"\
"	}\n"\
"\n"\
"	void main_body();\n"\
"\n"\
"	void main()\n"\
"	{\n"\
"		main_body();\n"\
"		OUT_COLOR = clamp(OUT_COLOR, 0.0, 1.0);\n"\
"		vec4 color = vec4(GammaToLinear(OUT_COLOR.rgb), OUT_COLOR.a);\n"\
"		float z = 1./gl_FragCoord.w;\n"\
"#if " QS_STRINGIFY (LINEAR_SPACE_OIT) "\n"\
"		float weight = clamp(color.a * color.a * 0.03 / (1e-5 + pow(z/2e5, 2.0)), 1e-2, 3e3);\n"\
"#else\n"\
"		float weight = clamp(color.a * color.a * 0.03 / (1e-5 + pow(z/1e7, 1.0)), 1e-2, 3e3);\n"\
"#endif\n"\
"		out_accum = vec4(color.rgb, color.a * weight);\n"\
"		out_accum.rgb *= out_accum.a;\n"\
"		out_reveal = color.a;\n"\
"	}\n"\
"\n"\
"	#define main main_body\n"\
"#else\n"\
"	layout(location=0) out vec4 OUT_COLOR;\n"\
"#endif // OIT\n"\

////////////////////////////////////////////////////////////////
//
// World
//
////////////////////////////////////////////////////////////////

static const char world_vertex_shader[] =
BINDLESS_VERTEX_HEADER
FRAMEDATA_BUFFER
LIGHT_BUFFER
WORLD_CALLDATA_BUFFER
WORLD_INSTANCEDATA_BUFFER
WORLD_VERTEX_BUFFER
"\n"
"layout(location=0) flat out uint out_flags;\n"
"layout(location=1) flat out float out_alpha;\n"
"layout(location=2) out vec3 out_pos;\n"
"#if MODE == " QS_STRINGIFY (WORLDSHADER_ALPHATEST) "\n"
"	layout(location=3) centroid out vec2 out_uv;\n"
"#else\n"
"	layout(location=3) out vec2 out_uv;\n"
"#endif\n"
"layout(location=4) centroid out vec2 out_lmuv;\n"
"layout(location=5) out float out_depth;\n"
"layout(location=6) noperspective out vec2 out_coord;\n"
"layout(location=7) flat out vec4 out_styles;\n"
"layout(location=8) flat out float out_lmofs;\n"
"#if BINDLESS\n"
"	layout(location=9) flat out uvec4 out_samplers;\n"
"#endif\n"
"\n"
"void main()\n"
"{\n"
"	Call call = call_data[DRAW_ID];\n"
"	int instance_id = GET_INSTANCE_ID(call);\n"
"	Instance instance = instance_data[instance_id];\n"
"	out_pos = Transform(in_pos, instance);\n"
"	gl_Position = ViewProj * vec4(out_pos, 1.0);\n"
"#if REVERSED_Z\n"
"	const float ZBIAS = -1./1024;\n"
"#else\n"
"	const float ZBIAS =  1./1024;\n"
"#endif\n"
"	if ((call.flags & CF_USE_POLYGON_OFFSET) != 0u)\n"
"		gl_Position.z += ZBIAS;\n"
"	out_uv = in_uv.xy;\n"
"	out_lmuv = in_uv.zw;\n"
"	out_depth = gl_Position.w;\n"
"	out_coord = (gl_Position.xy / gl_Position.w * 0.5 + 0.5) * vec2(LIGHT_TILES_X, LIGHT_TILES_Y);\n"
"	out_flags = call.flags;\n"
"#if MODE == " QS_STRINGIFY (WORLDSHADER_WATER) "\n"
"	out_alpha = instance.alpha < 0.0 ? call.wateralpha : instance.alpha;\n"
"#else\n"
"	out_alpha = instance.alpha < 0.0 ? 1.0 : instance.alpha;\n"
"#endif\n"
"	out_styles.x = GetLightStyle(in_styles.x);\n"
"	if (in_styles.y == 255)\n"
"		out_styles.yzw = vec3(-1.);\n"
"	else if (in_styles.z == 255)\n"
"		out_styles.yzw = vec3(GetLightStyle(in_styles.y), -1., -1.);\n"
"	else\n"
"		out_styles.yzw = vec3\n"
"		(\n"
"			GetLightStyle(in_styles.y),\n"
"			GetLightStyle(in_styles.z),\n"
"			GetLightStyle(in_styles.w)\n"
"		);\n"
"	if ((call.flags & CF_NOLIGHTMAP) != 0u)\n"
"		out_styles.xy = vec2(1., -1.);\n"
"	out_lmofs = in_lmofs;\n"
"#if BINDLESS\n"
"	out_samplers.xy = call.txhandle;\n"
"	if ((call.flags & CF_USE_FULLBRIGHT) != 0u)\n"
"		out_samplers.zw = call.fbhandle;\n"
"	else\n"
"		out_samplers.zw = out_samplers.xy;\n"
"#endif\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char world_fragment_shader[] =
"#if BINDLESS\n"
"	#extension GL_ARB_bindless_texture : require\n"
"#else\n"
"	layout(binding=0) uniform sampler2D Tex;\n"
"	layout(binding=1) uniform sampler2D FullbrightTex;\n"
"#endif\n"
"layout(binding=2) uniform sampler2D LMTex;\n"
"\n"
FRAMEDATA_BUFFER
LIGHT_BUFFER
LIGHT_CLUSTER_IMAGE("readonly")
WORLD_CALLDATA_BUFFER
WORLD_INSTANCEDATA_BUFFER
NOISE_FUNCTIONS
"\n"
"layout(location=0) flat in uint in_flags;\n"
"layout(location=1) flat in float in_alpha;\n"
"layout(location=2) in vec3 in_pos;\n"
"#if MODE == " QS_STRINGIFY (WORLDSHADER_ALPHATEST) "\n"
"	layout(location=3) centroid in vec2 in_uv;\n"
"#else\n"
"	layout(location=3) in vec2 in_uv;\n"
"#endif\n"
"layout(location=4) centroid in vec2 in_lmuv;\n"
"layout(location=5) in float in_depth;\n"
"layout(location=6) noperspective in vec2 in_coord;\n"
"layout(location=7) flat in vec4 in_styles;\n"
"layout(location=8) flat in float in_lmofs;\n"
"#if BINDLESS\n"
"	layout(location=9) flat in uvec4 in_samplers;\n"
"#endif\n"
"\n"
OIT_OUTPUT (out_fragcolor)
"\n"
"void main()\n"
"{\n"
"#if " QS_STRINGIFY (SHOW_WORLD_NORMALS) "\n"
"	out_fragcolor = vec4(0.5 + 0.5 * normalize(cross(dFdx(in_pos), dFdy(in_pos))), 0.75);\n"
"	return;\n"
"#endif\n"
"	vec3 fullbright = vec3(0.);\n"
"	vec2 uv = in_uv;\n"
"#if MODE == " QS_STRINGIFY (WORLDSHADER_WATER) "\n"
"	uv = uv * 2.0 + 0.125 * sin(uv.yx * (3.14159265 * 2.0) + Time);\n"
"#endif\n"
"#if BINDLESS\n"
"	sampler2D Tex = sampler2D(in_samplers.xy);\n"
"	sampler2D FullbrightTex;\n"
"	if ((in_flags & CF_USE_FULLBRIGHT) != 0u)\n"
"	{\n"
"		FullbrightTex = sampler2D(in_samplers.zw);\n"
"		fullbright = texture(FullbrightTex, uv).rgb;\n"
"	}\n"
"#else\n"
"	if ((in_flags & CF_USE_FULLBRIGHT) != 0u)\n"
"		fullbright = texture(FullbrightTex, uv).rgb;\n"
"#endif\n"
"#if DITHER >= 2\n"
"	vec4 result = texture(Tex, uv, -1.0);\n"
"#elif DITHER\n"
"	vec4 result = texture(Tex, uv, -0.5);\n"
"#else\n"
"	vec4 result = texture(Tex, uv);\n"
"#endif\n"
"#if MODE == " QS_STRINGIFY (WORLDSHADER_ALPHATEST) "\n"
"	// Note: for alpha-tested surfaces we need to compute the plane equation before discard is called,\n"
"	// otherwise we would get incorrect results for edge pixels due to invalid derivatives.\n"
"	vec4 plane;\n"
"	plane.xyz = normalize(cross(dFdx(in_pos), dFdy(in_pos)));\n"
"	plane.w = dot(in_pos, plane.xyz);\n"
"	if (result.a < 0.666)\n"
"		discard;\n"
"#endif\n"
"\n"
"	vec2 lmuv = in_lmuv;\n"
"#if DITHER\n"
"	vec2 lmsize = vec2(textureSize(LMTex, 0).xy) * 16.;\n"
"	lmuv = (floor(lmuv * lmsize) + 0.5) / lmsize;\n"
"#endif // DITHER\n"
"	vec4 lm0 = textureLod(LMTex, lmuv, 0.);\n"
"	vec3 total_light;\n"
"	if (in_styles.y < 0.) // single style fast path\n"
"		total_light = in_styles.x * lm0.xyz;\n"
"	else\n"
"	{\n"
"		vec4 lm1 = textureLod(LMTex, vec2(lmuv.x + in_lmofs, lmuv.y), 0.);\n"
"		if (in_styles.z < 0.) // 2 styles\n"
"		{\n"
"			total_light =\n"
"				in_styles.x * lm0.xyz +\n"
"				in_styles.y * lm1.xyz;\n"
"		}\n"
"		else // 3 or 4 lightstyles\n"
"		{\n"
"			vec4 lm2 = textureLod(LMTex, vec2(lmuv.x + in_lmofs * 2., lmuv.y), 0.);\n"
"			total_light = vec3\n"
"			(\n"
"				dot(in_styles, lm0),\n"
"				dot(in_styles, lm1),\n"
"				dot(in_styles, lm2)\n"
"			);\n"
"		}\n"
"	}\n"
"\n"
"	if (NumLights > 0u)\n"
"	{\n"
"		uint i, ofs;\n"
"		ivec3 cluster_coord;\n"
"		cluster_coord.x = int(floor(in_coord.x));\n"
"		cluster_coord.y = int(floor(in_coord.y));\n"
"		cluster_coord.z = int(floor(log2(in_depth) * ZLogScale + ZLogBias));\n"
"		uvec2 clusterdata = imageLoad(LightClusters, cluster_coord).xy;\n"
"		if ((clusterdata.x | clusterdata.y) != 0u)\n"
"		{\n"
"#if " QS_STRINGIFY (SHOW_ACTIVE_LIGHT_CLUSTERS) "\n"
"			int cluster_idx = cluster_coord.x + cluster_coord.y * LIGHT_TILES_X + cluster_coord.z * LIGHT_TILES_X * LIGHT_TILES_Y;\n"
"			total_light = vec3(ivec3((cluster_idx + 1) * 0x45d9f3b) >> ivec3(0, 8, 16) & 255) / 255.0;\n"
"#endif // SHOW_ACTIVE_LIGHT_CLUSTERS\n"
"#if MODE != " QS_STRINGIFY (WORLDSHADER_ALPHATEST) "\n"
"			// For non-alpha-tested geometry it is safe to defer the plane equation computation until it is actually needed\n"
"			vec4 plane;\n"
"			plane.xyz = normalize(cross(dFdx(in_pos), dFdy(in_pos)));\n"
"			plane.w = dot(in_pos, plane.xyz);\n"
"#endif\n"
"			vec3 dynamic_light = vec3(0.);\n"
"			for (i = 0u, ofs = 0u; i < 2u; i++, ofs += 32u)\n"
"			{\n"
"				uint mask = clusterdata[i];\n"
"				while (mask != 0u)\n"
"				{\n"
"					int j = findLSB(mask);\n"
"					mask ^= 1u << j;\n"
"					Light l = Lights[ofs + j];\n"
"					// mimics R_AddDynamicLights, up to a point\n"
"					float rad = l.radius;\n"
"					float dist = dot(l.origin, plane.xyz) - plane.w;\n"
"					rad -= abs(dist);\n"
"					float minlight = l.minlight;\n"
"					if (rad < minlight)\n"
"						continue;\n"
"					vec3 local_pos = l.origin - plane.xyz * dist;\n"
"					minlight = rad - minlight;\n"
"					dist = length(in_pos - local_pos);\n"
"					dynamic_light += clamp((minlight - dist) / 16.0, 0.0, 1.0) * max(0., rad - dist) / 256. * l.color;\n"
"				}\n"
"			}\n"
"			total_light += max(min(dynamic_light, 1. - total_light), 0.);\n"
"		}\n"
"	}\n"
"#if DITHER >= 2\n"
"	total_light = floor(total_light * 63. + 0.5) * (2./63.);\n"
"#else\n"
"	total_light *= 2.0;\n"
"#endif\n"
"#if MODE != " QS_STRINGIFY (WORLDSHADER_ALPHATEST) "\n"
"	result.rgb = mix(result.rgb, result.rgb * total_light, result.a);\n"
"#else\n"
"	result.rgb *= total_light;\n"
"#endif\n"
"	result.rgb += fullbright;\n"
"	result = clamp(result, 0.0, 1.0);\n"
"	result.rgb = ApplyFog(result.rgb, in_pos - EyePos);\n"
"\n"
"	result.a = in_alpha; // FIXME: This will make almost transparent things cut holes though heavy fog\n"
"	out_fragcolor = result;\n"
"#if DITHER == 1\n"
"	vec3 dpos = fwidth(in_pos);\n"
"	float farblend = clamp(max(dpos.x, max(dpos.y, dpos.z)) * 0.5 - 0.125, 0., 1.);\n"
"	farblend *= farblend;\n"
"	out_fragcolor.rgb = sqrt(out_fragcolor.rgb);\n"
"	float luma = dot(out_fragcolor.rgb, vec3(.25, .625, .125));\n"
"	float nearnoise = tri(whitenoise01(lmuv * lmsize)) * luma * TextureDither;\n"
"	float farnoise = Fog.w > 0. ? SCREEN_SPACE_NOISE() * ScreenDither : 0.;\n"
"	out_fragcolor.rgb += mix(nearnoise, farnoise, farblend);\n"
"	out_fragcolor.rgb *= out_fragcolor.rgb;\n"
"#endif // DITHER == 1\n"
"#if DITHER >= 2\n"
"	// nuke extra precision in 10-bit framebuffer\n"
"	out_fragcolor.rgb = floor(out_fragcolor.rgb * 255. + 0.5) * (1./255.);\n"
"#elif DITHER == 0\n"
"	out_fragcolor.rgb += SUPPRESS_BANDING() * ScreenDither;\n"
"#endif\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Water
//
////////////////////////////////////////////////////////////////

static const char water_vertex_shader[] =
BINDLESS_VERTEX_HEADER
FRAMEDATA_BUFFER
WORLD_CALLDATA_BUFFER
WORLD_INSTANCEDATA_BUFFER
WORLD_VERTEX_BUFFER
"\n"
"layout(location=0) flat out float out_alpha;"
"layout(location=1) out vec2 out_uv;\n"
"layout(location=2) out vec3 out_pos;\n"
"#if BINDLESS\n"
"	layout(location=3) flat out uvec2 out_sampler;\n"
"#endif\n"
"\n"
"void main()\n"
"{\n"
"	Call call = call_data[DRAW_ID];\n"
"	int instance_id = GET_INSTANCE_ID(call);\n"
"	Instance instance = instance_data[instance_id];\n"
"	vec3 pos = Transform(in_pos, instance);\n"
"	gl_Position = ViewProj * vec4(pos, 1.0);\n"
"	out_uv = in_uv.xy;\n"
"	out_pos = pos - EyePos;\n"
"	out_alpha = instance.alpha < 0.0 ? call.wateralpha : instance.alpha;\n"
"#if BINDLESS\n"
"	out_sampler = call.txhandle;\n"
"#endif\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char water_fragment_shader[] =
"#if BINDLESS\n"
"	#extension GL_ARB_bindless_texture : require\n"
"#else\n"
"	layout(binding=0) uniform sampler2D Tex;\n"
"#endif\n"
"\n"
FRAMEDATA_BUFFER
NOISE_FUNCTIONS
"\n"
"layout(location=0) flat in float in_alpha;\n"
"layout(location=1) in vec2 in_uv;\n"
"layout(location=2) in vec3 in_pos;\n"
"#if BINDLESS\n"
"	layout(location=3) flat in uvec2 in_sampler;\n"
"#endif\n"
"\n"
OIT_OUTPUT (out_fragcolor)
"\n"
"void main()\n"
"{\n"
"	vec2 uv = in_uv * 2.0 + 0.125 * sin(in_uv.yx * (3.14159265 * 2.0) + Time);\n"
"#if BINDLESS\n"
"	sampler2D Tex = sampler2D(in_sampler);\n"
"#endif\n"
"	vec4 result = texture(Tex, uv);\n"
"	result.rgb = ApplyFog(result.rgb, in_pos);\n"
"	result.a *= in_alpha;\n"
"	out_fragcolor = result;\n"
"#if DITHER\n"
"	if (Fog.w > 0.)\n"
"	{\n"
"		out_fragcolor.rgb = sqrt(out_fragcolor.rgb);\n"
"		out_fragcolor.rgb += SCREEN_SPACE_NOISE() * ScreenDither;\n"
"		out_fragcolor.rgb *= out_fragcolor.rgb;\n"
"	}\n"
"#else\n"
"	out_fragcolor.rgb += SUPPRESS_BANDING() * ScreenDither;\n"
"#endif\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Sky stencil mark
//
////////////////////////////////////////////////////////////////

static const char skystencil_vertex_shader[] =
BINDLESS_VERTEX_HEADER
FRAMEDATA_BUFFER
WORLD_CALLDATA_BUFFER
WORLD_INSTANCEDATA_BUFFER
WORLD_VERTEX_BUFFER
"\n"
"void main()\n"
"{\n"
"	Call call = call_data[DRAW_ID];\n"
"	int instance_id = GET_INSTANCE_ID(call);\n"
"	Instance instance = instance_data[instance_id];\n"
"	gl_Position = ViewProj * vec4(Transform(in_pos, instance), 1.0);\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Sky layers
//
////////////////////////////////////////////////////////////////

static const char sky_layers_vertex_shader[] =
BINDLESS_VERTEX_HEADER
FRAMEDATA_BUFFER
WORLD_CALLDATA_BUFFER
WORLD_INSTANCEDATA_BUFFER
WORLD_VERTEX_BUFFER
"\n"
"layout(location=0) out vec3 out_dir;\n"
"#if BINDLESS\n"
"	layout(location=1) flat out uvec4 out_samplers;\n"
"#endif\n"
"\n"
"void main()\n"
"{\n"
"	Call call = call_data[DRAW_ID];\n"
"	int instance_id = GET_INSTANCE_ID(call);\n"
"	Instance instance = instance_data[instance_id];\n"
"	vec3 pos = Transform(in_pos, instance);\n"
"	gl_Position = ViewProj * vec4(pos, 1.0);\n"
"	out_dir = pos - EyePos;\n"
"	out_dir.z *= 3.0; // flatten the sphere\n"
"#if BINDLESS\n"
"	out_samplers.xy = call.txhandle;\n"
"	out_samplers.zw = call.fbhandle;\n"
"#endif\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char sky_layers_fragment_shader[] =
"#if BINDLESS\n"
"	#extension GL_ARB_bindless_texture : require\n"
"#else\n"
"	layout(binding=0) uniform sampler2D SolidLayer;\n"
"	layout(binding=1) uniform sampler2D AlphaLayer;\n"
"#endif\n"
"\n"
FRAMEDATA_BUFFER
NOISE_FUNCTIONS
"\n"
"layout(location=0) in vec3 in_dir;\n"
"#if BINDLESS\n"
"	layout(location=1) flat in uvec4 in_samplers;\n"
"#endif\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"#if BINDLESS\n"
"	sampler2D SolidLayer = sampler2D(in_samplers.xy);\n"
"	sampler2D AlphaLayer = sampler2D(in_samplers.zw);\n"
"#endif\n"
"	vec2 uv = normalize(in_dir).xy * (189.0 / 64.0);\n"
"	vec4 result = texture(SolidLayer, uv + Time / 16.0);\n"
"	vec4 layer = texture(AlphaLayer, uv + Time / 8.0);\n"
"	result.rgb = mix(result.rgb, layer.rgb, layer.a);\n"
"	result.rgb = mix(result.rgb, SkyFog.rgb, SkyFog.a);\n"
"	out_fragcolor = result;\n"
"	out_fragcolor.rgb += SUPPRESS_BANDING() * ScreenDither;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Skybox cubemap
//
////////////////////////////////////////////////////////////////

static const char sky_cubemap_vertex_shader[] =
BINDLESS_VERTEX_HEADER
FRAMEDATA_BUFFER
WORLD_CALLDATA_BUFFER
WORLD_INSTANCEDATA_BUFFER
WORLD_VERTEX_BUFFER
"\n"
"layout(location=0) out vec3 out_dir;\n"
"\n"
"void main()\n"
"{\n"
"	Call call = call_data[DRAW_ID];\n"
"	int instance_id = GET_INSTANCE_ID(call);\n"
"	Instance instance = instance_data[instance_id];\n"
"	vec3 pos = Transform(in_pos, instance);\n"
"	gl_Position = ViewProj * vec4(pos, 1.0);\n"
"	out_dir.x = -(pos.y - EyePos.y);\n"
"	out_dir.y =  (pos.z - EyePos.z);\n"
"	out_dir.z =  (pos.x - EyePos.x);\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char sky_cubemap_fragment_shader[] =
FRAMEDATA_BUFFER
NOISE_FUNCTIONS
"\n"
"layout(binding=2) uniform samplerCube Skybox;\n"
"\n"
"layout(location=0) in vec3 in_dir;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"#if ANIM\n"
"	float t1 = WindPhase;\n"
"	float t2 = fract(t1) - 0.5;\n"
"	float blend = abs(t1 * 2.0);\n"
"	vec3 dir = normalize(in_dir);\n"
"	vec4 base = texture(Skybox, in_dir);\n"
"	vec4 layer1 = texture(Skybox, dir + t1 * WindDir);\n"
"	vec4 layer2 = texture(Skybox, dir + t2 * WindDir);\n"
"	layer1.a *= 1.0 - blend;\n"
"	layer2.a *= blend;\n"
"	layer1.rgb *= layer1.a;\n"
"	layer2.rgb *= layer2.a;\n"
"	vec4 combined = layer1 + layer2;\n"
"	out_fragcolor = vec4(base.rgb * (1.0 - combined.a) + combined.rgb, 1);\n"
"#else\n"
"	out_fragcolor = texture(Skybox, in_dir);\n"
"#endif\n"
"	out_fragcolor.rgb = mix(out_fragcolor.rgb, SkyFog.rgb, SkyFog.a);\n"
"#if DITHER\n"
"	out_fragcolor.rgb = sqrt(out_fragcolor.rgb);\n"
"	out_fragcolor.rgb += SCREEN_SPACE_NOISE() * ScreenDither;\n"
"	out_fragcolor.rgb *= out_fragcolor.rgb;\n"
"#else\n"
"	out_fragcolor.rgb += SUPPRESS_BANDING() * ScreenDither;\n"
"#endif\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Skybox side
//
////////////////////////////////////////////////////////////////

static const char sky_boxside_vertex_shader[] =
"layout(location=0) uniform mat4 MVP;\n"
"layout(location=1) uniform vec3 EyePos;\n"
"\n"
"layout(location=0) in vec3 in_dir;\n"
"layout(location=1) in vec2 in_uv;\n"
"\n"
"layout(location=0) out vec3 out_dir;\n"
"layout(location=1) out vec2 out_uv;\n"
"\n"
"void main()\n"
"{\n"
"	gl_Position = MVP * vec4(EyePos + in_dir, 1.0);\n"
"	gl_Position.z = gl_Position.w; // map to far plane\n"
"	out_dir = in_dir;\n"
"	out_uv = in_uv;\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char sky_boxside_fragment_shader[] =
"layout(binding=0) uniform sampler2D Tex;\n"
"\n"
NOISE_FUNCTIONS
"\n"
"layout(location=2) uniform vec4 Fog;\n"
"layout(location=3) uniform float ScreenDither;\n"
"\n"
"layout(location=0) in vec3 in_dir;\n"
"layout(location=1) in vec2 in_uv;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	out_fragcolor = texture(Tex, in_uv);\n"
"	out_fragcolor.rgb = mix(out_fragcolor.rgb, Fog.rgb, Fog.w);\n"
"#if DITHER\n"
"	out_fragcolor.rgb = sqrt(out_fragcolor.rgb);\n"
"	out_fragcolor.rgb += SCREEN_SPACE_NOISE() * ScreenDither;\n"
"	out_fragcolor.rgb *= out_fragcolor.rgb;\n"
"#else\n"
"	out_fragcolor.rgb += SUPPRESS_BANDING() * ScreenDither;\n"
"#endif\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Alias models
//
////////////////////////////////////////////////////////////////

#define ALIAS_INSTANCE_BUFFER \
"struct InstanceData\n"\
"{\n"\
"	vec4	WorldMatrix[3];\n"\
"	vec4	LightColor; // xyz=LightColor w=Alpha\n"\
"	int		Pose1;\n"\
"	int		Pose2;\n"\
"	float	Blend;\n"\
"	int		Padding;\n"\
"};\n"\
"\n"\
"layout(std430, binding=1) restrict readonly buffer InstanceBuffer\n"\
"{\n"\
"	mat4	ViewProj;\n"\
"	vec3	EyePos;\n"\
"	vec4	Fog;\n"\
"	float	ScreenDither;\n"\
"	InstanceData instances[];\n"\
"};\n"\

////////////////////////////////////////////////////////////////

static const char alias_vertex_shader[] =
ALIAS_INSTANCE_BUFFER
"\n"
"struct PoseVertex\n"
"{\n"
"	vec3 pos;\n"
"	vec3 nor;\n"
"};\n"
"\n"
"#if POSEVERTTYPE == 1 // PV_IQM (Skeletal)\n"
"	layout(location=0) in vec3 in_pos;\n"
"	layout(location=1) in vec4 in_nor;\n"
"	layout(location=2) in vec2 in_uv;\n"
"	layout(location=3) in vec4 in_weights;\n"
"	layout(location=4) in ivec4 in_indices;\n"
"\n"
"	layout(std430, binding=2) restrict readonly buffer PoseBuffer\n"
"	{\n"
"		mat3x4 BonePoses[];\n"
"	};\n"
"\n"
"	PoseVertex GetPoseVertex(uint pose)\n"
"	{\n"
"		mat3x4 blendmat = BonePoses[pose + in_indices.x] * in_weights.x;\n"
"		blendmat += BonePoses[pose + in_indices.y] * in_weights.y;\n"
"		if (in_weights.z + in_weights.w > 0.0)\n"
"		{\n"
"			blendmat += BonePoses[pose + in_indices.z] * in_weights.z;\n"
"			blendmat += BonePoses[pose + in_indices.w] * in_weights.w;\n"
"		}\n"
"		mat4x3 anim = transpose(blendmat);\n"
"		return PoseVertex((anim * vec4(in_pos, 1.0)).xyz, (anim * vec4(in_nor.xyz, 0.0)).xyz);\n"
"	}\n"
"\n"
"#else // PV_QUAKE1 || PV_MD3\n"
"	layout (location = 0) in vec2 in_uv;\n"
"\n"
"	layout(std430, binding=2) restrict readonly buffer BlendShapeBuffer\n"
"	{\n"
"		uvec2 PackedPosNor[];\n"
"	};\n"
"\n"
"	PoseVertex GetPoseVertex (uint pose)\n"
"	{\n"
"		uvec2 data = PackedPosNor[pose + gl_VertexID];\n"
"#if POSEVERTTYPE == 2 // PV_MD3\n"
"		PoseVertex ret;\n"
"		ret.pos = vec3((ivec3(data.xxy >> uvec3(0, 16, 0)) & 65535) - 32768);\n"
"		vec2 spherical = vec2((data.yy >> uvec2(16, 24)) & 255) * (2.0 * 3.14159265 / 255.0);\n"
"		float sinlat = sin(spherical.x);\n"
"		float coslat = cos(spherical.x);\n"
"		float sinlng = sin(spherical.y);\n"
"		float coslng = cos(spherical.y);\n"
"		ret.nor.x = coslng * sinlat;\n"
"		ret.nor.y = sinlng * sinlat;\n"
"		ret.nor.z = coslat;\n"
"		return ret;\n"
"#else // PV_QUAKE1\n"
"		return PoseVertex(vec3((data.xxx >> uvec3(0, 8, 16)) & 255), unpackSnorm4x8(data.y).xyz);\n"
"#endif // POSEVERTTYPE\n"
"	}\n"
"\n"
"#endif // POSEVERTTYPE check\n"
"\n"
"float r_avertexnormal_dot(vec3 vertexnormal, vec3 dir) // from MH \n"
"{\n"
"	float d = dot(vertexnormal, dir);\n"
"	if (d < 0.0)\n"
"		return 1.0 + d * (13.0 / 44.0);\n"
"	else\n"
"		return 1.0 + d;\n"
"}\n"
"\n"
"#if MODE == " QS_STRINGIFY (ALIASSHADER_NOPERSP) "\n"
"	layout(location=0) noperspective out vec2 out_texcoord;\n"
"#else\n"
"	layout(location=0) out vec2 out_texcoord;\n"
"#endif\n"
"layout(location=1) out vec4 out_color;\n"
"layout(location=2) out vec3 out_pos;\n"
"\n"
"void main()\n"
"{\n"
"	InstanceData inst = instances[gl_InstanceID];\n"
"	out_texcoord = in_uv;\n"
"	PoseVertex pose1 = GetPoseVertex(inst.Pose1);\n"
"	PoseVertex pose2 = GetPoseVertex(inst.Pose2);\n"
"	mat4x3 worldmatrix = transpose(mat3x4(inst.WorldMatrix[0], inst.WorldMatrix[1], inst.WorldMatrix[2]));\n"
"	vec3 lerpedVert = (worldmatrix * vec4(mix(pose1.pos, pose2.pos, inst.Blend), 1.0)).xyz;\n"
"	gl_Position = ViewProj * vec4(lerpedVert, 1.0);\n"
"	out_pos = lerpedVert - EyePos;\n"
"	// transform world X and Z axes to local space\n"
"	mat3 orientation = mat3(normalize(worldmatrix[0].xyz), normalize(worldmatrix[1].xyz), normalize(worldmatrix[2].xyz));\n"
"	orientation = transpose(orientation);\n"
"	vec3 shadevector = (orientation[0] + orientation[2]) / sqrt(2.0);\n"
"	float dot1 = r_avertexnormal_dot(pose1.nor, shadevector);\n"
"	float dot2 = r_avertexnormal_dot(pose2.nor, shadevector);\n"
"	out_color = clamp(inst.LightColor * vec4(vec3(mix(dot1, dot2, inst.Blend)), 1.0), 0.0, 1.0);\n"
"	uint overbright = floatBitsToUint(Fog.w) >> 31;\n"
"	out_color.rgb = ldexp(out_color.rgb, ivec3(overbright));\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char alias_fragment_shader[] =
ALIAS_INSTANCE_BUFFER
NOISE_FUNCTIONS
"\n"
"layout(binding=0) uniform sampler2D Tex;\n"
"layout(binding=1) uniform sampler2D FullbrightTex;\n"
"\n"
"#if MODE == " QS_STRINGIFY (ALIASSHADER_NOPERSP) "\n"
"	layout(location=0) noperspective in vec2 in_texcoord;\n"
"#else\n"
"	layout(location=0) in vec2 in_texcoord;\n"
"#endif\n"
"layout(location=1) in vec4 in_color;\n"
"layout(location=2) in vec3 in_pos;\n"
"\n"
OIT_OUTPUT (out_fragcolor)
"\n"
"void main()\n"
"{\n"
"	vec2 uv = in_texcoord;\n"
"#if MODE == " QS_STRINGIFY (ALIASSHADER_NOPERSP) "\n"
"	uv -= 0.5 / vec2(textureSize(Tex, 0).xy);\n"
"	vec4 result = textureLod(Tex, uv, 0.);\n"
"#else\n"
"	vec4 result = texture(Tex, uv);\n"
"#endif\n"
"#if ALPHATEST\n"
"	if (result.a < 0.666)\n"
"		discard;\n"
"	result.rgb *= in_color.rgb;\n"
"#else\n"
"	result.rgb = mix(result.rgb, result.rgb * in_color.rgb, result.a);\n"
"#endif\n"
"#if POSEVERTTYPE == 2 \n"
"	result.a *= in_color.a;\n"
"#else\n"
"	result.a = in_color.a;\n"
"#endif\n"
"#if MODE == " QS_STRINGIFY (ALIASSHADER_NOPERSP) "\n"
"	result.rgb += textureLod(FullbrightTex, uv, 0.).rgb;\n"
"#else\n"
"	result.rgb += texture(FullbrightTex, uv).rgb;\n"
"#endif\n"
"	result.rgb = clamp(result.rgb, 0.0, 1.0);\n"
"	float fog = exp2(abs(Fog.w) * -dot(in_pos, in_pos));\n"\
"	fog = clamp(fog, 0.0, 1.0);\n"
"	result.rgb = mix(Fog.rgb, result.rgb, fog);\n"
"	out_fragcolor = result;\n"
"#if MODE == " QS_STRINGIFY (ALIASSHADER_DITHER) " || MODE == " QS_STRINGIFY (ALIASSHADER_NOPERSP) "\n"
"	// Note: sign bit is used as overbright flag\n"
"	if (abs(Fog.w) > 0.)\n"
"	{\n"
"		out_fragcolor.rgb = sqrt(out_fragcolor.rgb);\n"
"		out_fragcolor.rgb += SCREEN_SPACE_NOISE() * ScreenDither;\n"
"		out_fragcolor.rgb *= out_fragcolor.rgb;\n"
"	}\n"
"#else\n"
"	out_fragcolor.rgb += SUPPRESS_BANDING() * ScreenDither;\n"
"#endif\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Sprites
//
////////////////////////////////////////////////////////////////

static const char sprites_vertex_shader[] =
FRAMEDATA_BUFFER
"\n"
"layout(location=0) in vec3 in_pos;\n"
"layout(location=1) in vec2 in_uv;\n"
"\n"
"layout(location=0) out vec2 out_uv;\n"
"layout(location=1) out vec3 out_pos;\n"
"\n"
"void main()\n"
"{\n"
"	gl_Position = ViewProj * vec4(in_pos, 1.0);\n"
"	out_pos = in_pos - EyePos;\n"
"	out_uv = in_uv;\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char sprites_fragment_shader[] =
FRAMEDATA_BUFFER
NOISE_FUNCTIONS
"\n"
"layout(binding=0) uniform sampler2D Tex;\n"
"\n"
"layout(location=0) in vec2 in_uv;\n"
"layout(location=1) in vec3 in_pos;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	vec4 result = texture(Tex, in_uv);\n"
"	if (result.a < 0.666)\n"
"		discard;\n"
"	result.rgb = ApplyFog(result.rgb, in_pos);\n"
"	out_fragcolor = result;\n"
"#if DITHER\n"
"	if (Fog.w > 0.)\n"
"	{\n"
"		out_fragcolor.rgb = sqrt(out_fragcolor.rgb);\n"
"		out_fragcolor.rgb += SCREEN_SPACE_NOISE() * ScreenDither;\n"
"		out_fragcolor.rgb *= out_fragcolor.rgb;\n"
"	}\n"
"#else\n"
"	out_fragcolor.rgb += SUPPRESS_BANDING() * ScreenDither;\n"
"#endif\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Particles
//
////////////////////////////////////////////////////////////////

static const char particles_vertex_shader[] =
FRAMEDATA_BUFFER
"\n"
"layout(location=0) in vec3 in_pos;\n"
"layout(location=1) in vec4 in_color;\n"
"\n"
"layout(location=0) out vec2 out_uv;\n"
"layout(location=1) out vec4 out_color;\n"
"layout(location=2) out vec3 out_pos;\n"
"\n"
"layout(location=0) uniform vec3 Params;\n"
"#define ProjScale	Params.xy\n"
"#define UVScale	Params.z\n"
"\n"
"void main()\n"
"{\n"
"	// figure the current corner: (-1, -1), (-1, 1), (1, -1) or (1, 1)\n"
"	uvec2 flipsign = uvec2(gl_VertexID, gl_VertexID >> 1) << 31;\n"
"	vec2 corner = uintBitsToFloat(floatBitsToUint(-1.0) ^ flipsign);\n"
"\n"
"	// project the center of the particle\n"
"	gl_Position = ViewProj * vec4(in_pos, 1.0);\n"
"\n"
"	// hack a scale up to keep particles from disappearing\n"
"	float depthscale = max(1.0 + gl_Position.w * 0.004, 1.08);\n"
"\n"
"	// perform the billboarding\n"
"	gl_Position.xy += ProjScale * uintBitsToFloat(floatBitsToUint(vec2(depthscale)) ^ flipsign);\n"
"\n"
"	out_pos = in_pos - EyePos; // FIXME: use corner position\n"
"	out_uv = corner * UVScale;\n"
"	out_color = in_color;\n"
"#if OIT\n"
"	out_color.a *= 0.9;\n"
"#endif\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char particles_fragment_shader[] =
FRAMEDATA_BUFFER
NOISE_FUNCTIONS
"\n"
"layout(location=0) in vec2 in_uv;\n"
"layout(location=1) in vec4 in_color;\n"
"layout(location=2) in vec3 in_pos;\n"
"\n"
OIT_OUTPUT (out_fragcolor)
"\n"
"void main()\n"
"{\n"
"	out_fragcolor = in_color;\n"
"	out_fragcolor.rgb = ApplyFog(out_fragcolor.rgb, in_pos);\n"
"	float radius = length(in_uv);\n"
"	float pixel = fwidth(radius);\n"
"	out_fragcolor.a *= clamp((1. - radius) / pixel, 0., 1.);\n"
"#if DITHER\n"
"	if (Fog.w > 0.)\n"
"	{\n"
"		out_fragcolor.rgb = sqrt(out_fragcolor.rgb);\n"
"		out_fragcolor.rgb += SCREEN_SPACE_NOISE() * ScreenDither;\n"
"		out_fragcolor.rgb *= out_fragcolor.rgb;\n"
"	}\n"
"#else\n"
"	out_fragcolor.rgb += SUPPRESS_BANDING() * ScreenDither;\n"
"#endif\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Debug 3D
//
////////////////////////////////////////////////////////////////

static const char debug3d_vertex_shader[] =
FRAMEDATA_BUFFER
"\n"
"layout(location=0) in vec3 in_pos;\n"
"layout(location=1) in vec4 in_color;\n"
"\n"
"layout(location=0) out vec4 out_color;\n"
"\n"
"void main()\n"
"{\n"
"	gl_Position = ViewProj * vec4(in_pos, 1.0);\n"
"	out_color = in_color;\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char debug3d_fragment_shader[] =
"\n"
"layout(location=0) in vec4 in_color;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	out_fragcolor = in_color;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// OIT resolve
//
////////////////////////////////////////////////////////////////

static const char oit_resove_vertex_shader[] =
"void main()\n"
"{\n"
"	ivec2 v = ivec2(gl_VertexID & 1, gl_VertexID >> 1);\n"
"	gl_Position = vec4(vec2(v) * 4.0 - 1.0, 0.0, 1.0);\n"
"}\n";

static const char oit_resove_fragment_shader[] =
"layout(early_fragment_tests) in;\n"
"\n"
"#if MSAA\n"
"	#define Sampler				sampler2DMS\n"
"	#define FetchSample(s, c)	texelFetch(s, c, gl_SampleID)\n"
"#else\n"
"	#define Sampler				sampler2D\n"
"	#define FetchSample(s, c)	texelFetch(s, c, 0)\n"
"#endif\n"
"\n"
"layout(binding=0) uniform Sampler TexAccum;\n"
"layout(binding=1) uniform Sampler TexReveal;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"vec3 LinearToGamma(vec3 v)\n"
"{\n"
"#if " QS_STRINGIFY (LINEAR_SPACE_OIT) "\n"
"	return sqrt(clamp(v, 0.0, 1.0));\n"
"#else\n"
"	return clamp(v, 0.0, 1.0);\n"
"#endif\n"
"}\n"
"\n"
"// get the max value between three values\n"
"float max3(vec3 v)\n"
"{\n"
"	return max(max(v.x, v.y), v.z);\n"
"}\n"
"\n"
"void main()\n"
"{\n"
"	ivec2 coords = ivec2(gl_FragCoord.xy);\n"
"	float revealage = FetchSample(TexReveal, coords).r;\n"
"	// Note: we're using the stencil buffer to discard pixels with no contribution\n"
"	//if (revealage >= 0.999)\n"
"	//	discard;\n"
"\n"
"	vec4 accumulation = FetchSample(TexAccum, coords);\n"
"	// suppress overflow\n"
"	if (isinf(max3(abs(accumulation.rgb))))\n"
"		accumulation.rgb = vec3(accumulation.a);\n"
"\n"
"	vec3 average_color = accumulation.rgb / max(accumulation.a, 1e-5);\n"
"	out_fragcolor = vec4(LinearToGamma(average_color), 1.0 - revealage);\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// COMPUTE SHADERS
//
////////////////////////////////////////////////////////////////
//
// Clear indirect draws
//
////////////////////////////////////////////////////////////////

static const char clear_indirect_compute_shader[] =
"layout(local_size_x=64) in;\n"
"\n"
WORLD_DRAW_BUFFER
"\n"
"void main()\n"
"{\n"
"	uint thread_id = gl_GlobalInvocationID.x;\n"
"	if (thread_id < cmds.length())\n"
"		cmds[thread_id].count = 0u;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Gather indirect draws
//
////////////////////////////////////////////////////////////////

static const char gather_indirect_compute_shader[] =
"layout(local_size_x=64) in;\n"
"\n"
DRAW_ELEMENTS_INDIRECT_COMMAND
"\n"
"layout(std430, binding=5) restrict readonly buffer DrawIndirectSrcBuffer\n"
"{\n"
"	DrawElementsIndirectCommand src_cmds[];\n"
"};\n"
"\n"
"layout(std430, binding=6) restrict writeonly buffer DrawIndirectDstBuffer\n"
"{\n"
"	DrawElementsIndirectCommand dst_cmds[];\n"
"};\n"
"\n"
"struct DrawRemap\n"
"{\n"
"	uint src_call;\n"
"	uint instance_data;\n"
"};\n"
"\n"
"layout(std430, binding=7) restrict readonly buffer DrawRemapBuffer\n"
"{\n"
"	DrawRemap remap_data[];\n"
"};\n"
"\n"
"#define MAX_INSTANCES " QS_STRINGIFY (MAX_BMODEL_INSTANCES) "u\n"
"\n"
"void main()\n"
"{\n"
"	uint thread_id = gl_GlobalInvocationID.x;\n"
"	uint num_calls = remap_data.length();\n"
"	if (thread_id >= num_calls)\n"
"		return;\n"
"	DrawRemap remap = remap_data[thread_id];\n"
"	DrawElementsIndirectCommand cmd = src_cmds[remap.src_call];\n"
"	cmd.baseInstance = remap.instance_data / MAX_INSTANCES;\n"
"	cmd.instanceCount = (remap.instance_data % MAX_INSTANCES) + 1u;\n"
"	if (cmd.count == 0u)\n"
"		cmd.instanceCount = 0u;\n"
"	dst_cmds[thread_id] = cmd;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Cull/mark: leaf vis/frustum culling, surface backface culling,
// index buffer + draw indirect buffer updates
//
////////////////////////////////////////////////////////////////

static const char cull_mark_compute_shader[] =
"layout(local_size_x=64) in;\n"
"\n"
DRAW_ELEMENTS_INDIRECT_COMMAND
"\n"
"// Note: some old Intel drivers error out on shaders\n"
"// that perform atomic operations on SSBO struct members.\n"
"// As a work-around, we use uints and manual indexing.\n"
"layout(std430, binding=1) buffer DrawIndirectBuffer\n"
"{\n"
"	//DrawElementsIndirectCommand cmds[];\n"
"	uint rawcmds[];\n"
"};\n"
"\n"
"#define CMD_COUNT(base)			rawcmds[base]\n"
"#define CMD_INSTANCE_COUNT(base)	rawcmds[base + 1]\n"
"#define CMD_FIRST_INDEX(base)		rawcmds[base + 2]\n"
"#define CMD_BASE_VERTEX(base)		rawcmds[base + 3]\n"
"#define CMD_BASE_INSTANCE(base)	rawcmds[base + 4]\n"
"#define SIZEOF_CMD					5 /* uints */\n"
"\n"
"layout(std430, binding=2) restrict writeonly buffer IndexBuffer\n"
"{\n"
"	uint indices[];\n"
"};\n"
"\n"
"layout(std430, binding=3) restrict readonly buffer VisBuffer\n"
"{\n"
"	uint vis[];\n"
"};\n"
"\n"
"struct MarkSurface\n"
"{\n"
"	uint packedleafsky; // bit 0=sky; 1..31=leafindex\n"
"	uint surfindex;\n"
"};\n"
"\n"
"layout(std430, binding=4) restrict readonly buffer MarkSurfaceBuffer\n"
"{\n"
"	MarkSurface marksurfs[];\n"
"};\n"
"\n"
"struct Surface\n"
"{\n"
"	vec4	plane;\n"
"	uint	framecount;\n"
"	uint	texnum;\n"
"	uint	numedges;\n"
"	uint	firstvert;\n"
"	vec3	mins;\n"
"	uint	_pad0;\n"
"	vec3	maxs;\n"
"	uint	_pad1;\n"
"};\n"
"\n"
"// Same issue as above\n"
"layout(std430, binding=5) restrict buffer SurfaceBuffer\n"
"{\n"
"	//Surface surfaces[];\n"
"	uvec4 rawsurfaces[];\n"
"};\n"
"\n"
"#define SURF_PLANE(base)			uintBitsToFloat(rawsurfaces[base])\n"
"#define SURF_FRAMECOUNT(base)		rawsurfaces[base + 1].x\n"
"#define SURF_TEXNUM(base)			rawsurfaces[base + 1].y\n"
"#define SURF_NUMEDGES(base)		rawsurfaces[base + 1].z\n"
"#define SURF_FIRSTVERT(base)		rawsurfaces[base + 1].w\n"
"#define SURF_MINS(base)			uintBitsToFloat(rawsurfaces[base + 2].xyz)\n"
"#define SURF_MAXS(base)			uintBitsToFloat(rawsurfaces[base + 3].xyz)\n"
"#define SIZEOF_SURFACE				4 /* uvec4s */\n"
"\n"
"layout(std140, binding=1) uniform FrameCullUBO\n"
"{\n"
"	vec4	frustum[4];\n"
"	vec3	vieworg;\n"
"	uint	oldskyleaf;\n"
"	uint	framecount;\n"
"};\n"
"\n"
"void main()\n"
"{\n"
"	uint thread_id = gl_GlobalInvocationID.x;\n"
"	if (thread_id >= marksurfs.length())\n"
"		return;\n"
"	MarkSurface mark = marksurfs[thread_id];\n"
"\n"
"	// sky culling: when r_oldskyleaf is 0, surfaces inside a sky leaf are skipped\n"
"	if ((mark.packedleafsky & 1u) > oldskyleaf)\n"
"		return;\n"
"\n"
"	// vis culling\n"
"	uint leaf = mark.packedleafsky >> 1u;\n"
"	uint visible = vis[leaf >> 5u] & (1u << (leaf & 31u));\n"
"	if (visible == 0u)\n"
"		return;\n"
"\n"
"	uint surfbase = mark.surfindex * uint(SIZEOF_SURFACE);\n"
"\n"
"	// backface culling\n"
"	vec4 surfplane = SURF_PLANE(surfbase);\n"
"	if (dot(surfplane.xyz, vieworg) < surfplane.w)\n"
"		return;\n"
"\n"
"	// frustum culling\n"
"	vec3 mins = SURF_MINS(surfbase);\n"
"	vec3 maxs = SURF_MAXS(surfbase);\n"
"	for (uint i = 0u; i < 4u; i++)\n"
"	{\n"
"		vec4 plane = frustum[i];\n"
"		vec3 v;\n"
"		v.x = plane.x < 0.0 ? mins.x : maxs.x;\n"
"		v.y = plane.y < 0.0 ? mins.y : maxs.y;\n"
"		v.z = plane.z < 0.0 ? mins.z : maxs.z;\n"
"		if (dot(plane.xyz, v) < plane.w)\n"
"			return;\n"
"	}\n"
"\n"
"	// surfaces can appear in multiple leaves\n"
"	// check if this is the first time this surface has passed culling this frame\n"
"	if (atomicExchange(SURF_FRAMECOUNT(surfbase), framecount) == framecount)\n"
"		return;\n"
"\n"
"	// surface is visible, append its triangles to the index buffer\n"
"	// and update the draw command corresponding to its texture number\n"
"	uint texnum = SURF_TEXNUM(surfbase);\n"
"	uint numedges = SURF_NUMEDGES(surfbase);\n"
"	uint firstvert = SURF_FIRSTVERT(surfbase);\n"
"	uint cmdbase = texnum * uint(SIZEOF_CMD);\n"
"	// some bsps out there have faces with < 2 edges, which would cause underflow below\n"
"	numedges = max(numedges, 2u);\n"
"	uint ofs = CMD_FIRST_INDEX(cmdbase) + atomicAdd(CMD_COUNT(cmdbase), 3u * (numedges - 2u));\n"
"	for (uint i = 2u; i < numedges; i++)\n"
"	{\n"
"		indices[ofs++] = firstvert;\n"
"		indices[ofs++] = firstvert + i - 1u;\n"
"		indices[ofs++] = firstvert + i;\n"
"	}\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Light clustering
//
////////////////////////////////////////////////////////////////

static const char cluster_lights_compute_shader[] =
"layout(local_size_x=8, local_size_y=8, local_size_z=1) in;\n"
"\n"
FRAMEDATA_BUFFER
LIGHT_BUFFER
"\n"
LIGHT_CLUSTER_IMAGE("writeonly")
"\n"
"layout(std140, binding=1) uniform InputUBO\n"
"{\n"
"	mat4	TransposedProj;\n"
"	mat4	View;\n"
"};\n"
"\n"
"shared vec4 local_lights[MAX_LIGHTS]; // xyz = view space pos; w = radius\n"
"\n"
"vec4 cluster_planes[6]; // view space; facing outside\n"
"vec3 cluster_center;\n"
"vec3 cluster_half_size;\n"
"\n"
"vec4 ExtractFrustumPlane(int axis, float ndcval, float side)\n"
"{\n"
"	vec4 plane = TransposedProj[axis] - ndcval * TransposedProj[3];\n"
"	return inversesqrt(dot(plane.xyz, plane.xyz)) * side * plane;\n"
"}\n"
"\n"
"void ComputeClusterPlanes(uvec3 gid)\n"
"{\n"
"	const float TileSizeX = 2.0 / float(LIGHT_TILES_X);\n"
"	const float TileSizeY = 2.0 / float(LIGHT_TILES_Y);\n"
"	float x0 = -1.0 + float(gid.x) * TileSizeX;\n"
"	float y0 = -1.0 + float(gid.y) * TileSizeY;\n"
"	float z0 = exp2((float(gid.z) - ZLogBias) / ZLogScale);\n"
"	cluster_planes[0] = ExtractFrustumPlane(0, x0,             -1.0);      // left\n"
"	cluster_planes[1] = ExtractFrustumPlane(0, x0 + TileSizeX,  1.0);      // right\n"
"	cluster_planes[2] = ExtractFrustumPlane(1, y0,             -1.0);      // bottom\n"
"	cluster_planes[3] = ExtractFrustumPlane(1, y0 + TileSizeY,  1.0);      // top\n"
"	cluster_planes[4] = vec4(-1.0, 0.0, 0.0,  z0);                         // near\n"
"	cluster_planes[5] = vec4( 1.0, 0.0, 0.0, -z0 * exp2(1.0 / ZLogScale)); // far\n"
"}\n"
"\n"
"float PointPlaneDistance(vec3 p, vec4 plane)\n"
"{\n"
"	return dot(p, plane.xyz) + plane.w;\n"
"}\n"
"\n"
"vec3 IntersectDepthPlane(vec3 dir, float depth)\n"
"{\n"
"	return vec3(depth, (depth / dir.x) * dir.yz);\n"
"}\n"
"\n"
"void ComputeClusterExtents()\n"
"{\n"
"	vec3 bl = cross(cluster_planes[2].xyz, cluster_planes[0].xyz); // bottom-left\n"
"	vec3 tr = cross(cluster_planes[3].xyz, cluster_planes[1].xyz); // top-right\n"
"	float depth_near = cluster_planes[4].w;\n"
"	float depth_far = -cluster_planes[5].w;\n"
"	vec3 p0 = IntersectDepthPlane(bl, depth_near);\n"
"	vec3 p1 = IntersectDepthPlane(bl, depth_far);\n"
"	vec3 p2 = IntersectDepthPlane(tr, depth_near);\n"
"	vec3 p3 = IntersectDepthPlane(tr, depth_far);\n"
"	vec3 cluster_mins = vec3(depth_near, min(min(p0.yz, p1.yz), min(p2.yz, p3.yz)));\n"
"	vec3 cluster_maxs = vec3(depth_far,  max(max(p0.yz, p1.yz), max(p2.yz, p3.yz)));\n"
"	cluster_center = (cluster_mins + cluster_maxs) * 0.5;\n"
"	cluster_half_size = (cluster_maxs - cluster_mins) * 0.5;\n"
"}\n"
"\n"
"bool LightTouchesCluster(vec4 l)\n"
"{\n"
"#if 1\n"
"	vec3 delta = max(abs(l.xyz - cluster_center) - cluster_half_size, 0.0);\n"
"	if (dot(delta, delta) >= l.w * l.w)\n"
"		return false;\n"
"#endif\n"
"#if 0\n"
"	for (int i = 0; i < 6; i++)\n"
"		if (PointPlaneDistance(l.xyz, cluster_planes[i]) > l.w)\n"
"			return false;\n"
"#endif\n"
"	return true;\n"
"}\n"
"\n"
"void main()\n"
"{\n"
"	uvec3 gid = gl_GlobalInvocationID;\n"
"	if (any(greaterThanEqual(gid, uvec3(LIGHT_TILES_X, LIGHT_TILES_Y, LIGHT_TILES_Z))))\n"
"		return;\n"
"	uint numlights = NumLights;\n"
"	if (numlights == 0u)\n"
"	{\n"
"		imageStore(LightClusters, ivec3(gid), uvec4(0u));\n"
"		return;\n"
"	}\n"
"	uint groupsize = gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z;\n"
"	uint numpasses = (numlights + (groupsize - 1u)) / groupsize;\n"
"	uint i, j, ofs;\n"
"	for (i = 0u, ofs = 0u; i < numpasses; i++, ofs += groupsize)\n"
"	{\n"
"		uint index = gl_LocalInvocationIndex + ofs;\n"
"		if (index < numlights)\n"
"		{\n"
"			Light l = Lights[index];\n"
"			local_lights[index] = vec4((View * vec4(l.origin, 1.0)).xyz, l.radius);\n"
"		}\n"
"	}\n"
"	memoryBarrierShared();\n"
"	barrier();\n"
"\n"
"	ComputeClusterPlanes(gid);\n"
"	ComputeClusterExtents();\n"
"\n"
"	uint clustermask[MAX_LIGHTS / 32];\n"
"	for (i = 0u; i < clustermask.length(); i++)\n"
"		clustermask[i] = 0u;\n"
"	for (i = 0u; i < numlights; i++)\n"
"		if (LightTouchesCluster(local_lights[i]))\n"
"			clustermask[i >> 5u] |= 1u << (i & 31u);\n"
"	imageStore(LightClusters, ivec3(gid), uvec4(clustermask[0], clustermask[1], 0u, 0u));\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Palette initialization
//
////////////////////////////////////////////////////////////////

static const char palette_init_compute_shader[] =
"layout(local_size_x=128) in;\n"
"\n"
"layout(location=0) uniform int Offset;\n"
"\n"
PALETTE_BUFFER
"\n"
"layout(r8ui, binding=0) uniform writeonly uimage3D PaletteLUT;\n"
"\n"
"vec3 sRGBToLinearRGB(vec3 v)\n"
"{\n"
"	return mix(v / 12.92, pow((v + 0.055) / 1.055, vec3(2.4)), greaterThan(v, vec3(0.04045)));\n"
"}\n"
"\n"
"// A perceptual color space for image processing - Bjorn Ottosson\n"
"// https://bottosson.github.io/posts/oklab/ \n"
"vec3 LinearRGBToOKLab(vec3 c)\n"
"{\n"
"	float l = 0.4122214708 * c.r + 0.5363325363 * c.g + 0.0514459929 * c.b;\n"
"	float m = 0.2119034982 * c.r + 0.6806995451 * c.g + 0.1073969566 * c.b;\n"
"	float s = 0.0883024619 * c.r + 0.2817188376 * c.g + 0.6299787005 * c.b;\n"
"\n"
"	float l_ = pow(l, 1./3.);\n"
"	float m_ = pow(m, 1./3.);\n"
"	float s_ = pow(s, 1./3.);\n"
"\n"
"	return vec3(\n"
"		0.2104542553*l_ + 0.7936177850*m_ - 0.0040720468*s_,\n"
"		1.9779984951*l_ - 2.4285922050*m_ + 0.4505937099*s_,\n"
"		0.0259040371*l_ + 0.7827717662*m_ - 0.8086757660*s_\n"
"	);\n"
"}\n"
"\n"
"vec3 NormalizeColor(uvec3 clr8)\n"
"{\n"
"	vec3 clr = vec3(clr8) * (1./255.);\n"
"#if MODE >= 1\n"
"	clr = sRGBToLinearRGB(clr);\n"
"	#if MODE >= 2\n"
"		clr = LinearRGBToOKLab(clr);\n"
"	#endif\n"
"#endif\n"
"	return clr;\n"
"}\n"
"\n"
"float ColorDistanceSquared(vec3 c0, vec3 c1)\n"
"{\n"
"	vec3 delta = c1 - c0;\n"
"#if MODE == 1\n"
"	// Colour metric - Thiadmer Riemersma\n"
"	// https://www.compuphase.com/cmetric.htm \n"
"	float rmean = (c0.r + c1.r) * 0.5;\n"
"	return dot(delta * delta, vec3(2. + rmean, 4., 3. - rmean));\n"
"#elif MODE == 2\n"
"	delta.x *= 1.25; // lightness\n"
"	delta.z *= 1.5;  // blue-yellow\n"
"#endif\n"
"	return dot(delta, delta);\n"
"}\n"
"\n"
"shared vec3 palcolors[256];\n"
"\n"
"void main()\n"
"{\n"
"	uint groupsize = gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z;\n"
"	uint numpasses = (256 + (groupsize - 1u)) / groupsize;\n"
"	uint i, ofs;\n"
"	for (i = 0u, ofs = 0u; i < numpasses; i++, ofs += groupsize)\n"
"	{\n"
"		uint idx = gl_LocalInvocationIndex + ofs;\n"
"		if (idx < 256u)\n"
"			palcolors[idx] = NormalizeColor(UnpackRGB8(Palette[idx]));\n"
"	}\n"
"	memoryBarrierShared();\n"
"	barrier();\n"
"\n"
"	uvec3 gid = gl_GlobalInvocationID + UnpackRGB8(uint(Offset));\n"
"	vec3 target = NormalizeColor((gid << 1) + (gid >> 6));\n"
"	uint bestidx = 0;\n"
"	float bestdist = 1e+32;\n"
"	for (i = 0u; i < 256u; i++)\n"
"	{\n"
"		vec3 candidate = palcolors[i];\n"
"		float dist = ColorDistanceSquared(target, candidate);\n"
"		if (dist < bestdist)\n"
"		{\n"
"			bestidx = i;\n"
"			bestdist = dist;\n"
"		}\n"
"	}\n"
"	imageStore(PaletteLUT, ivec3(gid), uvec4(bestidx, 0, 0, 0));\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Palette postprocess (color blending, gamma, contrast)
//
////////////////////////////////////////////////////////////////

static const char palette_postprocess_compute_shader[] =
"layout(local_size_x=64) in;\n"
"\n"
"layout(location=0) uniform vec2 GammmaContrast;\n"
"layout(location=1) uniform vec4 BlendColor;\n"
"\n"
PALETTE_BUFFER
"\n"
"layout(std430, binding=1) restrict writeonly buffer DstPaletteBuffer\n"
"{\n"
"	uint DstPalette[256];\n"
"};\n"
"\n"
"void main()\n"
"{\n"
"	float gamma = GammmaContrast.x;\n"
"	float contrast = GammmaContrast.y;\n"
"	uint idx = gl_GlobalInvocationID.x;\n"
"	if (idx >= 256u)\n"
"		return;\n"
"	vec3 color = vec3(UnpackRGB8(Palette[idx])) * (1./255.);\n"
"	color = mix(color, BlendColor.rgb, BlendColor.a);\n"
"	color *= contrast;\n"
"	color = pow(color, vec3(gamma));\n"
"	uvec3 dst = uvec3(clamp(color, 0., 1.) * 255. + .5);\n"
"	DstPalette[idx] = dst.r | (dst.g << 8) | (dst.b << 16) | 0xff000000u;\n"
"}\n";
