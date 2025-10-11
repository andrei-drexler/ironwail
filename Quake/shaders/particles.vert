layout(std140, binding=0) uniform FrameDataUBO
{
	mat4	ViewProj;
	vec4	Fog;
	vec4	SkyFog;
	vec3	WindDir;
	float	WindPhase;
	float	ScreenDither;
	float	TextureDither;
	vec3	EyePos;
	float	Time;
	float	ZLogScale;
	float	ZLogBias;
	uint	NumLights;
};

vec3 ApplyFog(vec3 clr, vec3 p)
{
	float fog = exp2(-Fog.w * dot(p, p));
	fog = clamp(fog, 0.0, 1.0);
	return mix(Fog.rgb, clr, fog);
}


layout(location=0) in vec3 in_pos;
layout(location=1) in vec4 in_color;

layout(location=0) out vec2 out_uv;
layout(location=1) out vec4 out_color;
layout(location=2) out vec3 out_pos;

layout(location=0) uniform vec3 Params;
#define ProjScale	Params.xy
#define UVScale	Params.z

void main()
{
	// figure the current corner: (-1, -1), (-1, 1), (1, -1) or (1, 1)
	uvec2 flipsign = uvec2(gl_VertexID, gl_VertexID >> 1) << 31;
	vec2 corner = uintBitsToFloat(floatBitsToUint(-1.0) ^ flipsign);

	// project the center of the particle
	gl_Position = ViewProj * vec4(in_pos, 1.0);

	// hack a scale up to keep particles from disappearing
	float depthscale = max(1.0 + gl_Position.w * 0.004, 1.08);

	// perform the billboarding
	gl_Position.xy += ProjScale * uintBitsToFloat(floatBitsToUint(vec2(depthscale)) ^ flipsign);

	out_pos = in_pos - EyePos; // FIXME: use corner position
	out_uv = corner * UVScale;
	out_color = in_color;
#if OIT
	out_color.a *= 0.9;
#endif
}
