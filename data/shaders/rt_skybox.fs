#version 400

in vec3 pos;

uniform samplerCube texCube;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 FragThreshold;

// Phase 4 unified fog: same FOG_COLOR/DENSITY as the lit shaders. The
// sky has no view depth, so it blends to the fog color at the horizon
// where distant fogged ground meets the sky. Zenith keeps the cubemap.
const vec3 FOG_COLOR = vec3(0.796,0.918,0.949)*0.8;

void main()
{
    vec3 sky = clamp(texture(texCube, pos) * 1.4, 0.0, 1.0).rgb;
    vec3 dir = normalize(pos);
    float h = clamp(dir.y, 0.0, 1.0);
    float fogFactor = pow(1.0 - h, 8.0);
    if (dir.y < 0.0)
        fogFactor = 1.0;
    vec3 col = mix(sky, FOG_COLOR, fogFactor);
    FragColor = vec4(col, 1.0);
    FragThreshold = vec4(0.0, 0.0, 0.0, 1.0);
}
