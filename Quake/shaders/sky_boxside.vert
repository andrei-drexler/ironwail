layout(location=0) uniform mat4 MVP;
layout(location=1) uniform vec3 EyePos;

layout(location=0) in vec3 in_dir;
layout(location=1) in vec2 in_uv;

layout(location=0) out vec3 out_dir;
layout(location=1) out vec2 out_uv;

void main()
{
	gl_Position = MVP * vec4(EyePos + in_dir, 1.0);
	gl_Position.z = gl_Position.w; // map to far plane
	out_dir = in_dir;
	out_uv = in_uv;
}
