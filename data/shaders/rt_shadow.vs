#version 400

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texcoord;
// Per-instance world matrix (scene base composed on the CPU); divisor
// set once per upload in Mesh::uploadInstances. The shadow pass draws
// every mesh once instanced instead of once per instance per cascade.
layout (location = 3) in mat4 instanceModel;

uniform mat4 proj;
uniform mat4 view;

out vec2 fTexcoord;

void main()
{
  gl_Position = proj * view * instanceModel * vec4(position, 1.0f);
  fTexcoord = texcoord;
}
