layout(binding=0) uniform sampler2D GammaTexture;
layout(binding=1) uniform usampler3D PaletteLUT;

layout(std430, binding=0) restrict readonly buffer PaletteBuffer
{
	uint Palette[256];
};

uvec3 UnpackRGB8(uint c)
{
	return uvec3(c, c >> 8, c >> 16) & 255u;
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

layout(location=0) uniform vec4 Params;

layout(location=0) out vec4 out_fragcolor;

void main()
{
	float gamma = Params.x;
	float contrast = Params.y;
	float scale = Params.z;
	float dither = Params.w;
	out_fragcolor = texelFetch(GammaTexture, ivec2(gl_FragCoord), 0);
#if PALETTIZE == 1
	vec2 noiseuv = floor(gl_FragCoord.xy * scale) + 0.5;
	out_fragcolor.rgb = sqrt(out_fragcolor.rgb);
	out_fragcolor.rgb += DITHER_NOISE(noiseuv) * dither;
	out_fragcolor.rgb *= out_fragcolor.rgb;
#endif // PALETTIZE == 1
#if PALETTIZE
	ivec3 clr = ivec3(clamp(out_fragcolor.rgb, 0., 1.) * 127. + 0.5);
	uint remap = Palette[texelFetch(PaletteLUT, clr, 0).x];
	out_fragcolor.rgb = vec3(UnpackRGB8(remap)) * (1./255.);
#else
	out_fragcolor.rgb *= contrast;
	out_fragcolor = vec4(pow(out_fragcolor.rgb, vec3(gamma)), 1.0);
#endif // PALETTIZE
}
