layout(early_fragment_tests) in;

#if MSAA
	#define Sampler				sampler2DMS
	#define FetchSample(s, c)	texelFetch(s, c, gl_SampleID)
#else
	#define Sampler				sampler2D
	#define FetchSample(s, c)	texelFetch(s, c, 0)
#endif

layout(binding=0) uniform Sampler TexAccum;
layout(binding=1) uniform Sampler TexReveal;

layout(location=0) out vec4 out_fragcolor;

vec3 LinearToGamma(vec3 v)
{
#if 0
	return sqrt(clamp(v, 0.0, 1.0));
#else
	return clamp(v, 0.0, 1.0);
#endif
}

// get the max value between three values
float max3(vec3 v)
{
	return max(max(v.x, v.y), v.z);
}

void main()
{
	ivec2 coords = ivec2(gl_FragCoord.xy);
	float revealage = FetchSample(TexReveal, coords).r;
	// Note: we're using the stencil buffer to discard pixels with no contribution
	//if (revealage >= 0.999)
	//	discard;

	vec4 accumulation = FetchSample(TexAccum, coords);
	// suppress overflow
	if (isinf(max3(abs(accumulation.rgb))))
		accumulation.rgb = vec3(accumulation.a);

	vec3 average_color = accumulation.rgb / max(accumulation.a, 1e-5);
	out_fragcolor = vec4(LinearToGamma(average_color), 1.0 - revealage);
}
