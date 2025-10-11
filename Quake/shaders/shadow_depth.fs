#if BINDLESS
        #extension GL_ARB_bindless_texture : require
#else
        layout(binding=0) uniform sampler2D Tex;
#endif

#include "shadow_common.glsl"

#if SHADOW_VSM
        layout(location=0) out vec2 out_moments;
#endif

const uint CF_ALPHA_TEST = 8u;

layout(location=0) flat in uint in_flags;
layout(location=1) in vec2 in_uv;
#if BINDLESS
        layout(location=2) flat in uvec2 in_sampler;
#endif

void main()
{
        if ((in_flags & CF_ALPHA_TEST) != 0u)
        {
#if BINDLESS
                sampler2D Tex = sampler2D(in_sampler);
#endif
                if (texture(Tex, in_uv).a < 0.666)
                        discard;
        }
#if SHADOW_VSM
        float depth = gl_FragCoord.z;
        out_moments = vec2(depth, depth * depth + 1e-5);
#endif
}
