layout(binding=0) uniform sampler2D Tex;

layout(location=0) uniform vec4 UVScaleWarpTime; // xy=Scale z=Warp w=Time
layout(location=1) uniform vec4 BlendColor;

layout(location=0) in vec2 in_uv;

layout(location=0) out vec4 out_fragcolor;

void main()
{
	vec2 uv = in_uv;
	vec2 uv_scale = UVScaleWarpTime.xy;

#if WARP
	float time = UVScaleWarpTime.w;
	float aspect = dFdy(uv.y) / dFdx(uv.x);
	vec2 warp_amp = UVScaleWarpTime.zz;
	warp_amp.y *= aspect;
	uv = warp_amp + uv * (1.0 - 2.0 * warp_amp); // remap to safe area
	uv += warp_amp * sin(vec2(uv.y / aspect, uv.x) * (3.14159265 * 8.0) + time);
#endif // WARP

	out_fragcolor = texture(Tex, uv * uv_scale);
	out_fragcolor.rgb = mix(out_fragcolor.rgb, BlendColor.rgb, BlendColor.a);
}
