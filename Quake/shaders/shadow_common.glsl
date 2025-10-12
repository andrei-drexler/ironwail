#ifndef SHADOW_COMMON_GLSL
#define SHADOW_COMMON_GLSL

layout(std140, binding=0) uniform FrameDataUBO
{
        mat4    ViewProj;
        vec4    Fog;
        vec4    SkyFog;
        vec3    WindDir;
        float   WindPhase;
        float   ScreenDither;
        float   TextureDither;
        float   Overbright;
        float   _Pad0;
        vec3    EyePos;
        float   Time;
        float   ZLogScale;
        float   ZLogBias;
        uint    NumLights;
        uint    _Pad1;
        uint    _Pad2;
        uint    _Pad3;
        mat4    ShadowViewProj[4];
        vec4    ShadowParams;
        vec4    ShadowFilter;
        vec4    ShadowVSM;
        vec4    ShadowSunDir;
        vec4    ShadowSunColor;
        vec4    ShadowCascadeSplits;
        vec4    ShadowCascadeStarts;
        vec4    ShadowCascadeFade;
        vec4    ShadowCascadeTexelSize;
        vec4    ShadowDebug;
};

#endif // SHADOW_COMMON_GLSL
