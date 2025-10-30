#if BINDLESS
        #extension GL_ARB_shader_draw_parameters : require
        #define DRAW_ID                 gl_DrawIDARB
#else
        layout(location=0) uniform int DrawID;
        #define DRAW_ID                 DrawID
#endif

#include "shadow_common.glsl"

struct Call
{
        uint    flags;
        float   wateralpha;
#if BINDLESS
        uvec2   txhandle;
        uvec2   fbhandle;
#else
        int             baseinstance;
        int             padding;
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
        vec4    mat[3];
        float   alpha;
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

layout(location=0) flat out uint out_flags;
layout(location=1) out vec2 out_uv;
#if BINDLESS
        layout(location=2) flat out uvec2 out_sampler;
#endif

void main()
{
        Call call = call_data[DRAW_ID];
        int instance_id = GET_INSTANCE_ID(call);
        Instance instance = instance_data[instance_id];
        vec3 world_pos = Transform(in_pos, instance);
        int cascadeIndex = int(ShadowCascadeFade.w + 0.5);
        cascadeIndex = clamp(cascadeIndex, 0, 3);
        gl_Position = ShadowViewProj[cascadeIndex] * vec4(world_pos, 1.0);
        out_flags = call.flags;
        out_uv = in_uv.xy;
#if BINDLESS
        out_sampler = call.txhandle;
#endif
}
