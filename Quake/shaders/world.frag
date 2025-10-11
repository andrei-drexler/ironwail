#if BINDLESS
	#extension GL_ARB_bindless_texture : require
#else
	layout(binding=0) uniform sampler2D Tex;
	layout(binding=1) uniform sampler2D FullbrightTex;
#endif
layout(binding=2) uniform sampler2D LMTex;
layout(binding=3) uniform sampler2DArray ShadowMap;

#include "shadow_common.glsl"

int   gShadowCascadeCount = 0;
int   gShadowCascadeIndex = 0;
int   gShadowCascadeNext = -1;
float gShadowCascadeBlend = 0.0;
float gShadowPrimaryValid = 0.0;
float gShadowSecondaryValid = 0.0;
vec3  gShadowCoordPrimary = vec3(0.0);
vec3  gShadowCoordSecondary = vec3(0.0);
float gShadowViewDepth = 0.0;
float gShadowCanonicalPrimary = 0.0;
float gShadowCanonicalSecondary = 0.0;

vec3 ApplyFog(vec3 clr, vec3 p)
{
        float fog = exp2(-Fog.w * dot(p, p));
	fog = clamp(fog, 0.0, 1.0);
	return mix(Fog.rgb, clr, fog);
}

#define LIGHT_TILES_X 32
#define LIGHT_TILES_Y 16
#define LIGHT_TILES_Z 32
#define MAX_LIGHTS    64

struct Light
{
	vec3	origin;
	float	radius;
	vec3	color;
	float	minlight;
};

layout(std430, binding=0) restrict readonly buffer LightBuffer
{
	float	LightStyles[64];
	Light	Lights[];
};

float GetLightStyle(int index)
{
	float result;
	if (index < 64)
		result = LightStyles[index];
	else
		result = 1.0;
	return result;
}

layout(rg32ui, binding=0) uniform readonly uimage3D LightClusters;
struct Call
{
	uint	flags;
	float	wateralpha;
#if BINDLESS
	uvec2	txhandle;
	uvec2	fbhandle;
#else
	int		baseinstance;
	int		padding;
#endif // BINDLESS
};
const uint
	CF_USE_POLYGON_OFFSET = 1u,
	CF_USE_FULLBRIGHT = 2u,
	CF_NOLIGHTMAP = 4u,
	CF_ALPHA_TEST = 8u
;

layout(std430, binding=1) restrict readonly buffer CallBuffer
{
	Call call_data[];
};

#if BINDLESS
	#define GET_INSTANCE_ID(call) (gl_BaseInstanceARB + gl_InstanceID)
#else
	#define GET_INSTANCE_ID(call) (call.baseinstance + gl_InstanceID)
#endif
struct Instance
{
	vec4	mat[3];
	float	alpha;
};

layout(std430, binding=2) restrict readonly buffer InstanceBuffer
{
	Instance instance_data[];
};

vec3 Transform(vec3 p, Instance instance)
{
	mat4x3 world = transpose(mat3x4(instance.mat[0], instance.mat[1], instance.mat[2]));
	return mat3(world[0], world[1], world[2]) * p + world[3];
}

// ALU-only 16x16 Bayer matrix
float bayer01(ivec2 coord)
{
	coord &= 15;
	coord.y ^= coord.x;
	uint v = uint(coord.y | (coord.x << 8));	// 0  0  0  0 | x3 x2 x1 x0 |  0  0  0  0 | y3 y2 y1 y0
	v = (v ^ (v << 2)) & 0x3333;				// 0  0 x3 x2 |  0  0 x1 x0 |  0  0 y3 y2 |  0  0 y1 y0
	v = (v ^ (v << 1)) & 0x5555;				// 0 x3  0 x2 |  0 x1  0 x0 |  0 y3  0 y2 |  0 y1  0 y0
	v |= v >> 7;								// 0 x3  0 x2 |  0 x1  0 x0 | x3 y3 x2 y2 | x1 y1 x0 y0
	v = bitfieldReverse(v) >> 24;				// 0  0  0  0 |  0  0  0  0 | y0 x0 y1 x1 | y2 x2 y3 x3
	return float(v) * (1.0/256.0);
}

