layout(location=0) in vec2 in_pos;
layout(location=1) in vec2 in_uv;
layout(location=2) in vec4 in_color;

layout(location=0) centroid out vec2 out_uv;
layout(location=1) centroid out vec4 out_color;

void main()
{
	gl_Position = vec4(in_pos, 0.0, 1.0);
	out_uv = in_uv;
	out_color = in_color;
}
