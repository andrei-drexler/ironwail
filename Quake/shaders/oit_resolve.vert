void main()
{
	ivec2 v = ivec2(gl_VertexID & 1, gl_VertexID >> 1);
	gl_Position = vec4(vec2(v) * 4.0 - 1.0, 0.0, 1.0);
}
