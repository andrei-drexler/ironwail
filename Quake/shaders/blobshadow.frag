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

#undef OUT_COLOR
#define OUT_COLOR out_fragcolor
#define main main_body
#else
        layout(location=0) out vec4 OUT_COLOR;
#endif // OIT

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
        OUT_COLOR = vec4(color, alpha);

        OUT_COLOR.rgb += bayer(ivec2(gl_FragCoord.xy)) * ScreenDither;
        OUT_COLOR = clamp(OUT_COLOR, 0.0, 1.0);
}
