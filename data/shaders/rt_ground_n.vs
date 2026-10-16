#version 400

layout (location = 0) in vec3 position;
layout (location = 2) in vec3 normal;

uniform mat4 mvp;
uniform mat4 mv;
uniform mat3 normalMatrix;
uniform mat3 viewMatrix;
uniform mat4 shadowMatrix;
uniform mat4 shadowMatrixCascade;
uniform int cascadeShadow;
uniform vec3 lightDir;

out vec3 fPosition;
out vec3 fNormal;
out vec4 fShadowcoord;
out vec4 fShadowcoordCascade;
out vec3 fLightDir;
out vec3 vViewPos;
out float vViewDepth;

out vec3 fTangent;
out vec3 fBinormal;

void computeTangentSpace(vec3 normal)
{
	vec3 tangent;
	vec3 binormal;

	vec3 c1 = cross(normal, vec3(0.0, 0.0, 1.0));
	vec3 c2 = cross(normal, vec3(0.0, 1.0, 0.0));

	if(length(c1)>length(c2))
	{
		tangent = c1;
	}
	else
	{
		tangent = c2;
	}

	fTangent = normalize(tangent);

	binormal = cross(normal, tangent);
	fBinormal = normalize(binormal);
}

void main()
{
  gl_Position = mvp * vec4(position, 1.0f);

  // Object-space position: stable tiling coordinates for the ground maps.
  fPosition = position;
  fShadowcoord = shadowMatrix * vec4(position, 1.0f);
  fShadowcoordCascade = shadowMatrixCascade * vec4(position, 1.0f);
  // World-space light direction to view space: rotation only.
  fLightDir = viewMatrix * lightDir;

  // Object-space normal to view space via the inverse-transpose.
  vec3 viewNormal = normalMatrix * normalize(normal);
  fNormal = viewNormal;
  computeTangentSpace(viewNormal);

  // View-space position feeds the specular view vector and fog depth.
  vec4 viewPos = mv * vec4(position, 1.0f);
  vViewPos = viewPos.xyz;
  vViewDepth = -viewPos.z;
}
