#version 400

in vec2 uv;

uniform sampler2D texScene;
uniform sampler2D texBloom;

layout (location = 0) out vec4 FragColor;


void main()
{
//FragColor = vec4(uv.x,uv.y,0.0,1.0);
        vec3 texel = vec3(texture(texScene, uv));
        vec3 texelBloom = vec3(texture(texBloom, uv));
        // No 0..1 clamp: this composite feeds ACES in the display pass,
        // which rolls highlights off naturally. Clamping here would crush
        // HDR range back to LDR before tonemapping.
  	FragColor = vec4(texel + texelBloom, 1.0f);
}