float bayer(ivec2 coord)
{
	return bayer01(coord) - 0.5;
}

// Hash without Sine
// https://www.shadertoy.com/view/4djSRW 
float whitenoise01(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * .1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.x + p3.y) * p3.z);
}

float whitenoise(vec2 p)
{
	return whitenoise01(p) - 0.5;
}

// Convert uniform distribution to triangle-shaped distribution
// Input in [0..1], output in [-1..1]
// Based on https://www.shadertoy.com/view/4t2SDh 
float tri(float x)
{
	float orig = x * 2.0 - 1.0;
	uint signbit = floatBitsToUint(orig) & 0x80000000u;
	x = sqrt(abs(orig)) - 1.;
	x = uintBitsToFloat(floatBitsToUint(x) ^ signbit);
	return x;
}

#define DITHER_NOISE(uv) tri(bayer01(ivec2(uv)))
#define SCREEN_SPACE_NOISE() DITHER_NOISE(floor(gl_FragCoord.xy)+0.5)
#define SUPPRESS_BANDING() bayer(ivec2(gl_FragCoord.xy))

float DepthToCanonical(float depth)
{
#if REVERSED_Z
        return 1.0 - depth;
#else
        return depth;
#endif
}

float SampleShadowHard(vec3 coord, float canonical_depth)
{
        float sample_depth = texture(ShadowMap, coord).r;
        float sample_canonical = DepthToCanonical(sample_depth);
        return canonical_depth <= sample_canonical ? 1.0 : 0.0;
}

float SampleShadowPCF(vec3 coord, float canonical_depth, float texel_size, int kernel_radius, float radius_scale)
{
        const int MAX_RADIUS = 3;
        if (kernel_radius <= 0 || texel_size <= 0.0)
                return SampleShadowHard(coord, canonical_depth);
        float step_size = texel_size * radius_scale;
        float sum = 0.0;
        float weight = 0.0;
        for (int y = -MAX_RADIUS; y <= MAX_RADIUS; ++y)
        {
                if (abs(y) > kernel_radius)
                        continue;
                for (int x = -MAX_RADIUS; x <= MAX_RADIUS; ++x)
                {
                        if (abs(x) > kernel_radius)
                                continue;
                        vec2 offset = vec2(float(x), float(y)) * step_size;
                        sum += SampleShadowHard(vec3(coord.xy + offset, coord.z), canonical_depth);
                        weight += 1.0;
                }
        }
        return weight > 0.0 ? sum / weight : 1.0;
}

float EvaluateShadowVSM(vec3 coord, float canonical_depth)
{
        vec2 moments = texture(ShadowMap, coord).rg;
#if REVERSED_Z
        float Ex = 1.0 - moments.x;
        float Ex2 = 1.0 - 2.0 * moments.x + moments.y;
#else
        float Ex = moments.x;
        float Ex2 = moments.y;
#endif
        float variance = max(Ex2 - Ex * Ex, ShadowVSM.z);
        float delta = canonical_depth - Ex;
        float p = variance / (variance + delta * delta);
        p = clamp((p - ShadowVSM.y) / (1.0 - ShadowVSM.y), 0.0, 1.0);
        if (canonical_depth <= Ex)
                return 1.0;
        return p;
}

bool ComputeCascadeCoord(int cascadeIndex, vec3 offset_pos, float ndotl, out vec3 out_coord, out float out_canonical)
{
        vec4 shadow_clip = ShadowViewProj[cascadeIndex] * vec4(offset_pos, 1.0);
        if (shadow_clip.w <= 0.0)
                return false;
        vec3 projected = shadow_clip.xyz / shadow_clip.w;
#if !REVERSED_Z
        projected.z = projected.z * 0.5 + 0.5;
#endif
        projected.xy = projected.xy * 0.5 + 0.5;
        float bias = ShadowParams.x + ShadowParams.y * ShadowParams.z * (1.0 - ndotl);
#if REVERSED_Z
        projected.z -= bias;
#else
        projected.z += bias;
#endif
        if (projected.x < 0.0 || projected.x > 1.0 || projected.y < 0.0 || projected.y > 1.0)
                return false;
        if (projected.z <= 0.0 || projected.z >= 1.0)
                return false;
        out_coord = vec3(projected.xy, float(cascadeIndex));
        out_canonical = DepthToCanonical(clamp(projected.z, 0.0, 1.0));
        return true;
}

