#version 400

in vec3 fPosition;
in vec2 fTexcoord;

uniform sampler2D texDiffuse;
uniform sampler2D texAlpha;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 FragThreshold;

// Phase 4: sun stays unfogged by design. It is an emissive billboard
// pinned 6 m from the camera, where exp2 fog is ~1.0 (no-op), and it
// must stay bright for the bloom/god-ray threshold pass.
// Its texture is sRGB like every other color input, so it is linearized
// here and gamma-encoded once in the display pass.

void main()
{
    float texelAlpha = texture(texAlpha, fTexcoord).r;

    vec3 texelDiffuse = pow(texture(texDiffuse, fTexcoord).rgb, vec3(2.2));

    FragColor = vec4(texelDiffuse,texelAlpha);
    FragThreshold = FragColor;
    //FragColor = texture(texDiffuse, fTexcoord);
}
