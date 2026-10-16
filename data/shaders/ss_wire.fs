#version 400

uniform vec3 wireColor;

layout (location = 0) out vec4 FragColor;

// Flat unlit wireframe color: the whole point is contrast against the
// shaded fill underneath, so no lighting, no textures here.
void main()
{
  FragColor = vec4(wireColor, 1.0f);
}
