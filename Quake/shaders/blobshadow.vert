struct ShadowInstance
{
        vec4    CenterRadius;
        vec4    Params;
};

layout(std430, binding=1) restrict readonly buffer InstanceBuffer
{
        mat4    ViewProj;
        vec3    EyePos;
        float   _Pad0;
        vec4    Fog;
        float   ScreenDither;
        vec3    _Pad1;
        ShadowInstance instances[];
};

layout(location=0) in vec2 in_pos;

layout(location=0) out vec2 out_uv;
layout(location=1) out float out_alpha;
layout(location=2) out vec3 out_pos;

void main()
{
        ShadowInstance inst = instances[gl_InstanceID];
        vec2 radii = vec2(inst.CenterRadius.w, inst.Params.x);
        vec2 offset = in_pos * radii;
        vec3 worldPos = inst.CenterRadius.xyz + vec3(offset, 0.0);
        out_uv = in_pos;
        out_alpha = inst.Params.y;
        out_pos = worldPos - EyePos;
        gl_Position = ViewProj * vec4(worldPos, 1.0);
}
