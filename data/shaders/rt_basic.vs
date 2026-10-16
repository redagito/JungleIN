#version 400

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texcoord;
layout (location = 2) in vec3 normal;
// Per-instance world matrix (scene base composed on the CPU); divisor
// set once per upload in Mesh::uploadInstances. One instanced draw
// replaces ~1400 per-instance draws per mesh per pass.
layout (location = 3) in mat4 instanceModel;

uniform mat4 proj;
uniform mat4 view;
uniform mat3 viewMatrix;
uniform vec3 lightDir;
// Light clip chain without the model (bias*proj*view): the model half
// varies per instance, so it multiplies here instead of arriving baked
// per draw like the old shadowMatrix uniforms did.
uniform mat4 shadowViewProj;
uniform mat4 shadowViewProjCascade;

out vec2 fTexcoord;
out vec3 fNormal;
out vec4 fShadowcoord;
out vec4 fShadowcoordCascade;
out vec3 fLightDir;
out float vViewDepth;


void main()
{
  vec4 fPosition = vec4(position, 1.0f);
  vec4 worldPos = instanceModel * fPosition;
  vec4 viewPos = view * worldPos;

  gl_Position = proj * viewPos;

  fTexcoord = texcoord;
  // Object-space normal to view space. Directions carry no translation.
  // Per-instance matrices forbid a shared normalMatrix uniform, so the
  // inverse-transpose is evaluated here (moved from the CPU loop, where
  // it cost a determinant+inverse per instance per pass).
  mat3 normalMatrix = transpose(inverse(mat3(view * instanceModel)));
  fNormal = normalMatrix * normal;

  fShadowcoord = shadowViewProj * worldPos;
  fShadowcoordCascade = shadowViewProjCascade * worldPos;

  // World-space light direction to view space: rotation only.
  fLightDir = viewMatrix * lightDir;

  // View-space forward distance for fog (Phase 4 owns the fog formula).
  // Identical to the old -(mv * fPosition).z: mv was exactly this
  // view*instance product, transmitted per draw.
  vViewDepth = -viewPos.z;
}
