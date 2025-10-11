#if BINDLESS
	#extension GL_ARB_shader_draw_parameters : require
	#define DRAW_ID			gl_DrawIDARB
#else
	layout(location=0) uniform int DrawID;
	#define DRAW_ID			DrawID
#endif

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
	CF_NOLIGHTMAP = 4u
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

layout(location=0) in vec3 in_pos;
layout(location=1) in vec4 in_uv;
layout(location=2) in float in_lmofs;
layout(location=3) in ivec4 in_styles;


layout(location=0) flat out float out_alpha;layout(location=1) out vec2 out_uv;
layout(location=2) out vec3 out_pos;
#if BINDLESS
	layout(location=3) flat out uvec2 out_sampler;
#endif

void main()
{
	Call call = call_data[DRAW_ID];
	int instance_id = GET_INSTANCE_ID(call);
	Instance instance = instance_data[instance_id];
	vec3 pos = Transform(in_pos, instance);
	gl_Position = ViewProj * vec4(pos, 1.0);
	out_uv = in_uv.xy;
	out_pos = pos - EyePos;
	out_alpha = instance.alpha < 0.0 ? call.wateralpha : instance.alpha;
#if BINDLESS
	out_sampler = call.txhandle;
#endif
}
