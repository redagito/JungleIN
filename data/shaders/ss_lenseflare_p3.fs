#version 400

in vec2 uv;

uniform sampler2D texScene;
uniform sampler2D texFlare;
uniform sampler2D texDirt;
uniform float flareIntensity;

layout (location = 0) out vec4 FragColor;


void main()
{
	//FragColor = vec4(uv.x,uv.y,0.0,1.0);
	vec3 texel = vec3(texture(texScene, uv));
	vec3 texelFlare = vec3(texture(texFlare, uv));
	vec3 texelDirt = vec3(texture(texDirt, uv));
	// No 0..1 clamp: ACES in the display pass rolls highlights off.
	// The 1.4 flare gain is untouched so this change isolates the clamp.
  	FragColor = vec4(
  		texel
                + texelDirt * texelFlare * 1.4 * flareIntensity
                + texelFlare * 0.10 * flareIntensity
        , 1.0f );
}
