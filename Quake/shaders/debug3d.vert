layout(std140, binding=0) uniform FrameDataUBO
{
	mat4	ViewProj;
	vec4	Fog;
	vec4	SkyFog;
	vec3	WindDir;
	float	WindPhase;
	float	ScreenDither;
	float	TextureDither;
	float	Overbright;
	float	_Pad0;
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

layout(location=0) out vec4 out_color;

void main()
{
	gl_Position = ViewProj * vec4(in_pos, 1.0);
	out_color = in_color;
}