float FilterShadowValue(vec3 coord, float canonical_depth, vec3 offset_pos)
{
        if (ShadowVSM.x > 0.5)
                return EvaluateShadowVSM(coord, canonical_depth);

        float texel_size = ShadowParams.z;
        int kernel_radius = clamp(int(ShadowFilter.x + 0.5), 0, 3);
        if (ShadowFilter.y <= 0.5 || kernel_radius <= 0 || texel_size <= 0.0)
                return SampleShadowHard(coord, canonical_depth);

        float distance = length(offset_pos - EyePos);
        float radius_scale = clamp(1.0 + ShadowFilter.w * distance, 1.0, 6.0);
        return SampleShadowPCF(coord, canonical_depth, texel_size, kernel_radius, radius_scale);
}

float EvaluateShadow(vec3 world_pos, vec3 normal, vec3 light_dir, float view_depth)
{
        gShadowCascadeCount = int(ShadowParams.w + 0.5);
        gShadowCascadeIndex = 0;
        gShadowCascadeNext = -1;
        gShadowCascadeBlend = 0.0;
        gShadowPrimaryValid = 0.0;
        gShadowSecondaryValid = 0.0;
        gShadowCoordPrimary = vec3(0.0);
        gShadowCoordSecondary = vec3(0.0);
        gShadowViewDepth = view_depth;
        gShadowCanonicalPrimary = 0.0;
        gShadowCanonicalSecondary = 0.0;

        int cascadeCount = gShadowCascadeCount;
        if (cascadeCount <= 0)
                return 1.0;

        float ndotl = max(dot(normal, light_dir), 0.0);
        float receiver_offset = ShadowFilter[2] * (1.0 - ndotl);
        vec3 offset_pos = world_pos;
        if (receiver_offset > 0.0)
                offset_pos += normal * receiver_offset;

        int cascadeIndex = cascadeCount - 1;
        for (int i = 0; i < cascadeCount; ++i)
        {
                if (view_depth <= ShadowCascadeSplits[i])
                {
                        cascadeIndex = i;
                        break;
                }
        }
        if (view_depth > ShadowCascadeSplits[cascadeCount - 1])
                return 1.0;

        vec3 coord;
        float canonical_depth;
        if (!ComputeCascadeCoord(cascadeIndex, offset_pos, ndotl, coord, canonical_depth))
                return 1.0;

        gShadowCascadeIndex = cascadeIndex;
        gShadowPrimaryValid = 1.0;
        gShadowCoordPrimary = coord;
        gShadowCanonicalPrimary = canonical_depth;

        float shadow = FilterShadowValue(coord, canonical_depth, offset_pos);

        if (ShadowCascadeFade.y > 0.5 && cascadeIndex < cascadeCount - 1)
        {
                float cascadeNear = ShadowCascadeStarts[cascadeIndex];
                float cascadeFar = ShadowCascadeSplits[cascadeIndex];
                float range = cascadeFar - cascadeNear;
                float fadeDistance = range * ShadowCascadeFade.x;
                if (fadeDistance > 0.0)
                {
                        float distanceToFar = cascadeFar - view_depth;
                        float blend = clamp(1.0 - distanceToFar / fadeDistance, 0.0, 1.0);
                        if (blend > 0.0)
                        {
                                vec3 nextCoord;
                                float nextCanonical;
                                if (ComputeCascadeCoord(cascadeIndex + 1, offset_pos, ndotl, nextCoord, nextCanonical))
                                {
                                        gShadowCascadeNext = cascadeIndex + 1;
                                        gShadowCascadeBlend = blend;
                                        gShadowSecondaryValid = 1.0;
                                        gShadowCoordSecondary = nextCoord;
                                        gShadowCanonicalSecondary = nextCanonical;
                                        float shadowNext = FilterShadowValue(nextCoord, nextCanonical, offset_pos);
                                        shadow = mix(shadow, shadowNext, blend);
                                }
                        }
                }
        }

        return shadow;
}

