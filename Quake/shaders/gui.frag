layout(binding=0) uniform sampler2D Tex;

layout(location=0) centroid in vec2 in_uv;
layout(location=1) centroid in vec4 in_color;

layout(location=0) out vec4 out_fragcolor;

void main()
{
	out_fragcolor = texture(Tex, in_uv) * in_color;
}
