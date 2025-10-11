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

float bayer01(ivec2 coord)
{
        coord &= 15;
        coord.y ^= coord.x;
        uint v = uint(coord.y | (coord.x << 8));
        v = (v ^ (v << 2)) & 0x3333u;
        v = (v ^ (v << 1)) & 0x5555u;
        v |= v >> 7;
        v = bitfieldReverse(v) >> 24;
        return float(v) * (1.0 / 256.0);
}

float bayer(ivec2 coord)
{
        return bayer01(coord) - 0.5;
}

layout(location=0) in vec2 in_uv;
layout(location=1) in float in_alpha;
layout(location=2) in vec3 in_pos;

layout(location=0) out vec4 out_fragcolor;

void main()
{
        float distSq = dot(in_uv, in_uv);
        if (distSq > 1.0)
                discard;

        float falloff = 1.0 - distSq;
        falloff *= falloff;
        float alpha = in_alpha * falloff;
        if (alpha <= 0.0)
                discard;

        float fog = exp2(abs(Fog.w) * -dot(in_pos, in_pos));
        fog = clamp(fog, 0.0, 1.0);

        vec3 color = mix(Fog.rgb, vec3(0.0), fog);
        out_fragcolor = vec4(color, alpha);

        out_fragcolor.rgb += bayer(ivec2(gl_FragCoord.xy)) * ScreenDither;
        out_fragcolor = clamp(out_fragcolor, 0.0, 1.0);
}