vec3 ComputeSunLight(vec3 world_pos, vec3 normal)
{
	if (ShadowParams.w <= 0.5)
		return vec3(0.0);
	vec3 light_dir = ShadowSunDir.xyz;
	float len_dir = length(light_dir);
	if (len_dir <= 0.0)
		return vec3(0.0);
	light_dir /= len_dir;
	float ndotl = max(dot(normal, light_dir), 0.0);
	if (ndotl <= 0.0)
		return vec3(0.0);
	float intensity = ShadowSunDir.w;
	if (intensity > 1.0)
		intensity *= (1.0 / 255.0);
        float visibility = EvaluateShadow(world_pos, normal, light_dir, in_depth);
        return ShadowSunColor.rgb * intensity * ndotl * visibility;
}

layout(location=0) flat in uint in_flags;
layout(location=1) flat in float in_alpha;
layout(location=2) in vec3 in_pos;
#if MODE == 1
	layout(location=3) centroid in vec2 in_uv;
#else
	layout(location=3) in vec2 in_uv;
#endif
layout(location=4) centroid in vec2 in_lmuv;
layout(location=5) in float in_depth;
layout(location=6) noperspective in vec2 in_coord;
layout(location=7) flat in vec4 in_styles;
layout(location=8) flat in float in_lmofs;
#if BINDLESS
	layout(location=9) flat in uvec4 in_samplers;
#endif

#define OUT_COLOR out_fragcolor
#if OIT
	vec4 OUT_COLOR;
	layout(location=0) out vec4 out_accum;
	layout(location=1) out float out_reveal;

	vec3 GammaToLinear(vec3 v)
	{
#if 0
		return v*v;
#else
		return v;
#endif
	}

	void main_body();

	void main()
	{
		main_body();
		OUT_COLOR = clamp(OUT_COLOR, 0.0, 1.0);
		vec4 color = vec4(GammaToLinear(OUT_COLOR.rgb), OUT_COLOR.a);
		float z = 1./gl_FragCoord.w;
#if 0
		float weight = clamp(color.a * color.a * 0.03 / (1e-5 + pow(z/2e5, 2.0)), 1e-2, 3e3);
#else
		float weight = clamp(color.a * color.a * 0.03 / (1e-5 + pow(z/1e7, 1.0)), 1e-2, 3e3);
#endif
		out_accum = vec4(color.rgb, color.a * weight);
		out_accum.rgb *= out_accum.a;
		out_reveal = color.a;
	}

	#define main main_body
#else
	layout(location=0) out vec4 OUT_COLOR;
#endif // OIT

