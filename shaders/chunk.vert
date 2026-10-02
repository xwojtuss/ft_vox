#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
} ubo;

layout(std430, set = 0, binding = 1) readonly buffer ObjectBuffer {
	mat4 models[];
} objects;

layout(constant_id = 0) const uint POSITION_X_WORD = 0;
layout(constant_id = 1) const uint POSITION_X_SHIFT = 0;
layout(constant_id = 2) const uint POSITION_X_BITS = 1;
layout(constant_id = 3) const uint POSITION_Y_WORD = 0;
layout(constant_id = 4) const uint POSITION_Y_SHIFT = 0;
layout(constant_id = 5) const uint POSITION_Y_BITS = 1;
layout(constant_id = 6) const uint POSITION_Z_WORD = 0;
layout(constant_id = 7) const uint POSITION_Z_SHIFT = 0;
layout(constant_id = 8) const uint POSITION_Z_BITS = 1;
layout(constant_id = 9) const uint UV_U_WORD = 0;
layout(constant_id = 10) const uint UV_U_SHIFT = 0;
layout(constant_id = 11) const uint UV_U_BITS = 1;
layout(constant_id = 12) const uint UV_V_WORD = 0;
layout(constant_id = 13) const uint UV_V_SHIFT = 0;
layout(constant_id = 14) const uint UV_V_BITS = 1;
layout(constant_id = 15) const uint POSITION_STEPS_PER_BLOCK = 1;
layout(constant_id = 16) const uint UV_STEPS_PER_TEXTURE = 1;

layout(location = 0) in uvec4 inVertex;

layout(location = 0) out vec2 fragTexCoord;

float unpack(uint word, uint shift, uint bits) {
	return float((inVertex[word] >> shift) & ((1u << bits) - 1u));
}

void	main() {
	vec3 position = vec3(
		unpack(POSITION_X_WORD, POSITION_X_SHIFT, POSITION_X_BITS),
		unpack(POSITION_Y_WORD, POSITION_Y_SHIFT, POSITION_Y_BITS),
		unpack(POSITION_Z_WORD, POSITION_Z_SHIFT, POSITION_Z_BITS)) / float(POSITION_STEPS_PER_BLOCK);

	gl_Position = ubo.proj * ubo.view * objects.models[gl_InstanceIndex] * vec4(position, 1.0);
	fragTexCoord = vec2(
		unpack(UV_U_WORD, UV_U_SHIFT, UV_U_BITS),
		unpack(UV_V_WORD, UV_V_SHIFT, UV_V_BITS)) / float(UV_STEPS_PER_TEXTURE);
}
