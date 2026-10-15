#version 400

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texcoord;
layout (location = 2) in vec3 normal;

uniform mat4 mvp;
uniform mat4 mv;
uniform mat3 normalMatrix;
uniform mat3 viewMatrix;
uniform mat4 shadowMatrix;
uniform mat4 shadowMatrixCascade;
uniform int cascadeShadow;
uniform vec3 lightDir;

out vec2 fTexcoord;
out vec3 fNormal;
out vec4 fShadowcoord;
out vec4 fShadowcoordCascade;
out vec3 fLightDir;
out float vViewDepth;


void main()
{
  vec4 fPosition = vec4(position, 1.0f);

  gl_Position = mvp * fPosition;

  fTexcoord = texcoord;
  // Object-space normal to view space. Directions carry no translation.
  fNormal = normalMatrix * normal;

  fShadowcoord = shadowMatrix * fPosition;
  fShadowcoordCascade = shadowMatrixCascade * fPosition;

  // World-space light direction to view space: rotation only.
  fLightDir = viewMatrix * lightDir;

  // View-space forward distance for fog (Phase 4 owns the fog formula).
  vViewDepth = -(mv * fPosition).z;
}