void main()
{
#if 0
	out_fragcolor = vec4(0.5 + 0.5 * normalize(cross(dFdx(in_pos), dFdy(in_pos))), 0.75);
	return;
#endif
	vec3 fullbright = vec3(0.);
	vec2 uv = in_uv;
#if MODE == 2
	uv = uv * 2.0 + 0.125 * sin(uv.yx * (3.14159265 * 2.0) + Time);
#endif
#if BINDLESS
	sampler2D Tex = sampler2D(in_samplers.xy);
	sampler2D FullbrightTex;
	if ((in_flags & CF_USE_FULLBRIGHT) != 0u)
	{
		FullbrightTex = sampler2D(in_samplers.zw);
		fullbright = texture(FullbrightTex, uv).rgb;
	}
#else
	if ((in_flags & CF_USE_FULLBRIGHT) != 0u)
		fullbright = texture(FullbrightTex, uv).rgb;
#endif
#if DITHER >= 2
	vec4 result = texture(Tex, uv, -1.0);
#elif DITHER
	vec4 result = texture(Tex, uv, -0.5);
#else
	vec4 result = texture(Tex, uv);
#endif
#if MODE == 1
	if (result.a < 0.666)
		discard;
#endif

	vec2 lmuv = in_lmuv;
#if DITHER
	vec2 lmsize = vec2(textureSize(LMTex, 0).xy) * 16.;
	lmuv = (floor(lmuv * lmsize) + 0.5) / lmsize;
#endif // DITHER
	vec4 lm0 = textureLod(LMTex, lmuv, 0.);
        vec3 static_light;
        if (in_styles.y < 0.) // single style fast path
                static_light = in_styles.x * lm0.xyz;
        else
        {
                vec4 lm1 = textureLod(LMTex, vec2(lmuv.x + in_lmofs, lmuv.y), 0.);
                if (in_styles.z < 0.) // 2 styles
                {
                        static_light =
                                in_styles.x * lm0.xyz +
                                in_styles.y * lm1.xyz;
                }
                else // 3 or 4 lightstyles
                {
                        vec4 lm2 = textureLod(LMTex, vec2(lmuv.x + in_lmofs * 2., lmuv.y), 0.);
                        static_light = vec3
                        (
                                dot(in_styles, lm0),
                                dot(in_styles, lm1),
                                dot(in_styles, lm2)
                        );
                }
        }

        vec3 surface_normal = vec3(0.0, 0.0, 1.0);
        vec3 surface_normal_vec = cross(dFdx(in_pos), dFdy(in_pos));
        float surface_normal_len = length(surface_normal_vec);
        if (surface_normal_len > 0.0)
                surface_normal = surface_normal_vec / surface_normal_len;
        vec3 total_light = clamp(static_light, 0.0, 1.0);

        if (NumLights > 0u)
        {
                uint i, ofs;
                ivec3 cluster_coord;
                cluster_coord.x = int(floor(in_coord.x));
		cluster_coord.y = int(floor(in_coord.y));
		cluster_coord.z = int(floor(log2(in_depth) * ZLogScale + ZLogBias));
		uvec2 clusterdata = imageLoad(LightClusters, cluster_coord).xy;
		if ((clusterdata.x | clusterdata.y) != 0u)
		{
#if 0
			int cluster_idx = cluster_coord.x + cluster_coord.y * LIGHT_TILES_X + cluster_coord.z * LIGHT_TILES_X * LIGHT_TILES_Y;
			total_light = vec3(ivec3((cluster_idx + 1) * 0x45d9f3b) >> ivec3(0, 8, 16) & 255) / 255.0;
#endif // SHOW_ACTIVE_LIGHT_CLUSTERS
			vec3 dynamic_light = vec3(0.);
			vec4 plane;
			plane.xyz = surface_normal;
			plane.w = dot(in_pos, plane.xyz);
			for (i = 0u, ofs = 0u; i < 2u; i++, ofs += 32u)
			{
				uint mask = clusterdata[i];
				while (mask != 0u)
				{
					int j = findLSB(mask);
					mask ^= 1u << j;
					Light l = Lights[ofs + j];
					// mimics R_AddDynamicLights, up to a point
					float rad = l.radius;
					float dist = dot(l.origin, plane.xyz) - plane.w;
					rad -= abs(dist);
					float minlight = l.minlight;
					if (rad < minlight)
						continue;
					vec3 local_pos = l.origin - plane.xyz * dist;
					minlight = rad - minlight;
					dist = length(in_pos - local_pos);
					dynamic_light += clamp((minlight - dist) / 16.0, 0.0, 1.0) * max(0., rad - dist) / 256. * l.color;
				}
			}
                        total_light += max(min(dynamic_light, 1. - total_light), 0.);
                }
        }

        vec3 sun_light = ComputeSunLight(in_pos, surface_normal);
        total_light += max(min(sun_light, 1. - total_light), 0.);
#if DITHER >= 2
        vec3 clamped_light = clamp(total_light, 0.0, 1.0);
        vec3 total_lightmap = clamp(floor(clamped_light * 63. + 0.5) * (Overbright / 63.), 0.0, Overbright);
#else
        vec3 total_lightmap = clamp(total_light * Overbright, 0.0, Overbright);
#endif
#if MODE != 1
        result.rgb = mix(result.rgb, result.rgb * total_lightmap, result.a);
#else
        result.rgb *= total_lightmap;
#endif
	result.rgb += fullbright;
	result = clamp(result, 0.0, 1.0);
        result.rgb = ApplyFog(result.rgb, in_pos - EyePos);

        if (ShadowDebug.x > 0.5 && gShadowPrimaryValid > 0.5)
        {
                const vec3 cascadeColors[4] = vec3[4]
                (
                        vec3(0.95, 0.45, 0.45),
                        vec3(0.45, 0.95, 0.55),
                        vec3(0.45, 0.65, 0.95),
                        vec3(0.95, 0.75, 0.45)
                );
                int idx = clamp(gShadowCascadeIndex, 0, 3);
                vec3 overlay = cascadeColors[idx];
                if (gShadowSecondaryValid > 0.5)
                {
                        int nextIdx = clamp(gShadowCascadeNext, 0, 3);
                        overlay = mix(overlay, cascadeColors[nextIdx], clamp(gShadowCascadeBlend, 0.0, 1.0));
                }
                result.rgb = mix(result.rgb, overlay, 0.35);
        }

        int debugMapIndex = int(ShadowDebug.y + 0.5) - 1;
        if (debugMapIndex >= 0)
        {
                vec3 coord = vec3(0.0);
                float canonicalDepth = 0.0;
                bool valid = false;
                if (debugMapIndex == gShadowCascadeIndex && gShadowPrimaryValid > 0.5)
                {
                        coord = gShadowCoordPrimary;
                        canonicalDepth = gShadowCanonicalPrimary;
                        valid = true;
                }
                else if (debugMapIndex == gShadowCascadeNext && gShadowSecondaryValid > 0.5)
                {
                        coord = gShadowCoordSecondary;
                        canonicalDepth = gShadowCanonicalSecondary;
                        valid = true;
                }
                else
                {
                        vec3 dbgDir = ShadowSunDir.xyz;
                        float dbgLen = length(dbgDir);
                        if (dbgLen > 0.0)
                        {
                                dbgDir /= dbgLen;
                                float ndotl_dbg = max(dot(surface_normal, dbgDir), 0.0);
                                float receiver_offset_dbg = ShadowFilter[2] * (1.0 - ndotl_dbg);
                                vec3 offset_dbg = in_pos;
                                if (receiver_offset_dbg > 0.0)
                                        offset_dbg += surface_normal * receiver_offset_dbg;
                                if (ComputeCascadeCoord(debugMapIndex, offset_dbg, ndotl_dbg, coord, canonicalDepth))
                                        valid = true;
                        }
                }
                if (valid)
                {
                        float depthSample = texture(ShadowMap, coord).r;
                        float canonicalSample = DepthToCanonical(depthSample);
                        result = vec4(vec3(canonicalSample), 1.0);
                }
                else
                {
                        result = vec4(vec3(0.0), 1.0);
                }
        }

        result.a = in_alpha; // FIXME: This will make almost transparent things cut holes though heavy fog
        out_fragcolor = result;
#if DITHER == 1
	vec3 dpos = fwidth(in_pos);
	float farblend = clamp(max(dpos.x, max(dpos.y, dpos.z)) * 0.5 - 0.125, 0., 1.);
	farblend *= farblend;
	out_fragcolor.rgb = sqrt(out_fragcolor.rgb);
	float luma = dot(out_fragcolor.rgb, vec3(.25, .625, .125));
	float nearnoise = tri(whitenoise01(lmuv * lmsize)) * luma * TextureDither;
	float farnoise = Fog.w > 0. ? SCREEN_SPACE_NOISE() * ScreenDither : 0.;
	out_fragcolor.rgb += mix(nearnoise, farnoise, farblend);
	out_fragcolor.rgb *= out_fragcolor.rgb;
#endif // DITHER == 1
#if DITHER >= 2
	// nuke extra precision in 10-bit framebuffer
	out_fragcolor.rgb = floor(out_fragcolor.rgb * 255. + 0.5) * (1./255.);
#elif DITHER == 0
	out_fragcolor.rgb += SUPPRESS_BANDING() * ScreenDither;
#endif
}
