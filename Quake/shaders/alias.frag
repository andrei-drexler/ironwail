struct InstanceData
{
	vec4	WorldMatrix[3];
	vec4	LightColor; // xyz=LightColor w=Alpha
	int		Pose1;
	int		Pose2;
	float	Blend;
	int		Padding;
};

layout(std430, binding=1) restrict readonly buffer InstanceBuffer
{
	mat4	ViewProj;
	vec3	EyePos;
	vec4	Fog;
	float	ScreenDither;
	InstanceData instances[];
};
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

layout(binding=0) uniform sampler2D Tex;
layout(binding=1) uniform sampler2D FullbrightTex;

#if MODE == 2
	layout(location=0) noperspective in vec2 in_texcoord;
#else
	layout(location=0) in vec2 in_texcoord;
#endif
layout(location=1) in vec4 in_color;
layout(location=2) in vec3 in_pos;

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
	vec2 uv = in_texcoord;
#if MODE == 2
	uv -= 0.5 / vec2(textureSize(Tex, 0).xy);
	vec4 result = textureLod(Tex, uv, 0.);
#else
	vec4 result = texture(Tex, uv);
#endif
#if ALPHATEST
	if (result.a < 0.666)
		discard;
	result.rgb *= in_color.rgb;
#else
	result.rgb = mix(result.rgb, result.rgb * in_color.rgb, result.a);
#endif
	result.a = in_color.a; // FIXME: This will make almost transparent things cut holes though heavy fog
#if MODE == 2
	result.rgb += textureLod(FullbrightTex, uv, 0.).rgb;
#else
	result.rgb += texture(FullbrightTex, uv).rgb;
#endif
	result.rgb = clamp(result.rgb, 0.0, 1.0);
	float fog = exp2(abs(Fog.w) * -dot(in_pos, in_pos));
	fog = clamp(fog, 0.0, 1.0);
	result.rgb = mix(Fog.rgb, result.rgb, fog);
	out_fragcolor = result;
#if MODE == 1 || MODE == 2
	// Note: sign bit is used as overbright flag
	if (abs(Fog.w) > 0.)
	{
		out_fragcolor.rgb = sqrt(out_fragcolor.rgb);
		out_fragcolor.rgb += SCREEN_SPACE_NOISE() * ScreenDither;
		out_fragcolor.rgb *= out_fragcolor.rgb;
	}
#else
	out_fragcolor.rgb += SUPPRESS_BANDING() * ScreenDither;
#endif
}
