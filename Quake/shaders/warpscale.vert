layout(location=0) out vec2 out_uv;

void main()
{
	ivec2 v = ivec2(gl_VertexID & 1, gl_VertexID >> 1);
	out_uv = vec2(v) * 2.0;
	gl_Position = vec4(out_uv * 2.0 - 1.0, 0.0, 1.0);
}
