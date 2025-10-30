struct InstanceData
{
	vec4	WorldMatrix[3];
	vec4	LightColor; // xyz=LightColor w=Alpha
	int		Pose1;
	int		Pose2;
	float	Blend;
	int		Padding;
};

layout(std430, binding=1) restrict readonly buffer InstanceBuffer
{
	mat4	ViewProj;
	vec3	EyePos;
	vec4	Fog;
	float	ScreenDither;
	InstanceData instances[];
};

struct PoseVertex
{
	vec3 pos;
	vec3 nor;
};

#if MD5
	layout(location=0) in vec3 in_pos;
	layout(location=1) in vec4 in_nor;
	layout(location=2) in vec2 in_uv;
	layout(location=3) in vec4 in_weights;
	layout(location=4) in ivec4 in_indices;

	layout(std430, binding=2) restrict readonly buffer PoseBuffer
	{
		mat3x4 BonePoses[];
	};

	PoseVertex GetPoseVertex(uint pose)
	{
		mat3x4 blendmat = BonePoses[pose + in_indices.x] * in_weights.x;
		blendmat += BonePoses[pose + in_indices.y] * in_weights.y;
		if (in_weights.z + in_weights.w > 0.0)
		{
			blendmat += BonePoses[pose + in_indices.z] * in_weights.z;
			blendmat += BonePoses[pose + in_indices.w] * in_weights.w;
		}
		mat4x3 anim = transpose(blendmat);
		return PoseVertex((anim * vec4(in_pos, 1.0)).xyz, (anim * vec4(in_nor.xyz, 0.0)).xyz);
	}

#else
	layout(location=0) in vec2 in_uv;

	layout(std430, binding=2) restrict readonly buffer BlendShapeBuffer
	{
		uvec2 PackedPosNor[];
	};

	PoseVertex GetPoseVertex(uint pose)
	{
		uvec2 data = PackedPosNor[pose + gl_VertexID];
		return PoseVertex(vec3((data.xxx >> uvec3(0, 8, 16)) & 255), unpackSnorm4x8(data.y).xyz);
	}

#endif // MD5

float r_avertexnormal_dot(vec3 vertexnormal, vec3 dir) // from MH 
{
	float d = dot(vertexnormal, dir);
	// wtf - this reproduces anorm_dots within as reasonable a degree of tolerance as the >= 0 case
	if (d < 0.0)
		return 1.0 + d * (13.0 / 44.0);
	else
		return 1.0 + d;
}

#if MODE == 2
	layout(location=0) noperspective out vec2 out_texcoord;
#else
	layout(location=0) out vec2 out_texcoord;
#endif
layout(location=1) out vec4 out_color;
layout(location=2) out vec3 out_pos;

void main()
{
	InstanceData inst = instances[gl_InstanceID];
	out_texcoord = in_uv;
	PoseVertex pose1 = GetPoseVertex(inst.Pose1);
	PoseVertex pose2 = GetPoseVertex(inst.Pose2);
	mat4x3 worldmatrix = transpose(mat3x4(inst.WorldMatrix[0], inst.WorldMatrix[1], inst.WorldMatrix[2]));
	vec3 lerpedVert = (worldmatrix * vec4(mix(pose1.pos, pose2.pos, inst.Blend), 1.0)).xyz;
	gl_Position = ViewProj * vec4(lerpedVert, 1.0);
	out_pos = lerpedVert - EyePos;
	// transform world X and Z axes to local space
	mat3 orientation = mat3(normalize(worldmatrix[0].xyz), normalize(worldmatrix[1].xyz), normalize(worldmatrix[2].xyz));
	orientation = transpose(orientation);
	vec3 shadevector = (orientation[0] + orientation[2]) / sqrt(2.0);
	float dot1 = r_avertexnormal_dot(pose1.nor, shadevector);
	float dot2 = r_avertexnormal_dot(pose2.nor, shadevector);
	out_color = clamp(inst.LightColor * vec4(vec3(mix(dot1, dot2, inst.Blend)), 1.0), 0.0, 1.0);
	uint overbright = floatBitsToUint(Fog.w) >> 31;
	out_color.rgb = ldexp(out_color.rgb, ivec3(overbright));
}
